#pragma once

#include "UnitTest.h"

class FlightFirmwarePluginTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _missionClearIsNotAdvertised();
    void _coordinateHomeActionIsNotAdvertised();
    void _takeoffIsNotAFlightMode();
    void _flightOperationTelemetryIsDecodedPerVehicle();
};
