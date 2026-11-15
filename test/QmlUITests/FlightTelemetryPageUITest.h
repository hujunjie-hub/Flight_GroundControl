#pragma once

#include "VehicleConfigUITestBase.h"

class FlightTelemetryPageUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

private slots:
    void _testUnknownInvalidAndStaleStatesAreRendered();
    void _testFourReferencePagesAndResponsiveLayouts();
};
