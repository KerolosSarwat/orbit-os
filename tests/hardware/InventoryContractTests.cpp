#include "../../src/hardware/HardwareInventorySnapshot.h"
#include "../../src/hardware/InventorySerializer.h"
#include "../../src/hardware/HardwareInspectorService.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

using namespace Orbit::Hardware;

namespace {

int failures = 0;

void check(bool condition, const QString &message)
{
    QTextStream stream(stderr);
    if (!condition) {
        stream << "FAIL: " << message << Qt::endl;
        ++failures;
    } else {
        stream << "PASS: " << message << Qt::endl;
    }
}

QByteArray readFixture(const QString &directory, const QString &fileName)
{
    QFile file(directory + QLatin1Char('/') + fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        check(false, QStringLiteral("open fixture %1").arg(fileName));
        return {};
    }
    return file.readAll();
}

void testReferenceFixture(const QString &fixtureDirectory)
{
    HardwareInventorySnapshot snapshot;
    QStringList errors;
    const QByteArray bytes = readFixture(fixtureDirectory,
                                         QStringLiteral("reference-laptop-partial.json"));
    const bool parsed = InventorySerializer::fromJson(bytes, &snapshot, &errors);
    check(parsed, QStringLiteral("reference laptop fixture parses"));
    if (!parsed) {
        for (const QString &error : errors)
            check(false, error);
        return;
    }

    check(snapshot.schemaVersion == HardwareInventorySnapshot::CurrentSchemaVersion,
          QStringLiteral("reference fixture uses the current schema"));
    check(snapshot.processors.size() == 1, QStringLiteral("reference fixture has one CPU package"));
    check(snapshot.graphics.size() == 2, QStringLiteral("reference fixture keeps both GPUs separate"));
    check(snapshot.graphics.at(1).toObject().value(QStringLiteral("driver"))
              .toObject().value(QStringLiteral("state")).toString() == QStringLiteral("not-reported"),
          QStringLiteral("unknown NVIDIA driver state is preserved"));
    check(snapshot.validate().isEmpty(), QStringLiteral("reference fixture passes validation"));

    HardwareInventorySnapshot roundTrip;
    const QByteArray serialized = InventorySerializer::toJson(snapshot, QJsonDocument::Compact);
    check(InventorySerializer::fromJson(serialized, &roundTrip, &errors),
          QStringLiteral("serialized snapshot round-trips"));
    check(roundTrip.graphics.size() == snapshot.graphics.size(),
          QStringLiteral("round-trip preserves graphics adapters"));
}

void testUnknownDeviceFixture(const QString &fixtureDirectory)
{
    HardwareInventorySnapshot snapshot;
    QStringList errors;
    const QByteArray bytes = readFixture(fixtureDirectory,
                                         QStringLiteral("unknown-device-partial.json"));
    const bool parsed = InventorySerializer::fromJson(bytes, &snapshot, &errors);
    check(parsed, QStringLiteral("unknown device fixture parses"));
    if (!parsed) {
        for (const QString &error : errors)
            check(false, error);
        return;
    }

    check(snapshot.machine.value(QStringLiteral("model")).toObject()
              .value(QStringLiteral("state")).toString() == QStringLiteral("not-reported"),
          QStringLiteral("missing machine model remains explicitly not-reported"));
    check(snapshot.graphics.isEmpty(), QStringLiteral("no GPU is not silently invented"));
    check(snapshot.validate().isEmpty(), QStringLiteral("unknown device fixture passes validation"));
}

void testEvidenceRules()
{
    const EvidenceValue unavailable = EvidenceValue::unavailable(
        EvidenceState::NotReported,
        QStringLiteral("test-provider"),
        QStringLiteral("No source exposed this property."));
    QString error;
    check(unavailable.isValid(&error), QStringLiteral("null not-reported evidence is valid"));

    EvidenceValue parsed;
    check(EvidenceValue::fromJson(unavailable.toJson(), &parsed, &error),
          QStringLiteral("evidence serializes and parses"));

    EvidenceValue invalid = EvidenceValue::known(
        QJsonValue(QJsonValue::Null),
        QStringLiteral("test-provider"),
        EvidenceState::Present);
    check(!invalid.isValid(&error),
          QStringLiteral("present evidence cannot contain a null value"));

    invalid = EvidenceValue::known(QJsonValue(10),
                                   QString(),
                                   EvidenceState::Present);
    check(!invalid.isValid(&error), QStringLiteral("evidence source is mandatory"));
}


void testHardwareInspectorService()
{
    HardwareInspectorService service;
    check(!service.scanning(), QStringLiteral("hardware inspector starts idle"));
    check(!service.hasInventory(), QStringLiteral("hardware inspector requires a new scan before showing inventory"));
#ifdef Q_OS_WIN
    check(service.available(), QStringLiteral("Windows build exposes the native hardware collector"));
#else
    check(!service.available(), QStringLiteral("non-Windows build does not claim a missing hardware collector"));
    check(service.statusMessage().contains(QStringLiteral("Windows only")),
          QStringLiteral("non-Windows inspector explains collector availability"));
#endif
}

void testRejectsMalformedSnapshot(const QString &fixtureDirectory)
{
    QJsonParseError parseError;
    const QJsonDocument fixture = QJsonDocument::fromJson(
        readFixture(fixtureDirectory, QStringLiteral("reference-laptop-partial.json")),
        &parseError);
    QJsonObject root = fixture.object();
    QJsonObject machine = root.value(QStringLiteral("machine")).toObject();
    machine.insert(QStringLiteral("manufacturer"), QJsonObject{
        {QStringLiteral("value"), QStringLiteral("Example OEM")},
        {QStringLiteral("source"), QString()},
        {QStringLiteral("observedAtUtc"), QStringLiteral("2026-11-04T10:00:00.000Z")},
        {QStringLiteral("state"), QStringLiteral("present")},
        {QStringLiteral("confidence"), QStringLiteral("high")}
    });
    root.insert(QStringLiteral("machine"), machine);

    HardwareInventorySnapshot snapshot;
    QStringList errors;
    check(!HardwareInventorySnapshot::fromJson(root, &snapshot, &errors),
          QStringLiteral("snapshot with missing evidence source is rejected"));
    check(!errors.isEmpty(), QStringLiteral("malformed snapshot returns diagnostics"));

    machine = root.value(QStringLiteral("machine")).toObject();
    QJsonObject manufacturer = machine.value(QStringLiteral("manufacturer")).toObject();
    manufacturer.remove(QStringLiteral("source"));
    manufacturer.insert(QStringLiteral("source"), QStringLiteral("fixture"));
    manufacturer.remove(QStringLiteral("state"));
    machine.insert(QStringLiteral("manufacturer"), manufacturer);
    root.insert(QStringLiteral("machine"), machine);
    errors.clear();
    check(!HardwareInventorySnapshot::fromJson(root, &snapshot, &errors),
          QStringLiteral("evidence object with missing state is rejected"));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QString fixtureDirectory = argc > 1
        ? QString::fromLocal8Bit(argv[1])
        : QStringLiteral("tests/hardware/fixtures");

    testReferenceFixture(fixtureDirectory);
    testUnknownDeviceFixture(fixtureDirectory);
    testEvidenceRules();
    testHardwareInspectorService();
    testRejectsMalformedSnapshot(fixtureDirectory);

    QTextStream stream(stdout);
    stream << (failures == 0
                   ? QStringLiteral("All hardware inventory contract tests passed.")
                   : QStringLiteral("%1 hardware inventory contract test(s) failed.").arg(failures))
           << Qt::endl;
    return failures == 0 ? 0 : 1;
}
