#pragma once

#include "HardwareInventorySnapshot.h"

#include <QStringList>

namespace Orbit::Hardware {

class WindowsExtendedHardwareProbe final
{
public:
    static void collect(HardwareInventorySnapshot *snapshot,
                        QStringList *warnings = nullptr);
};

} // namespace Orbit::Hardware
