#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>

namespace Orbit::Hardware {

enum class EvidenceState {
    Present,
    Described,
    DriverBound,
    RuntimeAccessible,
    Verified,
    NotReported,
    PermissionDenied,
    ProbeSkipped,
    Unsupported,
    Error
};

enum class Confidence {
    High,
    Medium,
    Low
};

QString toString(EvidenceState state);
QString toString(Confidence confidence);
bool evidenceStateFromString(const QString &value, EvidenceState *state);
bool confidenceFromString(const QString &value, Confidence *confidence);

struct EvidenceValue
{
    QJsonValue value = QJsonValue(QJsonValue::Null);
    QString unit;
    QString source;
    QDateTime observedAtUtc;
    EvidenceState state = EvidenceState::NotReported;
    Confidence confidence = Confidence::Low;
    QString note;

    static EvidenceValue known(
        const QJsonValue &value,
        const QString &source,
        EvidenceState state = EvidenceState::Described,
        Confidence confidence = Confidence::High,
        const QString &unit = QString(),
        const QString &note = QString());

    static EvidenceValue unavailable(
        EvidenceState state,
        const QString &source,
        const QString &note = QString());

    QJsonObject toJson() const;
    bool isValid(QString *error = nullptr) const;
    static bool fromJson(const QJsonObject &object,
                         EvidenceValue *result,
                         QString *error = nullptr);
};

struct HardwareInventorySnapshot
{
    static constexpr int CurrentSchemaVersion = 1;

    int schemaVersion = CurrentSchemaVersion;
    QString scanId;
    QDateTime collectedAtUtc;
    QString collectorName;
    QString collectorVersion;
    QJsonObject platform;
    QString referenceProfile;

    QJsonObject machine;
    QJsonArray processors;
    QJsonArray graphics;
    QJsonArray displays;
    QJsonObject memory;
    QJsonArray storage;
    QJsonObject power;
    QJsonObject thermals;
    QJsonArray connectivity;
    QJsonObject security;
    QJsonArray limitations;

    QJsonObject toJson() const;
    QStringList validate() const;
    static bool fromJson(const QJsonObject &object,
                         HardwareInventorySnapshot *snapshot,
                         QStringList *errors = nullptr);
};

} // namespace Orbit::Hardware
