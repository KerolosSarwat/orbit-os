#include "InventorySerializer.h"

#include <QJsonParseError>

namespace Orbit::Hardware {

QJsonDocument InventorySerializer::toDocument(const HardwareInventorySnapshot &snapshot)
{
    return QJsonDocument(snapshot.toJson());
}

QByteArray InventorySerializer::toJson(const HardwareInventorySnapshot &snapshot,
                                       QJsonDocument::JsonFormat format)
{
    return toDocument(snapshot).toJson(format);
}

bool InventorySerializer::fromJson(const QByteArray &json,
                                   HardwareInventorySnapshot *snapshot,
                                   QStringList *errors)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errors) {
            errors->clear();
            errors->append(QStringLiteral("JSON parse error at byte %1: %2")
                               .arg(parseError.offset)
                               .arg(parseError.errorString()));
        }
        return false;
    }
    return fromDocument(document, snapshot, errors);
}

bool InventorySerializer::fromDocument(const QJsonDocument &document,
                                       HardwareInventorySnapshot *snapshot,
                                       QStringList *errors)
{
    if (!document.isObject()) {
        if (errors) {
            errors->clear();
            errors->append(QStringLiteral("inventory root must be a JSON object"));
        }
        return false;
    }
    return HardwareInventorySnapshot::fromJson(document.object(), snapshot, errors);
}

} // namespace Orbit::Hardware
