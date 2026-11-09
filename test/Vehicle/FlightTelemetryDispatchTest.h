#pragma once

#include "BaseClasses/CommsTest.h"

class FlightTelemetryDispatchTest : public CommsTest
{
    Q_OBJECT

private slots:
    void _validFrameTraversesMavlinkParserAndVehicleDispatch();
    void _invalidFramesAndSourcesDoNotUpdateTelemetry();
    void _telemetryIsIsolatedAcrossVehicles();
    void _telemetryStartsFreshAfterReconnect();
};
