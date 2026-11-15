#pragma once

#include "VehicleConfigUITestBase.h"

class FlightStorageGimbalPageUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

private slots:
    void _testStoragePageUsesFreshValidityGatedTelemetry();
    void _testGimbalPageShowsSafeEmptyState();
    void _testGimbalOwnershipDragAndReleaseFlow();
};
