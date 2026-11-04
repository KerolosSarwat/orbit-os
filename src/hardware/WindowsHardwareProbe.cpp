#include "WindowsHardwareProbe.h"

#include <QtGlobal>

#ifdef Q_OS_WIN

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>

#include <QJsonArray>
#include <QList>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSettings>
#include <QSysInfo>
#include <QUuid>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>
#include <vector>

namespace Orbit::Hardware {
namespace {

// Provider signatures are packed as their four ASCII bytes: 'RSMB'.
constexpr DWORD kSmbiosProvider = 0x52534D42;

QString windowsErrorNote(DWORD error)
{
    return QStringLiteral("Windows API error %1").arg(error);
}

QJsonValue jsonUnsigned(quint64 value)
{
    // Current desktop memory capacities and device counters are below the
    // exact-integer limit of IEEE-754 doubles used by QJsonValue.
    return QJsonValue(static_cast<double>(value));
}

QJsonObject known(const QJsonValue &value,
                  const QString &source,
                  EvidenceState state = EvidenceState::Verified,
                  Confidence confidence = Confidence::High,
                  const QString &unit = QString(),
                  const QString &note = QString())
{
    return EvidenceValue::known(value, source, state, confidence, unit, note).toJson();
}

QJsonObject unavailable(EvidenceState state,
                        const QString &source,
                        const QString &note)
{
    return EvidenceValue::unavailable(state, source, note).toJson();
}

QString vendorNameFromPciId(USHORT vendorId)
{
    switch (vendorId) {
    case 0x8086: return QStringLiteral("Intel");
    case 0x10DE: return QStringLiteral("NVIDIA");
    case 0x1002: return QStringLiteral("AMD");
    case 0x1022: return QStringLiteral("AMD");
    case 0x1414: return QStringLiteral("Microsoft");
    default: return QStringLiteral("Unknown");
    }
}

QString hexId(USHORT value)
{
    return QStringLiteral("0x%1").arg(value, 4, 16, QLatin1Char('0')).toUpper();
}

QString smbiosStringAt(const BYTE *table,
                       size_t tableLength,
                       size_t stringsOffset,
                       BYTE stringIndex)
{
    if (!table || stringIndex == 0 || stringsOffset >= tableLength)
        return {};

    size_t cursor = stringsOffset;
    BYTE currentIndex = 1;

    while (cursor < tableLength && table[cursor] != 0) {
        const size_t start = cursor;
        while (cursor < tableLength && table[cursor] != 0)
            ++cursor;

        if (cursor >= tableLength)
            return {};

        if (currentIndex == stringIndex)
            return QString::fromLatin1(reinterpret_cast<const char *>(table + start),
                                       static_cast<qsizetype>(cursor - start)).trimmed();

        ++currentIndex;
        ++cursor;
    }

    return {};
}

struct SmbiosFacts
{
    QString manufacturer;
    QString model;
    QString sku;
    QString boardManufacturer;
    QString boardProduct;
    QString biosVendor;
    QString biosVersion;
    QString biosReleaseDate;
    QString version;
};

bool readSmbiosFacts(SmbiosFacts *facts, QString *error)
{
    if (!facts) {
        if (error)
            *error = QStringLiteral("SMBIOS output pointer is null");
        return false;
    }

    const UINT required = GetSystemFirmwareTable(kSmbiosProvider, 0, nullptr, 0);
    if (required < 8) {
        if (error)
            *error = windowsErrorNote(GetLastError());
        return false;
    }

    std::vector<BYTE> raw(required);
    const UINT returned = GetSystemFirmwareTable(kSmbiosProvider, 0,
                                                  raw.data(), required);
    if (returned < 8 || returned > raw.size()) {
        if (error)
            *error = windowsErrorNote(GetLastError());
        return false;
    }

    // RAW_SMBIOS_DATA starts with four one-byte fields followed by a DWORD
    // length. The explicit offsets avoid relying on SDK-specific definitions.
    const BYTE major = raw[1];
    const BYTE minor = raw[2];
    DWORD tableLength = 0;
    std::memcpy(&tableLength, raw.data() + 4, sizeof(tableLength));
    const size_t headerSize = 8;
    const size_t safeLength = std::min<size_t>(
        static_cast<size_t>(tableLength),
        static_cast<size_t>(returned) - headerSize);
    const BYTE *table = raw.data() + headerSize;

    facts->version = QStringLiteral("%1.%2").arg(major).arg(minor);

    size_t offset = 0;
    while (offset + 4 <= safeLength) {
        const BYTE type = table[offset];
        const BYTE length = table[offset + 1];
        if (length < 4 || offset + length > safeLength)
            break;

        const BYTE *record = table + offset;
        const size_t stringsOffset = offset + length;

        const auto stringField = [&](size_t fieldOffset) -> QString {
            if (fieldOffset >= length)
                return {};
            return smbiosStringAt(table, safeLength, stringsOffset, record[fieldOffset]);
        };

        if (type == 0) { // BIOS Information
            if (facts->biosVendor.isEmpty())
                facts->biosVendor = stringField(4);
            if (facts->biosVersion.isEmpty())
                facts->biosVersion = stringField(5);
            if (facts->biosReleaseDate.isEmpty())
                facts->biosReleaseDate = stringField(8);
        } else if (type == 1) { // System Information
            if (facts->manufacturer.isEmpty())
                facts->manufacturer = stringField(4);
            if (facts->model.isEmpty())
                facts->model = stringField(5);
            if (facts->sku.isEmpty())
                facts->sku = stringField(25);
        } else if (type == 2) { // Baseboard Information
            if (facts->boardManufacturer.isEmpty())
                facts->boardManufacturer = stringField(4);
            if (facts->boardProduct.isEmpty())
                facts->boardProduct = stringField(5);
        }

        // Each SMBIOS structure ends with a double NUL after its string set.
        size_t next = stringsOffset;
        while (next + 1 < safeLength
               && !(table[next] == 0 && table[next + 1] == 0)) {
            ++next;
        }
        if (next + 1 >= safeLength)
            break;

        offset = next + 2;
        if (type == 127)
            break;
    }

    if (error)
        error->clear();
    return true;
}

struct CpuTopology
{
    int physicalCores = 0;
    int logicalProcessors = 0;
    int activeLogicalProcessorsFromWindows = 0;
    QMap<int, int> efficiencyClassCounts;
};

bool readCpuTopology(CpuTopology *topology, QString *error)
{
    if (!topology) {
        if (error)
            *error = QStringLiteral("CPU topology output pointer is null");
        return false;
    }

    DWORD required = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &required);
    if (required == 0 || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        if (error)
            *error = windowsErrorNote(GetLastError());
        return false;
    }

    std::vector<BYTE> buffer(required);
    DWORD returned = required;
    if (!GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()),
            &returned)) {
        if (error)
            *error = windowsErrorNote(GetLastError());
        return false;
    }

    // The API returns a sequence of variable-sized records. Only require
    // enough bytes for the fixed Relationship + Size header here; the
    // record's own Size is validated before any union member is accessed.
    constexpr size_t fixedHeaderSize =
        offsetof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, Processor);
    size_t offset = 0;
    while (offset + fixedHeaderSize <= returned) {
        const auto *info =
            reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>(buffer.data() + offset);
        constexpr size_t minimumRecordSize =
            offsetof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, Processor)
            + offsetof(PROCESSOR_RELATIONSHIP, GroupMask)
            + sizeof(GROUP_AFFINITY);
        if (info->Size < minimumRecordSize || offset + info->Size > returned)
            break;

        if (info->Relationship == RelationProcessorCore) {
            const PROCESSOR_RELATIONSHIP &processor = info->Processor;
            const size_t requiredForGroups =
                offsetof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, Processor)
                + offsetof(PROCESSOR_RELATIONSHIP, GroupMask)
                + (static_cast<size_t>(processor.GroupCount) * sizeof(GROUP_AFFINITY));
            if (processor.GroupCount == 0 || requiredForGroups > info->Size)
                break;

            ++topology->physicalCores;
            for (WORD group = 0; group < processor.GroupCount; ++group) {
                const KAFFINITY mask = processor.GroupMask[group].Mask;
                topology->logicalProcessors += static_cast<int>(
                    std::popcount(static_cast<unsigned long long>(mask)));
            }

            // PROCESSOR_RELATIONSHIP.EfficiencyClass is byte 1 in the
            // documented Windows ABI (after Flags). Some MinGW-w64 SDK
            // revisions omit the named member while retaining the ABI layout.
            // Read the documented byte offset so those toolchains still build.
            constexpr size_t efficiencyClassOffset =
                offsetof(PROCESSOR_RELATIONSHIP, Flags) + sizeof(BYTE);
            static_assert(offsetof(PROCESSOR_RELATIONSHIP, GroupCount)
                          > efficiencyClassOffset,
                          "PROCESSOR_RELATIONSHIP layout is incompatible with Windows ABI");
            const auto *processorBytes =
                reinterpret_cast<const BYTE *>(&processor);
            const BYTE efficiencyClass = processorBytes[efficiencyClassOffset];

            // Zero is a valid class; count it like every other class.
            topology->efficiencyClassCounts[efficiencyClass] += 1;
        }

        offset += info->Size;
    }

    if (topology->physicalCores <= 0 || topology->logicalProcessors <= 0) {
        if (error)
            *error = QStringLiteral("Windows returned no active processor-core topology");
        return false;
    }

    // Cross-check the sum of per-core affinity masks against the independent
    // system-wide active logical-processor count. This catches parser mistakes
    // and incomplete traversal of the variable-sized records.
    const DWORD activeLogicalProcessors = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    topology->activeLogicalProcessorsFromWindows =
        static_cast<int>(activeLogicalProcessors);
    if (activeLogicalProcessors == 0
        || topology->logicalProcessors != static_cast<int>(activeLogicalProcessors)) {
        if (error) {
            *error = QStringLiteral(
                "CPU topology mismatch: per-core affinity masks report %1 logical processors, "
                "but GetActiveProcessorCount reports %2.")
                .arg(topology->logicalProcessors)
                .arg(activeLogicalProcessors);
        }
        return false;
    }

    if (error)
        error->clear();
    return true;
}

QStringList readMultiStringProperty(HDEVINFO deviceInfoSet,
                                    SP_DEVINFO_DATA *deviceInfo,
                                    DWORD property)
{
    DWORD propertyType = 0;
    DWORD required = 0;
    SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, deviceInfo, property,
                                      &propertyType, nullptr, 0, &required);
    if (required == 0 || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return {};

    std::vector<BYTE> buffer(required + sizeof(wchar_t));
    if (!SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, deviceInfo, property,
                                           &propertyType, buffer.data(),
                                           static_cast<DWORD>(buffer.size()), nullptr)) {
        return {};
    }

    QStringList values;
    if (propertyType == REG_MULTI_SZ) {
        const wchar_t *cursor = reinterpret_cast<const wchar_t *>(buffer.data());
        while (*cursor != L'\0') {
            const QString value = QString::fromWCharArray(cursor);
            if (!value.isEmpty())
                values.append(value);
            cursor += value.size() + 1;
        }
    } else if (propertyType == REG_SZ) {
        const QString value = QString::fromWCharArray(
            reinterpret_cast<const wchar_t *>(buffer.data()));
        if (!value.isEmpty())
            values.append(value);
    }
    return values;
}

QString readStringProperty(HDEVINFO deviceInfoSet,
                           SP_DEVINFO_DATA *deviceInfo,
                           DWORD property)
{
    const QStringList values = readMultiStringProperty(deviceInfoSet, deviceInfo, property);
    return values.isEmpty() ? QString() : values.first();
}

struct PnpDisplayDevice
{
    QString description;
    QString manufacturer;
    QString driverKey;
    QString hardwareId;
    USHORT vendorId = 0;
    USHORT deviceId = 0;
    bool hasPciId = false;
};

QList<PnpDisplayDevice> enumeratePresentDisplayDevices(QStringList *warnings)
{
    QList<PnpDisplayDevice> devices;
    HDEVINFO set = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY,
                                        nullptr, nullptr, DIGCF_PRESENT);
    if (set == INVALID_HANDLE_VALUE) {
        if (warnings)
            warnings->append(QStringLiteral("Display adapter enumeration failed: %1")
                                 .arg(windowsErrorNote(GetLastError())));
        return devices;
    }

    const QRegularExpression pciPattern(
        QStringLiteral("VEN_([0-9A-Fa-f]{4}).*DEV_([0-9A-Fa-f]{4})"));

    for (DWORD index = 0;; ++index) {
        SP_DEVINFO_DATA info{};
        info.cbSize = sizeof(info);
        if (!SetupDiEnumDeviceInfo(set, index, &info)) {
            const DWORD error = GetLastError();
            if (error != ERROR_NO_MORE_ITEMS && warnings) {
                warnings->append(QStringLiteral("Display device enumeration stopped: %1")
                                     .arg(windowsErrorNote(error)));
            }
            break;
        }

        PnpDisplayDevice device;
        device.description = readStringProperty(set, &info, SPDRP_FRIENDLYNAME);
        if (device.description.isEmpty())
            device.description = readStringProperty(set, &info, SPDRP_DEVICEDESC);
        device.manufacturer = readStringProperty(set, &info, SPDRP_MFG);
        device.driverKey = readStringProperty(set, &info, SPDRP_DRIVER);

        const QStringList hardwareIds =
            readMultiStringProperty(set, &info, SPDRP_HARDWAREID);
        for (const QString &hardwareId : hardwareIds) {
            const QRegularExpressionMatch match = pciPattern.match(hardwareId);
            if (!match.hasMatch())
                continue;

            bool vendorOk = false;
            bool deviceOk = false;
            const uint vendor = match.captured(1).toUInt(&vendorOk, 16);
            const uint product = match.captured(2).toUInt(&deviceOk, 16);
            if (vendorOk && deviceOk && vendor <= 0xFFFF && product <= 0xFFFF) {
                device.vendorId = static_cast<USHORT>(vendor);
                device.deviceId = static_cast<USHORT>(product);
                device.hardwareId = hardwareId;
                device.hasPciId = true;
                break;
            }
        }

        devices.append(device);
    }

    SetupDiDestroyDeviceInfoList(set);
    return devices;
}

QJsonArray buildGraphicsInventory(QStringList *warnings)
{
    QJsonArray graphics;
    const QList<PnpDisplayDevice> devices = enumeratePresentDisplayDevices(warnings);
    const QString source = QStringLiteral("Windows:SetupAPI/GUID_DEVCLASS_DISPLAY/DIGCF_PRESENT");

    for (const PnpDisplayDevice &device : devices) {
        QJsonObject adapter;
        adapter.insert(QStringLiteral("role"),
                       QStringLiteral("unknown"));
        adapter.insert(QStringLiteral("roleNote"),
                       QStringLiteral("Adapter role is not inferred from vendor name alone; later profile matching or display-path discovery will classify it."));

        adapter.insert(QStringLiteral("model"),
                       device.description.isEmpty()
                           ? unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("Windows did not report a display-device description."))
                           : known(device.description, source, EvidenceState::Present));

        adapter.insert(QStringLiteral("manufacturer"),
                       device.manufacturer.isEmpty()
                           ? unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("Windows did not report the device manufacturer string."))
                           : known(device.manufacturer, source, EvidenceState::Described,
                                   Confidence::Medium));

        adapter.insert(QStringLiteral("vendorId"),
                       device.hasPciId
                           ? known(hexId(device.vendorId), source, EvidenceState::Present)
                           : unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("No PCI vendor ID was exposed by the PnP hardware IDs.")));

        adapter.insert(QStringLiteral("vendorName"),
                       device.hasPciId
                           ? known(vendorNameFromPciId(device.vendorId), source,
                                   EvidenceState::Described, Confidence::High)
                           : unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("Vendor could not be identified from a PCI ID.")));

        adapter.insert(QStringLiteral("deviceId"),
                       device.hasPciId
                           ? known(hexId(device.deviceId), source, EvidenceState::Present)
                           : unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("No PCI device ID was exposed by the PnP hardware IDs.")));

        adapter.insert(QStringLiteral("pnpHardwareId"),
                       device.hardwareId.isEmpty()
                           ? unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("No PCI hardware ID was exposed."))
                           : known(device.hardwareId, source, EvidenceState::Present,
                                   Confidence::High));

        adapter.insert(QStringLiteral("driver"),
                       device.driverKey.isEmpty()
                           ? unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("No associated PnP driver registry key was reported."))
                           : known(device.driverKey, source, EvidenceState::DriverBound,
                                   Confidence::Medium,
                                   QString(),
                                   QStringLiteral("PnP driver association found; runtime operation and version are not yet verified.")));

        adapter.insert(QStringLiteral("dedicatedVideoMemoryBytes"),
                       unavailable(EvidenceState::ProbeSkipped,
                                   QStringLiteral("Windows:passive-inventory-policy"),
                                   QStringLiteral("DXGI runtime memory probing is deferred so the passive scan does not initialize a sleeping discrete GPU.")));

        adapter.insert(QStringLiteral("sharedSystemMemoryBytes"),
                       unavailable(EvidenceState::ProbeSkipped,
                                   QStringLiteral("Windows:passive-inventory-policy"),
                                   QStringLiteral("DXGI runtime memory probing is deferred in the passive scan.")));

        graphics.append(adapter);
    }

    if (devices.isEmpty() && warnings)
        warnings->append(QStringLiteral("No present display adapters were returned by SetupAPI."));
    return graphics;
}

} // namespace

bool WindowsHardwareProbe::isAvailable()
{
    return true;
}

HardwareInventorySnapshot WindowsHardwareProbe::collect(QStringList *warnings)
{
    HardwareInventorySnapshot snapshot;
    snapshot.scanId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    snapshot.collectedAtUtc = QDateTime::currentDateTimeUtc();
    snapshot.collectorName = QStringLiteral("orbit-windows-passive-probe");
    snapshot.collectorVersion = QStringLiteral("0.2.0");
    snapshot.referenceProfile = QStringLiteral("ORBIT-REF-LAPTOP-01");

    snapshot.platform.insert(QStringLiteral("os"), QStringLiteral("windows"));
    snapshot.platform.insert(QStringLiteral("architecture"), QSysInfo::currentCpuArchitecture());
    snapshot.platform.insert(QStringLiteral("version"), QSysInfo::productVersion());
    snapshot.platform.insert(QStringLiteral("productName"), QSysInfo::prettyProductName());

    QString smbiosError;
    SmbiosFacts smbios;
    const bool smbiosAvailable = readSmbiosFacts(&smbios, &smbiosError);
    const QString smbiosSource = QStringLiteral("Windows:GetSystemFirmwareTable(RSMB)/SMBIOS");

    if (smbiosAvailable) {
        snapshot.machine.insert(QStringLiteral("manufacturer"),
            smbios.manufacturer.isEmpty()
                ? unavailable(EvidenceState::NotReported, smbiosSource,
                              QStringLiteral("SMBIOS Type 1 did not provide a manufacturer string."))
                : known(smbios.manufacturer, smbiosSource + QStringLiteral("/Type 1"),
                        EvidenceState::Described, Confidence::Medium));

        snapshot.machine.insert(QStringLiteral("model"),
            smbios.model.isEmpty()
                ? unavailable(EvidenceState::NotReported, smbiosSource,
                              QStringLiteral("SMBIOS Type 1 did not provide a product name."))
                : known(smbios.model, smbiosSource + QStringLiteral("/Type 1"),
                        EvidenceState::Described, Confidence::Medium));

        snapshot.machine.insert(QStringLiteral("sku"),
            smbios.sku.isEmpty()
                ? unavailable(EvidenceState::NotReported, smbiosSource,
                              QStringLiteral("SMBIOS Type 1 did not provide an SKU."))
                : known(smbios.sku, smbiosSource + QStringLiteral("/Type 1"),
                        EvidenceState::Described, Confidence::Medium));

        snapshot.machine.insert(QStringLiteral("baseboardManufacturer"),
            smbios.boardManufacturer.isEmpty()
                ? unavailable(EvidenceState::NotReported, smbiosSource,
                              QStringLiteral("SMBIOS Type 2 did not provide a baseboard manufacturer."))
                : known(smbios.boardManufacturer, smbiosSource + QStringLiteral("/Type 2"),
                        EvidenceState::Described, Confidence::Medium));

        snapshot.machine.insert(QStringLiteral("baseboardProduct"),
            smbios.boardProduct.isEmpty()
                ? unavailable(EvidenceState::NotReported, smbiosSource,
                              QStringLiteral("SMBIOS Type 2 did not provide a baseboard product."))
                : known(smbios.boardProduct, smbiosSource + QStringLiteral("/Type 2"),
                        EvidenceState::Described, Confidence::Medium));

        QJsonObject firmware;
        firmware.insert(QStringLiteral("biosVendor"),
            smbios.biosVendor.isEmpty()
                ? unavailable(EvidenceState::NotReported, smbiosSource,
                              QStringLiteral("SMBIOS Type 0 did not provide a BIOS vendor."))
                : known(smbios.biosVendor, smbiosSource + QStringLiteral("/Type 0"),
                        EvidenceState::Described, Confidence::Medium));
        firmware.insert(QStringLiteral("biosVersion"),
            smbios.biosVersion.isEmpty()
                ? unavailable(EvidenceState::NotReported, smbiosSource,
                              QStringLiteral("SMBIOS Type 0 did not provide a BIOS version."))
                : known(smbios.biosVersion, smbiosSource + QStringLiteral("/Type 0"),
                        EvidenceState::Described, Confidence::Medium));
        firmware.insert(QStringLiteral("biosReleaseDate"),
            smbios.biosReleaseDate.isEmpty()
                ? unavailable(EvidenceState::NotReported, smbiosSource,
                              QStringLiteral("SMBIOS Type 0 did not provide a BIOS release date."))
                : known(smbios.biosReleaseDate, smbiosSource + QStringLiteral("/Type 0"),
                        EvidenceState::Described, Confidence::Medium));
        firmware.insert(QStringLiteral("smbiosVersion"),
                        known(smbios.version, smbiosSource,
                              EvidenceState::Described, Confidence::High));
        snapshot.machine.insert(QStringLiteral("firmware"), firmware);
    } else {
        snapshot.machine.insert(QStringLiteral("manufacturer"),
            unavailable(EvidenceState::Error, smbiosSource, smbiosError));
        snapshot.machine.insert(QStringLiteral("model"),
            unavailable(EvidenceState::Error, smbiosSource, smbiosError));
        if (warnings)
            warnings->append(QStringLiteral("SMBIOS inventory is unavailable: %1").arg(smbiosError));
    }

    QSettings processorKey(
        QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
        QSettings::NativeFormat);
    const QString cpuModel =
        processorKey.value(QStringLiteral("ProcessorNameString")).toString().trimmed();
    const QString cpuVendor =
        processorKey.value(QStringLiteral("VendorIdentifier")).toString().trimmed();

    QJsonObject processor;
    const QString cpuSource = QStringLiteral("Windows:CentralProcessor registry key");
    processor.insert(QStringLiteral("model"),
        cpuModel.isEmpty()
            ? unavailable(EvidenceState::NotReported, cpuSource,
                          QStringLiteral("ProcessorNameString was not available."))
            : known(cpuModel, cpuSource, EvidenceState::Present, Confidence::High));
    processor.insert(QStringLiteral("vendor"),
        cpuVendor.isEmpty()
            ? unavailable(EvidenceState::NotReported, cpuSource,
                          QStringLiteral("VendorIdentifier was not available."))
            : known(cpuVendor.startsWith(QStringLiteral("GenuineIntel"))
                        ? QStringLiteral("Intel")
                        : (cpuVendor.startsWith(QStringLiteral("AuthenticAMD"))
                               ? QStringLiteral("AMD") : cpuVendor),
                    cpuSource, EvidenceState::Present, Confidence::High));

    CpuTopology topology;
    QString topologyError;
    if (readCpuTopology(&topology, &topologyError)) {
        const QString topologySource =
            QStringLiteral("Windows:GetLogicalProcessorInformationEx(RelationProcessorCore)");
        processor.insert(QStringLiteral("physicalCores"),
                         known(topology.physicalCores, topologySource,
                               EvidenceState::Verified, Confidence::High, QStringLiteral("count")));
        processor.insert(QStringLiteral("logicalProcessors"),
                         known(topology.logicalProcessors, topologySource,
                               EvidenceState::Verified, Confidence::High, QStringLiteral("count")));
        processor.insert(QStringLiteral("activeLogicalProcessorsApi"),
                         known(topology.activeLogicalProcessorsFromWindows,
                               QStringLiteral("Windows:GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)"),
                               EvidenceState::Verified, Confidence::High, QStringLiteral("count")));

        QJsonArray efficiencyClasses;
        for (auto it = topology.efficiencyClassCounts.cbegin();
             it != topology.efficiencyClassCounts.cend(); ++it) {
            efficiencyClasses.append(QJsonObject{
                {QStringLiteral("efficiencyClass"), it.key()},
                {QStringLiteral("coreCount"), it.value()}
            });
        }
        processor.insert(QStringLiteral("efficiencyClasses"), efficiencyClasses);

        if (topology.efficiencyClassCounts.size() >= 2) {
            const int efficiencyClass = topology.efficiencyClassCounts.firstKey();
            const int performanceClass = topology.efficiencyClassCounts.lastKey();
            processor.insert(QStringLiteral("efficiencyCores"),
                             known(topology.efficiencyClassCounts.value(efficiencyClass),
                                   topologySource, EvidenceState::Verified,
                                   Confidence::High, QStringLiteral("count")));
            processor.insert(QStringLiteral("performanceCores"),
                             known(topology.efficiencyClassCounts.value(performanceClass),
                                   topologySource, EvidenceState::Verified,
                                   Confidence::High, QStringLiteral("count")));
        } else {
            processor.insert(QStringLiteral("efficiencyCores"),
                             unavailable(EvidenceState::NotReported, topologySource,
                                         QStringLiteral("Windows did not expose multiple processor efficiency classes.")));
            processor.insert(QStringLiteral("performanceCores"),
                             unavailable(EvidenceState::NotReported, topologySource,
                                         QStringLiteral("Windows did not expose multiple processor efficiency classes.")));
        }
    } else {
        const QString topologySource =
            QStringLiteral("Windows:GetLogicalProcessorInformationEx(RelationProcessorCore)");
        processor.insert(QStringLiteral("physicalCores"),
                         unavailable(EvidenceState::Error, topologySource, topologyError));
        processor.insert(QStringLiteral("logicalProcessors"),
                         unavailable(EvidenceState::Error, topologySource, topologyError));
        processor.insert(QStringLiteral("performanceCores"),
                         unavailable(EvidenceState::Error, topologySource, topologyError));
        processor.insert(QStringLiteral("efficiencyCores"),
                         unavailable(EvidenceState::Error, topologySource, topologyError));
        if (warnings)
            warnings->append(QStringLiteral("CPU topology is unavailable: %1").arg(topologyError));
    }
    snapshot.processors.append(processor);

    const QString memorySourceInstalled =
        QStringLiteral("Windows:GetPhysicallyInstalledSystemMemory/SMBIOS");
    ULONGLONG installedKilobytes = 0;
    if (GetPhysicallyInstalledSystemMemory(&installedKilobytes)) {
        snapshot.memory.insert(QStringLiteral("installedBytes"),
                               known(jsonUnsigned(installedKilobytes * 1024ULL),
                                     memorySourceInstalled, EvidenceState::Described,
                                     Confidence::High, QStringLiteral("bytes")));
    } else {
        snapshot.memory.insert(QStringLiteral("installedBytes"),
                               unavailable(EvidenceState::Error, memorySourceInstalled,
                                           windowsErrorNote(GetLastError())));
    }

    const QString memorySourceUsable =
        QStringLiteral("Windows:GlobalMemoryStatusEx");
    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        snapshot.memory.insert(QStringLiteral("usableBytes"),
                               known(jsonUnsigned(memoryStatus.ullTotalPhys),
                                     memorySourceUsable, EvidenceState::Verified,
                                     Confidence::High, QStringLiteral("bytes")));
        snapshot.memory.insert(QStringLiteral("availableBytes"),
                               known(jsonUnsigned(memoryStatus.ullAvailPhys),
                                     memorySourceUsable, EvidenceState::Verified,
                                     Confidence::High, QStringLiteral("bytes")));
    } else {
        snapshot.memory.insert(QStringLiteral("usableBytes"),
                               unavailable(EvidenceState::Error, memorySourceUsable,
                                           windowsErrorNote(GetLastError())));
        snapshot.memory.insert(QStringLiteral("availableBytes"),
                               unavailable(EvidenceState::Error, memorySourceUsable,
                                           windowsErrorNote(GetLastError())));
    }

    snapshot.graphics = buildGraphicsInventory(warnings);

    snapshot.power.insert(QStringLiteral("battery"),
                          unavailable(EvidenceState::Unsupported,
                                      QStringLiteral("orbit-windows-passive-probe"),
                                      QStringLiteral("Battery enumeration is scheduled for the next H0.2 increment.")));
    snapshot.thermals.insert(QStringLiteral("sensors"), QJsonArray{});
    snapshot.thermals.insert(QStringLiteral("fans"), QJsonArray{});
    snapshot.limitations.append(
        QStringLiteral("H0.2 initial passive scan covers SMBIOS identity, CPU topology, RAM and present display adapters only."));
    snapshot.limitations.append(
        QStringLiteral("GPU runtime memory, driver version, display routing, storage devices, battery, thermal sensors and peripheral details are not yet collected."));
    snapshot.limitations.append(
        QStringLiteral("The reference profile ID marks the requested validation target; this collector does not yet assert that the scanned machine matches that profile."));

    if (warnings) {
        for (const QString &warning : *warnings)
            snapshot.limitations.append(warning);
    }

    return snapshot;
}

} // namespace Orbit::Hardware

#else

namespace Orbit::Hardware {

bool WindowsHardwareProbe::isAvailable()
{
    return false;
}

HardwareInventorySnapshot WindowsHardwareProbe::collect(QStringList *warnings)
{
    Q_UNUSED(warnings);
    return {};
}

} // namespace Orbit::Hardware

#endif
