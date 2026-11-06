#include "HardwareInventorySnapshot.h"

#include <QSet>

namespace Orbit::Hardware {
namespace {

bool isUnavailableState(EvidenceState state)
{
    return state == EvidenceState::NotReported
        || state == EvidenceState::PermissionDenied
        || state == EvidenceState::ProbeSkipped
        || state == EvidenceState::Unsupported
        || state == EvidenceState::Error;
}

void validateEvidenceRecursively(const QJsonValue &value,
                                 const QString &path,
                                 QStringList *errors)
{
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (qsizetype i = 0; i < array.size(); ++i) {
            validateEvidenceRecursively(array.at(i),
                                        path + QLatin1Char('[') + QString::number(i) + QLatin1Char(']'),
                                        errors);
        }
        return;
    }

    if (!value.isObject())
        return;

    const QJsonObject object = value.toObject();
    const bool looksLikeEvidence =
        object.contains(QStringLiteral("source"))
        || object.contains(QStringLiteral("observedAtUtc"))
        || object.contains(QStringLiteral("confidence"))
        || (object.contains(QStringLiteral("state")) && object.contains(QStringLiteral("value")));

    if (looksLikeEvidence) {
        QString error;
        EvidenceValue evidence;
        if (!EvidenceValue::fromJson(object, &evidence, &error))
            errors->append(path + QStringLiteral(": ") + error);
    }

    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.key() == QStringLiteral("value") && looksLikeEvidence)
            continue;
        validateEvidenceRecursively(it.value(),
                                    path.isEmpty() ? it.key() : path + QLatin1Char('.') + it.key(),
                                    errors);
    }
}

bool parseRequiredString(const QJsonObject &object,
                         const QString &key,
                         QString *target,
                         QStringList *errors)
{
    const QJsonValue value = object.value(key);
    if (!value.isString() || value.toString().trimmed().isEmpty()) {
        errors->append(QStringLiteral("root.") + key
                       + QStringLiteral(": must be a non-empty string"));
        return false;
    }
    *target = value.toString();
    return true;
}

} // namespace

QString toString(EvidenceState state)
{
    switch (state) {
    case EvidenceState::Present: return QStringLiteral("present");
    case EvidenceState::Described: return QStringLiteral("described");
    case EvidenceState::DriverBound: return QStringLiteral("driver-bound");
    case EvidenceState::RuntimeAccessible: return QStringLiteral("runtime-accessible");
    case EvidenceState::Verified: return QStringLiteral("verified");
    case EvidenceState::NotReported: return QStringLiteral("not-reported");
    case EvidenceState::PermissionDenied: return QStringLiteral("permission-denied");
    case EvidenceState::ProbeSkipped: return QStringLiteral("probe-skipped");
    case EvidenceState::Unsupported: return QStringLiteral("unsupported");
    case EvidenceState::Error: return QStringLiteral("error");
    }
    return QString();
}

QString toString(Confidence confidence)
{
    switch (confidence) {
    case Confidence::High: return QStringLiteral("high");
    case Confidence::Medium: return QStringLiteral("medium");
    case Confidence::Low: return QStringLiteral("low");
    }
    return QString();
}

bool evidenceStateFromString(const QString &value, EvidenceState *state)
{
    if (!state)
        return false;

    if (value == QStringLiteral("present")) *state = EvidenceState::Present;
    else if (value == QStringLiteral("described")) *state = EvidenceState::Described;
    else if (value == QStringLiteral("driver-bound")) *state = EvidenceState::DriverBound;
    else if (value == QStringLiteral("runtime-accessible")) *state = EvidenceState::RuntimeAccessible;
    else if (value == QStringLiteral("verified")) *state = EvidenceState::Verified;
    else if (value == QStringLiteral("not-reported")) *state = EvidenceState::NotReported;
    else if (value == QStringLiteral("permission-denied")) *state = EvidenceState::PermissionDenied;
    else if (value == QStringLiteral("probe-skipped")) *state = EvidenceState::ProbeSkipped;
    else if (value == QStringLiteral("unsupported")) *state = EvidenceState::Unsupported;
    else if (value == QStringLiteral("error")) *state = EvidenceState::Error;
    else return false;
    return true;
}

bool confidenceFromString(const QString &value, Confidence *confidence)
{
    if (!confidence)
        return false;

    if (value == QStringLiteral("high")) *confidence = Confidence::High;
    else if (value == QStringLiteral("medium")) *confidence = Confidence::Medium;
    else if (value == QStringLiteral("low")) *confidence = Confidence::Low;
    else return false;
    return true;
}

EvidenceValue EvidenceValue::known(const QJsonValue &value,
                                   const QString &source,
                                   EvidenceState state,
                                   Confidence confidence,
                                   const QString &unit,
                                   const QString &note)
{
    EvidenceValue result;
    result.value = value;
    result.source = source;
    result.state = state;
    result.confidence = confidence;
    result.unit = unit;
    result.note = note;
    result.observedAtUtc = QDateTime::currentDateTimeUtc();
    return result;
}

EvidenceValue EvidenceValue::unavailable(EvidenceState state,
                                         const QString &source,
                                         const QString &note)
{
    EvidenceValue result;
    result.value = QJsonValue(QJsonValue::Null);
    result.source = source;
    result.state = state;
    result.confidence = Confidence::Low;
    result.note = note;
    result.observedAtUtc = QDateTime::currentDateTimeUtc();
    return result;
}

QJsonObject EvidenceValue::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("value"), value);
    if (!unit.isEmpty())
        object.insert(QStringLiteral("unit"), unit);
    object.insert(QStringLiteral("source"), source);
    object.insert(QStringLiteral("observedAtUtc"),
                  observedAtUtc.toUTC().toString(Qt::ISODateWithMs));
    object.insert(QStringLiteral("state"), toString(state));
    object.insert(QStringLiteral("confidence"), toString(confidence));
    if (!note.isEmpty())
        object.insert(QStringLiteral("note"), note);
    return object;
}

bool EvidenceValue::isValid(QString *error) const
{
    const auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    if (source.trimmed().isEmpty())
        return fail(QStringLiteral("evidence source is required"));
    if (!observedAtUtc.isValid())
        return fail(QStringLiteral("observedAtUtc must be a valid timestamp"));
    if (toString(state).isEmpty())
        return fail(QStringLiteral("evidence state is invalid"));
    if (toString(confidence).isEmpty())
        return fail(QStringLiteral("confidence is invalid"));
    if (isUnavailableState(state) && !value.isNull())
        return fail(QStringLiteral("unavailable evidence states must have a null value"));
    if (!isUnavailableState(state) && value.isNull())
        return fail(QStringLiteral("available evidence states must have a non-null value"));

    if (error)
        error->clear();
    return true;
}

bool EvidenceValue::fromJson(const QJsonObject &object,
                             EvidenceValue *result,
                             QString *error)
{
    const auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    if (!result)
        return fail(QStringLiteral("result pointer is null"));
    if (!object.contains(QStringLiteral("value")))
        return fail(QStringLiteral("missing 'value'"));

    const QJsonValue sourceValue = object.value(QStringLiteral("source"));
    if (!sourceValue.isString() || sourceValue.toString().trimmed().isEmpty())
        return fail(QStringLiteral("'source' must be a non-empty string"));

    const QJsonValue timestampValue = object.value(QStringLiteral("observedAtUtc"));
    if (!timestampValue.isString())
        return fail(QStringLiteral("'observedAtUtc' must be a string"));
    QDateTime timestamp = QDateTime::fromString(timestampValue.toString(), Qt::ISODateWithMs);
    if (!timestamp.isValid())
        timestamp = QDateTime::fromString(timestampValue.toString(), Qt::ISODate);
    if (!timestamp.isValid())
        return fail(QStringLiteral("'observedAtUtc' must be an ISO-8601 timestamp"));

    EvidenceState parsedState;
    if (!evidenceStateFromString(object.value(QStringLiteral("state")).toString(), &parsedState))
        return fail(QStringLiteral("'state' is not a recognized evidence state"));

    Confidence parsedConfidence;
    if (!confidenceFromString(object.value(QStringLiteral("confidence")).toString(),
                              &parsedConfidence))
        return fail(QStringLiteral("'confidence' must be high, medium or low"));

    EvidenceValue parsed;
    parsed.value = object.value(QStringLiteral("value"));
    parsed.source = sourceValue.toString();
    parsed.observedAtUtc = timestamp.toUTC();
    parsed.state = parsedState;
    parsed.confidence = parsedConfidence;
    parsed.unit = object.value(QStringLiteral("unit")).toString();
    parsed.note = object.value(QStringLiteral("note")).toString();

    QString validationError;
    if (!parsed.isValid(&validationError))
        return fail(validationError);

    *result = parsed;
    if (error)
        error->clear();
    return true;
}

QJsonObject HardwareInventorySnapshot::toJson() const
{
    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), schemaVersion);
    root.insert(QStringLiteral("scanId"), scanId);
    root.insert(QStringLiteral("collectedAtUtc"),
                collectedAtUtc.toUTC().toString(Qt::ISODateWithMs));
    root.insert(QStringLiteral("collectorName"), collectorName);
    root.insert(QStringLiteral("collectorVersion"), collectorVersion);
    root.insert(QStringLiteral("platform"), platform);
    if (!referenceProfile.isEmpty())
        root.insert(QStringLiteral("referenceProfile"), referenceProfile);

    root.insert(QStringLiteral("machine"), machine);
    root.insert(QStringLiteral("processors"), processors);
    root.insert(QStringLiteral("graphics"), graphics);
    root.insert(QStringLiteral("displays"), displays);
    root.insert(QStringLiteral("memory"), memory);
    root.insert(QStringLiteral("storage"), storage);
    root.insert(QStringLiteral("power"), power);
    root.insert(QStringLiteral("thermals"), thermals);
    root.insert(QStringLiteral("connectivity"), connectivity);
    root.insert(QStringLiteral("security"), security);
    root.insert(QStringLiteral("limitations"), limitations);
    return root;
}

QStringList HardwareInventorySnapshot::validate() const
{
    QStringList errors;
    if (schemaVersion != CurrentSchemaVersion)
        errors.append(QStringLiteral("schemaVersion: unsupported schema version"));
    if (scanId.trimmed().isEmpty())
        errors.append(QStringLiteral("scanId: must not be empty"));
    if (!collectedAtUtc.isValid())
        errors.append(QStringLiteral("collectedAtUtc: must be a valid timestamp"));
    if (collectorName.trimmed().isEmpty())
        errors.append(QStringLiteral("collectorName: must not be empty"));
    if (collectorVersion.trimmed().isEmpty())
        errors.append(QStringLiteral("collectorVersion: must not be empty"));
    if (platform.value(QStringLiteral("os")).toString().trimmed().isEmpty())
        errors.append(QStringLiteral("platform.os: must be a non-empty string"));
    if (platform.value(QStringLiteral("architecture")).toString().trimmed().isEmpty())
        errors.append(QStringLiteral("platform.architecture: must be a non-empty string"));

    validateEvidenceRecursively(toJson(), QString(), &errors);
    return errors;
}

bool HardwareInventorySnapshot::fromJson(const QJsonObject &object,
                                          HardwareInventorySnapshot *snapshot,
                                          QStringList *errors)
{
    QStringList localErrors;
    if (!snapshot) {
        localErrors.append(QStringLiteral("snapshot pointer is null"));
        if (errors) *errors = localErrors;
        return false;
    }

    const QJsonValue version = object.value(QStringLiteral("schemaVersion"));
    if (!version.isDouble()
        || version.toDouble() != static_cast<double>(version.toInt(-1))) {
        localErrors.append(QStringLiteral("schemaVersion: must be an integer"));
    }

    HardwareInventorySnapshot parsed;
    if (version.isDouble()
        && version.toDouble() == static_cast<double>(version.toInt(-1))) {
        parsed.schemaVersion = version.toInt(-1);
    }

    parseRequiredString(object, QStringLiteral("scanId"), &parsed.scanId, &localErrors);
    parseRequiredString(object, QStringLiteral("collectorName"), &parsed.collectorName, &localErrors);
    parseRequiredString(object, QStringLiteral("collectorVersion"), &parsed.collectorVersion, &localErrors);

    const QJsonValue collectedValue = object.value(QStringLiteral("collectedAtUtc"));
    if (collectedValue.isString()) {
        parsed.collectedAtUtc = QDateTime::fromString(collectedValue.toString(), Qt::ISODateWithMs);
        if (!parsed.collectedAtUtc.isValid())
            parsed.collectedAtUtc = QDateTime::fromString(collectedValue.toString(), Qt::ISODate);
    }
    if (!parsed.collectedAtUtc.isValid())
        localErrors.append(QStringLiteral("collectedAtUtc: must be an ISO-8601 timestamp"));

    parsed.platform = object.value(QStringLiteral("platform")).toObject();
    parsed.referenceProfile = object.value(QStringLiteral("referenceProfile")).toString();
    parsed.machine = object.value(QStringLiteral("machine")).toObject();
    parsed.processors = object.value(QStringLiteral("processors")).toArray();
    parsed.graphics = object.value(QStringLiteral("graphics")).toArray();
    // Optional for schema-v1 snapshots created before H0.4 introduced displays.
    parsed.displays = object.value(QStringLiteral("displays")).toArray();
    parsed.memory = object.value(QStringLiteral("memory")).toObject();
    parsed.storage = object.value(QStringLiteral("storage")).toArray();
    parsed.power = object.value(QStringLiteral("power")).toObject();
    parsed.thermals = object.value(QStringLiteral("thermals")).toObject();
    parsed.connectivity = object.value(QStringLiteral("connectivity")).toArray();
    parsed.security = object.value(QStringLiteral("security")).toObject();
    parsed.limitations = object.value(QStringLiteral("limitations")).toArray();

    const auto requireType = [&object, &localErrors](const QString &key,
                                                     QJsonValue::Type expectedType) {
        if (object.value(key).type() != expectedType)
            localErrors.append(QStringLiteral("root.") + key + QStringLiteral(": wrong JSON type"));
    };
    requireType(QStringLiteral("platform"), QJsonValue::Object);
    requireType(QStringLiteral("machine"), QJsonValue::Object);
    requireType(QStringLiteral("processors"), QJsonValue::Array);
    requireType(QStringLiteral("graphics"), QJsonValue::Array);
    requireType(QStringLiteral("memory"), QJsonValue::Object);
    requireType(QStringLiteral("storage"), QJsonValue::Array);
    requireType(QStringLiteral("power"), QJsonValue::Object);
    requireType(QStringLiteral("thermals"), QJsonValue::Object);
    requireType(QStringLiteral("connectivity"), QJsonValue::Array);
    requireType(QStringLiteral("security"), QJsonValue::Object);
    requireType(QStringLiteral("limitations"), QJsonValue::Array);

    localErrors.append(parsed.validate());
    if (!localErrors.isEmpty()) {
        if (errors) *errors = localErrors;
        return false;
    }

    *snapshot = parsed;
    if (errors) errors->clear();
    return true;
}

} // namespace Orbit::Hardware
