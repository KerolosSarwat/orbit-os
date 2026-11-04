#pragma once

#include "HardwareInventorySnapshot.h"

#include <QStringList>

namespace Orbit::Hardware {

class WindowsHardwareProbe final
{
public:
    static bool isAvailable();
    static HardwareInventorySnapshot collect(QStringList *warnings = nullptr);
};

} // namespace Orbit::Hardware
