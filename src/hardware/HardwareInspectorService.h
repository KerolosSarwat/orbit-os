#pragma once

#include <QObject>
#include <QByteArray>
#include <QVariantMap>

#include "HardwareInventorySnapshot.h"

class QFutureWatcher;

namespace Orbit::Hardware {

class HardwareInspectorService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool hasInventory READ hasInventory NOTIFY inventoryChanged)
    Q_PROPERTY(QVariantMap inventory READ inventory NOTIFY inventoryChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString lastScanAt READ lastScanAt NOTIFY inventoryChanged)

public:
    explicit HardwareInspectorService(QObject *parent = nullptr);

    bool available() const;
    bool scanning() const;
    bool hasInventory() const;
    QVariantMap inventory() const;
    QString statusMessage() const;
    QString lastScanAt() const;

    Q_INVOKABLE void scan();
    Q_INVOKABLE QString exportReport();

signals:
    void scanningChanged();
    void inventoryChanged();
    void statusMessageChanged();

private:
    bool m_available = false;
    bool m_scanning = false;
    bool m_hasInventory = false;
    QVariantMap m_inventory;
    QString m_statusMessage;
    QString m_lastScanAt;
    QByteArray m_latestJson;
    HardwareInventorySnapshot m_latestSnapshot;
};

} // namespace Orbit::Hardware
