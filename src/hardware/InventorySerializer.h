#pragma once

#include "HardwareInventorySnapshot.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QStringList>

namespace Orbit::Hardware {

class InventorySerializer final
{
public:
    static QJsonDocument toDocument(const HardwareInventorySnapshot &snapshot);
    static QByteArray toJson(const HardwareInventorySnapshot &snapshot,
                             QJsonDocument::JsonFormat format = QJsonDocument::Indented);
    static bool fromJson(const QByteArray &json,
                         HardwareInventorySnapshot *snapshot,
                         QStringList *errors = nullptr);
    static bool fromDocument(const QJsonDocument &document,
                             HardwareInventorySnapshot *snapshot,
                             QStringList *errors = nullptr);
};

} // namespace Orbit::Hardware
