#include "WindowsExtendedHardwareProbe.h"

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
#include <ntdddisk.h>
#include <ntddstor.h>

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QStringList>

#include <algorithm>
#include <cwchar>
#include <vector>

namespace Orbit::Hardware {
namespace {

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

QJsonValue jsonUnsigned(quint64 value)
{
    return QJsonValue(static_cast<double>(value));
}

QString windowsErrorNote(DWORD error)
{
    return QStringLiteral("Windows API error %1").arg(error);
}

QStringList readMultiStringProperty(HDEVINFO set,
                                    SP_DEVINFO_DATA *info,
                                    DWORD property)
{
    DWORD type = 0;
    DWORD required = 0;
    SetupDiGetDeviceRegistryPropertyW(set, info, property, &type,
                                      nullptr, 0, &required);
    if (required == 0 || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return {};

    std::vector<BYTE> buffer(static_cast<size_t>(required) + sizeof(wchar_t), 0);
    if (!SetupDiGetDeviceRegistryPropertyW(set, info, property, &type,
                                           buffer.data(),
                                           static_cast<DWORD>(buffer.size()),
                                           nullptr)) {
        return {};
    }

    QStringList values;
    const wchar_t *cursor = reinterpret_cast<const wchar_t *>(buffer.data());
    if (type == REG_MULTI_SZ) {
        while (*cursor != L'\0') {
            const size_t length = std::wcslen(cursor);
            if (length == 0)
                break;
            values.append(QString::fromWCharArray(cursor, static_cast<qsizetype>(length)));
            cursor += length + 1;
        }
    } else if (type == REG_SZ) {
        const size_t length = std::wcslen(cursor);
        if (length > 0)
            values.append(QString::fromWCharArray(cursor, static_cast<qsizetype>(length)));
    }
    return values;
}

QString readStringProperty(HDEVINFO set, SP_DEVINFO_DATA *info, DWORD property)
{
    const QStringList values = readMultiStringProperty(set, info, property);
    return values.isEmpty() ? QString() : values.first();
}

QJsonObject driverEvidence(const QString &driverKey)
{
    const QString source = QStringLiteral(
        "Windows:HKLM/SYSTEM/CurrentControlSet/Control/Class/") + driverKey;
    if (driverKey.trimmed().isEmpty()) {
        return {
            {QStringLiteral("driverVersion"),
             unavailable(EvidenceState::NotReported, source,
                         QStringLiteral("No installed PnP driver key was reported."))},
            {QStringLiteral("driverDate"),
             unavailable(EvidenceState::NotReported, source,
                         QStringLiteral("No installed PnP driver key was reported."))},
            {QStringLiteral("driverProvider"),
             unavailable(EvidenceState::NotReported, source,
                         QStringLiteral("No installed PnP driver key was reported."))}
        };
    }

    const QString registryPath =
        QStringLiteral("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\")
        + driverKey;
    QSettings settings(registryPath, QSettings::NativeFormat);

    const QString version = settings.value(QStringLiteral("DriverVersion")).toString().trimmed();
    const QString date = settings.value(QStringLiteral("DriverDate")).toString().trimmed();
    const QString provider = settings.value(QStringLiteral("ProviderName")).toString().trimmed();

    return {
        {QStringLiteral("driverVersion"),
         version.isEmpty()
             ? unavailable(EvidenceState::NotReported, source,
                           QStringLiteral("The driver registry key did not expose DriverVersion."))
             : known(version, source + QStringLiteral("/DriverVersion"),
                     EvidenceState::Described, Confidence::Medium)},
        {QStringLiteral("driverDate"),
         date.isEmpty()
             ? unavailable(EvidenceState::NotReported, source,
                           QStringLiteral("The driver registry key did not expose DriverDate."))
             : known(date, source + QStringLiteral("/DriverDate"),
                     EvidenceState::Described, Confidence::Medium)},
        {QStringLiteral("driverProvider"),
         provider.isEmpty()
             ? unavailable(EvidenceState::NotReported, source,
                           QStringLiteral("The driver registry key did not expose ProviderName."))
             : known(provider, source + QStringLiteral("/ProviderName"),
                     EvidenceState::Described, Confidence::Medium)}
    };
}

QString descriptorString(const std::vector<BYTE> &buffer, DWORD offset)
{
    if (offset == 0 || offset >= buffer.size())
        return {};

    const char *begin = reinterpret_cast<const char *>(buffer.data() + offset);
    const size_t remaining = buffer.size() - offset;
    size_t length = 0;
    while (length < remaining && begin[length] != '\0')
        ++length;
    if (length == 0 || length == remaining)
        return {};
    return QString::fromLatin1(begin, static_cast<qsizetype>(length)).trimmed();
}

QString storageBusName(STORAGE_BUS_TYPE type)
{
    switch (type) {
    case BusTypeScsi: return QStringLiteral("SCSI");
    case BusTypeAtapi: return QStringLiteral("ATAPI");
    case BusTypeAta: return QStringLiteral("ATA");
    case BusType1394: return QStringLiteral("IEEE 1394");
    case BusTypeSsa: return QStringLiteral("SSA");
    case BusTypeFibre: return QStringLiteral("Fibre Channel");
    case BusTypeUsb: return QStringLiteral("USB");
    case BusTypeRAID: return QStringLiteral("RAID");
    case BusTypeiScsi: return QStringLiteral("iSCSI");
    case BusTypeSas: return QStringLiteral("SAS");
    case BusTypeSata: return QStringLiteral("SATA");
    case BusTypeSd: return QStringLiteral("SD");
    case BusTypeMmc: return QStringLiteral("MMC");
    case BusTypeVirtual: return QStringLiteral("Virtual");
    case BusTypeFileBackedVirtual: return QStringLiteral("File-backed virtual");
    // Later Windows SDKs append bus types after BusTypeFileBackedVirtual.
    // Numeric cases preserve compatibility with older MinGW-w64 SDK headers.
    case 16: return QStringLiteral("Storage Spaces");
    case 17: return QStringLiteral("NVMe");
    case 18: return QStringLiteral("Storage Class Memory");
    case 19: return QStringLiteral("UFS");
    case 20: return QStringLiteral("NVMe over Fabrics");
    default: return QStringLiteral("Unknown");
    }
}

QJsonArray enumerateStorage(QStringList *warnings)
{
    QJsonArray storage;
    const QString source = QStringLiteral("Windows:SetupAPI/GUID_DEVINTERFACE_DISK");
    HDEVINFO set = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK, nullptr, nullptr,
                                        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE) {
        if (warnings) {
            warnings->append(QStringLiteral("Disk interface enumeration failed: %1")
                                 .arg(windowsErrorNote(GetLastError())));
        }
        return storage;
    }

    for (DWORD index = 0;; ++index) {
        SP_DEVICE_INTERFACE_DATA interfaceData{};
        interfaceData.cbSize = sizeof(interfaceData);
        if (!SetupDiEnumDeviceInterfaces(set, nullptr, &GUID_DEVINTERFACE_DISK,
                                         index, &interfaceData)) {
            const DWORD error = GetLastError();
            if (error != ERROR_NO_MORE_ITEMS && warnings) {
                warnings->append(QStringLiteral("Disk interface enumeration stopped: %1")
                                     .arg(windowsErrorNote(error)));
            }
            break;
        }

        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(set, &interfaceData, nullptr, 0,
                                         &required, nullptr);
        if (required < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) {
            continue;
        }

        std::vector<BYTE> detailBuffer(required);
        auto *detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(
            detailBuffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        SP_DEVINFO_DATA deviceInfo{};
        deviceInfo.cbSize = sizeof(deviceInfo);
        if (!SetupDiGetDeviceInterfaceDetailW(set, &interfaceData, detail, required,
                                              nullptr, &deviceInfo)) {
            continue;
        }

        QJsonObject disk;
        const QString friendlyName = readStringProperty(set, &deviceInfo, SPDRP_FRIENDLYNAME);
        const QString deviceDescription = readStringProperty(set, &deviceInfo, SPDRP_DEVICEDESC);
        const QString manufacturer = readStringProperty(set, &deviceInfo, SPDRP_MFG);
        const QString driverKey = readStringProperty(set, &deviceInfo, SPDRP_DRIVER);
        const QStringList hardwareIds = readMultiStringProperty(set, &deviceInfo, SPDRP_HARDWAREID);

        const QString displayName = !friendlyName.isEmpty()
            ? friendlyName : deviceDescription;
        disk.insert(QStringLiteral("model"),
                    displayName.isEmpty()
                        ? unavailable(EvidenceState::NotReported, source,
                                      QStringLiteral("Windows did not report a disk model or friendly name."))
                        : known(displayName, source, EvidenceState::Present));
        disk.insert(QStringLiteral("pnpManufacturer"),
                    manufacturer.isEmpty()
                        ? unavailable(EvidenceState::NotReported, source,
                                      QStringLiteral("Windows did not report the PnP manufacturer string."))
                        : known(manufacturer, source, EvidenceState::Described, Confidence::Medium));
        disk.insert(QStringLiteral("hardwareId"),
                    hardwareIds.isEmpty()
                        ? unavailable(EvidenceState::NotReported, source,
                                      QStringLiteral("Windows did not report a disk hardware ID."))
                        : known(hardwareIds.first(), source, EvidenceState::Present, Confidence::High));
        disk.insert(QStringLiteral("driver"),
                    driverKey.isEmpty()
                        ? unavailable(EvidenceState::NotReported, source,
                                      QStringLiteral("No associated PnP driver key was reported."))
                        : known(driverKey, source, EvidenceState::DriverBound,
                                Confidence::Medium,
                                QString(),
                                QStringLiteral("PnP driver association; this does not prove runtime operation.")));
        const QJsonObject driver = driverEvidence(driverKey);
        for (auto it = driver.constBegin(); it != driver.constEnd(); ++it)
            disk.insert(it.key(), it.value());

        HANDLE handle = CreateFileW(detail->DevicePath, GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) {
            const DWORD error = GetLastError();
            const EvidenceState state = error == ERROR_ACCESS_DENIED
                ? EvidenceState::PermissionDenied : EvidenceState::Error;
            disk.insert(QStringLiteral("capacityBytes"),
                        unavailable(state, source, windowsErrorNote(error)));
            disk.insert(QStringLiteral("busType"),
                        unavailable(state, source, windowsErrorNote(error)));
            disk.insert(QStringLiteral("vendor"),
                        unavailable(state, source, windowsErrorNote(error)));
            disk.insert(QStringLiteral("revision"),
                        unavailable(state, source, windowsErrorNote(error)));
            disk.insert(QStringLiteral("deviceNumber"),
                        unavailable(state, source, windowsErrorNote(error)));
            storage.append(disk);
            continue;
        }

        STORAGE_DEVICE_NUMBER deviceNumber{};
        DWORD returned = 0;
        if (DeviceIoControl(handle, IOCTL_STORAGE_GET_DEVICE_NUMBER,
                            nullptr, 0, &deviceNumber, sizeof(deviceNumber),
                            &returned, nullptr)) {
            disk.insert(QStringLiteral("deviceNumber"),
                        known(static_cast<int>(deviceNumber.DeviceNumber),
                              QStringLiteral("Windows:IOCTL_STORAGE_GET_DEVICE_NUMBER"),
                              EvidenceState::Verified, Confidence::High, QStringLiteral("index")));
        } else {
            const DWORD error = GetLastError();
            disk.insert(QStringLiteral("deviceNumber"),
                        unavailable(error == ERROR_ACCESS_DENIED
                                        ? EvidenceState::PermissionDenied : EvidenceState::Error,
                                    QStringLiteral("Windows:IOCTL_STORAGE_GET_DEVICE_NUMBER"),
                                    windowsErrorNote(error)));
        }

        GET_LENGTH_INFORMATION lengthInfo{};
        returned = 0;
        if (DeviceIoControl(handle, IOCTL_DISK_GET_LENGTH_INFO,
                            nullptr, 0, &lengthInfo, sizeof(lengthInfo),
                            &returned, nullptr) && lengthInfo.Length.QuadPart > 0) {
            disk.insert(QStringLiteral("capacityBytes"),
                        known(jsonUnsigned(static_cast<quint64>(lengthInfo.Length.QuadPart)),
                              QStringLiteral("Windows:IOCTL_DISK_GET_LENGTH_INFO"),
                              EvidenceState::Verified, Confidence::High, QStringLiteral("bytes")));
        } else {
            const DWORD error = GetLastError();
            disk.insert(QStringLiteral("capacityBytes"),
                        unavailable(error == ERROR_ACCESS_DENIED
                                        ? EvidenceState::PermissionDenied : EvidenceState::NotReported,
                                    QStringLiteral("Windows:IOCTL_DISK_GET_LENGTH_INFO"),
                                    windowsErrorNote(error)));
        }

        STORAGE_PROPERTY_QUERY query{};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;
        STORAGE_DESCRIPTOR_HEADER descriptorHeader{};
        returned = 0;
        if (DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
                            &query, sizeof(query),
                            &descriptorHeader, sizeof(descriptorHeader),
                            &returned, nullptr)
            && descriptorHeader.Size >= sizeof(STORAGE_DEVICE_DESCRIPTOR)
            && descriptorHeader.Size <= 1024 * 1024) {
            std::vector<BYTE> descriptorBuffer(descriptorHeader.Size);
            returned = 0;
            if (DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
                                &query, sizeof(query),
                                descriptorBuffer.data(),
                                static_cast<DWORD>(descriptorBuffer.size()),
                                &returned, nullptr)
                && returned >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
                const auto *descriptor =
                    reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR *>(descriptorBuffer.data());
                const QString vendor = descriptorString(descriptorBuffer, descriptor->VendorIdOffset);
                const QString product = descriptorString(descriptorBuffer, descriptor->ProductIdOffset);
                const QString revision = descriptorString(descriptorBuffer, descriptor->ProductRevisionOffset);
                const QString busName = storageBusName(descriptor->BusType);

                disk.insert(QStringLiteral("model"),
                            product.isEmpty()
                                ? disk.value(QStringLiteral("model"))
                                : known(product, QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                        EvidenceState::Described, Confidence::High));
                disk.insert(QStringLiteral("vendor"),
                            vendor.isEmpty()
                                ? unavailable(EvidenceState::NotReported,
                                              QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                              QStringLiteral("The device descriptor did not expose a vendor string."))
                                : known(vendor, QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                        EvidenceState::Described, Confidence::High));
                disk.insert(QStringLiteral("revision"),
                            revision.isEmpty()
                                ? unavailable(EvidenceState::NotReported,
                                              QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                              QStringLiteral("The device descriptor did not expose a product revision."))
                                : known(revision, QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                        EvidenceState::Described, Confidence::Medium));
                disk.insert(QStringLiteral("busType"),
                            known(busName, QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                  EvidenceState::Verified, Confidence::High));
                disk.insert(QStringLiteral("busTypeCode"),
                            known(static_cast<int>(descriptor->BusType),
                                  QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                  EvidenceState::Verified, Confidence::High));
            } else {
                const DWORD error = GetLastError();
                disk.insert(QStringLiteral("vendor"),
                            unavailable(EvidenceState::NotReported,
                                        QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                        windowsErrorNote(error)));
                disk.insert(QStringLiteral("revision"),
                            unavailable(EvidenceState::NotReported,
                                        QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                        windowsErrorNote(error)));
                disk.insert(QStringLiteral("busType"),
                            unavailable(EvidenceState::NotReported,
                                        QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                        windowsErrorNote(error)));
            }
        } else {
            const DWORD error = GetLastError();
            const EvidenceState state = error == ERROR_ACCESS_DENIED
                ? EvidenceState::PermissionDenied : EvidenceState::NotReported;
            disk.insert(QStringLiteral("vendor"),
                        unavailable(state, QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                    windowsErrorNote(error)));
            disk.insert(QStringLiteral("revision"),
                        unavailable(state, QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                    windowsErrorNote(error)));
            disk.insert(QStringLiteral("busType"),
                        unavailable(state, QStringLiteral("Windows:IOCTL_STORAGE_QUERY_PROPERTY"),
                                    windowsErrorNote(error)));
        }

        CloseHandle(handle);
        storage.append(disk);
    }

    SetupDiDestroyDeviceInfoList(set);
    return storage;
}

QJsonObject collectBattery()
{
    const QString source = QStringLiteral("Windows:GetSystemPowerStatus");
    SYSTEM_POWER_STATUS status{};
    if (!GetSystemPowerStatus(&status)) {
        const QString error = windowsErrorNote(GetLastError());
        return {
            {QStringLiteral("present"),
             unavailable(EvidenceState::Error, source, error)},
            {QStringLiteral("acLineStatus"),
             unavailable(EvidenceState::Error, source, error)},
            {QStringLiteral("batteryPercent"),
             unavailable(EvidenceState::Error, source, error)},
            {QStringLiteral("charging"),
             unavailable(EvidenceState::Error, source, error)},
            {QStringLiteral("batteryLifeSeconds"),
             unavailable(EvidenceState::Error, source, error)},
            {QStringLiteral("batteryFullLifeSeconds"),
             unavailable(EvidenceState::Error, source, error)}
        };
    }

    QJsonObject battery;
    if (status.BatteryFlag == 255) {
        battery.insert(QStringLiteral("present"),
                       unavailable(EvidenceState::NotReported, source,
                                   QStringLiteral("Windows reported unknown battery status.")));
    } else {
        battery.insert(QStringLiteral("present"),
                       known(status.BatteryFlag != 128, source,
                             EvidenceState::Verified, Confidence::High));
    }

    QString acLabel = QStringLiteral("unknown");
    if (status.ACLineStatus == 0) acLabel = QStringLiteral("offline");
    else if (status.ACLineStatus == 1) acLabel = QStringLiteral("online");
    battery.insert(QStringLiteral("acLineStatus"),
                   status.ACLineStatus == 255
                       ? unavailable(EvidenceState::NotReported, source,
                                     QStringLiteral("Windows did not report AC line status."))
                       : known(acLabel, source, EvidenceState::Verified, Confidence::High));

    if (status.BatteryFlag == 128) {
        const QString note = QStringLiteral("Windows reports that no system battery is present.");
        battery.insert(QStringLiteral("batteryPercent"),
                       unavailable(EvidenceState::Unsupported, source, note));
        battery.insert(QStringLiteral("charging"),
                       unavailable(EvidenceState::Unsupported, source, note));
        battery.insert(QStringLiteral("batteryLifeSeconds"),
                       unavailable(EvidenceState::Unsupported, source, note));
        battery.insert(QStringLiteral("batteryFullLifeSeconds"),
                       unavailable(EvidenceState::Unsupported, source, note));
    } else {
        battery.insert(QStringLiteral("batteryPercent"),
                       status.BatteryLifePercent == 255
                           ? unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("Windows did not report battery percentage."))
                           : known(static_cast<int>(status.BatteryLifePercent), source,
                                   EvidenceState::Verified, Confidence::High,
                                   QStringLiteral("percent")));
        battery.insert(QStringLiteral("charging"),
                       status.BatteryFlag == 255
                           ? unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("Windows did not report charging state."))
                           : known((status.BatteryFlag & 8) != 0, source,
                                   EvidenceState::Verified, Confidence::High));
        battery.insert(QStringLiteral("batteryLifeSeconds"),
                       status.BatteryLifeTime == 0xFFFFFFFF
                           ? unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("Remaining battery time is unknown or AC power is connected."))
                           : known(static_cast<quint64>(status.BatteryLifeTime), source,
                                   EvidenceState::Described, Confidence::Medium,
                                   QStringLiteral("seconds")));
        battery.insert(QStringLiteral("batteryFullLifeSeconds"),
                       status.BatteryFullLifeTime == 0xFFFFFFFF
                           ? unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("Estimated full-charge runtime is unavailable."))
                           : known(static_cast<quint64>(status.BatteryFullLifeTime), source,
                                   EvidenceState::Described, Confidence::Low,
                                   QStringLiteral("seconds")));
    }

    battery.insert(QStringLiteral("batterySaver"),
                   known(status.SystemStatusFlag == 1, source,
                         EvidenceState::Verified, Confidence::High));
    battery.insert(QStringLiteral("designCapacityWh"),
                   unavailable(EvidenceState::NotReported, source,
                               QStringLiteral("GetSystemPowerStatus does not expose battery design capacity.")));
    battery.insert(QStringLiteral("fullChargeCapacityWh"),
                   unavailable(EvidenceState::NotReported, source,
                               QStringLiteral("GetSystemPowerStatus does not expose full-charge capacity.")));
    return battery;
}

QJsonArray enumerateDisplays(QStringList *warnings)
{
    QJsonArray displays;
    const QString source = QStringLiteral("Windows:EnumDisplayDevices/EnumDisplaySettingsEx");

    for (DWORD index = 0;; ++index) {
        DISPLAY_DEVICEW adapter{};
        adapter.cb = sizeof(adapter);
        if (!EnumDisplayDevicesW(nullptr, index, &adapter, 0))
            break;

        if ((adapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0
            || (adapter.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0) {
            continue;
        }

        DISPLAY_DEVICEW monitor{};
        monitor.cb = sizeof(monitor);
        const bool hasMonitor = EnumDisplayDevicesW(adapter.DeviceName, 0, &monitor, 0) != FALSE;

        QJsonObject display;
        display.insert(QStringLiteral("adapterName"),
                       known(QString::fromWCharArray(adapter.DeviceString),
                             source, EvidenceState::Present, Confidence::High));
        display.insert(QStringLiteral("deviceName"),
                       known(QString::fromWCharArray(adapter.DeviceName),
                             source, EvidenceState::Present, Confidence::High));
        display.insert(QStringLiteral("monitorName"),
                       hasMonitor && monitor.DeviceString[0] != L'\0'
                           ? known(QString::fromWCharArray(monitor.DeviceString),
                                   source, EvidenceState::Described, Confidence::Medium)
                           : unavailable(EvidenceState::NotReported, source,
                                         QStringLiteral("Windows did not provide a friendly monitor name.")));
        display.insert(QStringLiteral("attachedToDesktop"),
                       known(true, source, EvidenceState::Verified, Confidence::High));

        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        if (EnumDisplaySettingsExW(adapter.DeviceName, ENUM_CURRENT_SETTINGS, &mode, 0)) {
            display.insert(QStringLiteral("widthPixels"),
                           known(static_cast<int>(mode.dmPelsWidth), source,
                                 EvidenceState::Verified, Confidence::High, QStringLiteral("pixels")));
            display.insert(QStringLiteral("heightPixels"),
                           known(static_cast<int>(mode.dmPelsHeight), source,
                                 EvidenceState::Verified, Confidence::High, QStringLiteral("pixels")));
            display.insert(QStringLiteral("bitsPerPixel"),
                           known(static_cast<int>(mode.dmBitsPerPel), source,
                                 EvidenceState::Verified, Confidence::High, QStringLiteral("bits")));
            display.insert(QStringLiteral("refreshRateHz"),
                           mode.dmDisplayFrequency > 1
                               ? known(static_cast<int>(mode.dmDisplayFrequency), source,
                                       EvidenceState::Verified, Confidence::Medium, QStringLiteral("Hz"),
                                       QStringLiteral("Current mode reported by Windows; this is not the panel's maximum refresh capability."))
                               : unavailable(EvidenceState::NotReported, source,
                                             QStringLiteral("Windows did not report a usable current refresh rate.")));
        } else {
            const DWORD error = GetLastError();
            display.insert(QStringLiteral("widthPixels"),
                           unavailable(EvidenceState::Error, source, windowsErrorNote(error)));
            display.insert(QStringLiteral("heightPixels"),
                           unavailable(EvidenceState::Error, source, windowsErrorNote(error)));
            display.insert(QStringLiteral("bitsPerPixel"),
                           unavailable(EvidenceState::Error, source, windowsErrorNote(error)));
            display.insert(QStringLiteral("refreshRateHz"),
                           unavailable(EvidenceState::Error, source, windowsErrorNote(error)));
        }
        displays.append(display);
    }

    if (displays.isEmpty() && warnings) {
        warnings->append(QStringLiteral("Windows did not report an attached desktop display through EnumDisplayDevices."));
    }
    return displays;
}

QJsonArray enumeratePnpClass(const GUID &classGuid,
                             const QString &category,
                             const QString &source,
                             QStringList *warnings)
{
    QJsonArray devices;
    HDEVINFO set = SetupDiGetClassDevsW(&classGuid, nullptr, nullptr, DIGCF_PRESENT);
    if (set == INVALID_HANDLE_VALUE) {
        if (warnings) {
            warnings->append(QStringLiteral("%1 device enumeration failed: %2")
                                 .arg(category, windowsErrorNote(GetLastError())));
        }
        return devices;
    }

    for (DWORD index = 0;; ++index) {
        SP_DEVINFO_DATA info{};
        info.cbSize = sizeof(info);
        if (!SetupDiEnumDeviceInfo(set, index, &info)) {
            const DWORD error = GetLastError();
            if (error != ERROR_NO_MORE_ITEMS && warnings) {
                warnings->append(QStringLiteral("%1 device enumeration stopped: %2")
                                     .arg(category, windowsErrorNote(error)));
            }
            break;
        }

        const QString friendlyName = readStringProperty(set, &info, SPDRP_FRIENDLYNAME);
        const QString description = readStringProperty(set, &info, SPDRP_DEVICEDESC);
        const QString manufacturer = readStringProperty(set, &info, SPDRP_MFG);
        const QString className = readStringProperty(set, &info, SPDRP_CLASS);
        const QString driverKey = readStringProperty(set, &info, SPDRP_DRIVER);
        const QStringList hardwareIds = readMultiStringProperty(set, &info, SPDRP_HARDWAREID);
        const QString model = !friendlyName.isEmpty() ? friendlyName : description;

        QJsonObject device;
        device.insert(QStringLiteral("category"), category);
        device.insert(QStringLiteral("model"),
                      model.isEmpty()
                          ? unavailable(EvidenceState::NotReported, source,
                                        QStringLiteral("Windows did not report a friendly name or description."))
                          : known(model, source, EvidenceState::Present, Confidence::High));
        device.insert(QStringLiteral("manufacturer"),
                      manufacturer.isEmpty()
                          ? unavailable(EvidenceState::NotReported, source,
                                        QStringLiteral("Windows did not report the manufacturer string."))
                          : known(manufacturer, source, EvidenceState::Described, Confidence::Medium));
        device.insert(QStringLiteral("deviceClass"),
                      className.isEmpty()
                          ? unavailable(EvidenceState::NotReported, source,
                                        QStringLiteral("Windows did not report the device setup class name."))
                          : known(className, source, EvidenceState::Described, Confidence::High));
        device.insert(QStringLiteral("hardwareId"),
                      hardwareIds.isEmpty()
                          ? unavailable(EvidenceState::NotReported, source,
                                        QStringLiteral("Windows did not report a PnP hardware ID."))
                          : known(hardwareIds.first(), source, EvidenceState::Present, Confidence::High));
        device.insert(QStringLiteral("driver"),
                      driverKey.isEmpty()
                          ? unavailable(EvidenceState::NotReported, source,
                                        QStringLiteral("No associated PnP driver key was reported."))
                          : known(driverKey, source, EvidenceState::DriverBound,
                                  Confidence::Medium,
                                  QString(),
                                  QStringLiteral("PnP driver association; this does not prove runtime operation.")));
        const QJsonObject driver = driverEvidence(driverKey);
        for (auto it = driver.constBegin(); it != driver.constEnd(); ++it)
            device.insert(it.key(), it.value());

        devices.append(device);
    }

    SetupDiDestroyDeviceInfoList(set);
    return devices;
}

void enrichGraphicsDriverVersions(QJsonArray *graphics)
{
    if (!graphics)
        return;

    for (qsizetype i = 0; i < graphics->size(); ++i) {
        QJsonObject adapter = graphics->at(i).toObject();
        const QString driverKey = adapter.value(QStringLiteral("driver"))
                                      .toObject()
                                      .value(QStringLiteral("value")).toString();
        const QJsonObject driver = driverEvidence(driverKey);
        for (auto it = driver.constBegin(); it != driver.constEnd(); ++it)
            adapter.insert(it.key(), it.value());
        (*graphics)[i] = adapter;
    }
}

} // namespace

void WindowsExtendedHardwareProbe::collect(HardwareInventorySnapshot *snapshot,
                                            QStringList *warnings)
{
    if (!snapshot)
        return;

    snapshot->storage = enumerateStorage(warnings);
    snapshot->power.insert(QStringLiteral("battery"), collectBattery());
    snapshot->displays = enumerateDisplays(warnings);

    QJsonArray connectivity;
    const QString sourceNet = QStringLiteral("Windows:SetupAPI/GUID_DEVCLASS_NET/DIGCF_PRESENT");
    const QString sourceBluetooth = QStringLiteral("Windows:SetupAPI/Bluetooth/DIGCF_PRESENT");
    const QString sourceMedia = QStringLiteral("Windows:SetupAPI/GUID_DEVCLASS_MEDIA/DIGCF_PRESENT");

    const QJsonArray network = enumeratePnpClass(GUID_DEVCLASS_NET,
                                                  QStringLiteral("network-adapter"),
                                                  sourceNet, warnings);
    const GUID bluetoothClass = {
        0xe0cbf06c, 0xcd8b, 0x4647, {0xbb, 0x8a, 0x26, 0x3b, 0x43, 0xf0, 0xf9, 0x74}
    };
    const QJsonArray bluetooth = enumeratePnpClass(bluetoothClass,
                                                    QStringLiteral("bluetooth"),
                                                    sourceBluetooth, warnings);
    const QJsonArray media = enumeratePnpClass(GUID_DEVCLASS_MEDIA,
                                                QStringLiteral("media-controller"),
                                                sourceMedia, warnings);

    for (const QJsonValue &device : network) connectivity.append(device);
    for (const QJsonValue &device : bluetooth) connectivity.append(device);
    for (const QJsonValue &device : media) connectivity.append(device);
    snapshot->connectivity = connectivity;

    enrichGraphicsDriverVersions(&snapshot->graphics);

    snapshot->limitations.append(
        QStringLiteral("H0.4 collects present disk interfaces, current battery/AC status, attached display current modes, and present network/Bluetooth/media-controller devices."));
    snapshot->limitations.append(
        QStringLiteral("Disk serial numbers are deliberately not read or exported. Battery design/full-charge capacity, panel maximum refresh capabilities, user-mode audio endpoints, configured NVIDIA TGP, GPU runtime memory, thermal/fan telemetry, TPM and Secure Boot are not yet collected."));
}

} // namespace Orbit::Hardware

#else

namespace Orbit::Hardware {

void WindowsExtendedHardwareProbe::collect(HardwareInventorySnapshot *snapshot,
                                            QStringList *warnings)
{
    Q_UNUSED(snapshot);
    Q_UNUSED(warnings);
}

} // namespace Orbit::Hardware

#endif
