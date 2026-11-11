#pragma once

#include "BaseClasses/CommsTest.h"

class FlightTelemetryDispatchTest : public CommsTest
{
    Q_OBJECT

private slots:
    void initTestCase();
    void _validFrameTraversesMavlinkParserAndVehicleDispatch();
    void _allFcuFamiliesTraverseVehicleDispatch();
    void _invalidFramesAndSourcesDoNotUpdateTelemetry();
    void _telemetryIsIsolatedAcrossVehicles();
    void _telemetryStartsFreshAfterReconnect();
    void _freshFrameStillUpdates();
    void _equalTimestampIsIdempotent();
    void _staleOperationFrameDoesNotOverwrite();
    void _staleNavigationFrameDoesNotOverwrite();
    void _newBootIdResetsWatermark();
    void _delayedOldBootFrameIsRejected();
    void _unknownBootIdDoesNotResetWatermark();
    void _familyIsFreshAfterAcceptedFrame();
    void _familyBecomesStaleWithoutFrames();
    void _familyRecoversToFreshAfterNewFrame();
    void _staleTimestampDoesNotRefreshFreshness();
    void _equalTimestampDoesNotRefreshFreshness();
    void _familyStalenessIsIndependentPerFamily();
    void _disconnectStopsRefreshingFamilies();
    void _reconnectResumesFreshnessOnNewEpoch();
    void _mockLinkDropsFramesDeterministically();
    void _mockLinkDelaysFramesWithoutLosingThem();
    void _staleNotifyFiresWhenDeadlinePasses();
    void _staleNotifyIsEmittedOnlyOnce();
    void _freshFrameNotifiesImmediately();
    void _thresholdChangeReschedulesDeadline();
    void _multipleFamiliesGoStaleInOneNotification();
    void _staleNotificationDoesNotDisturbFreshFamily();
    void _disconnectedVehicleStillNotifiesStale();
    void _qmlBindingObservesStaleTransition();
    void _destroyedGroupStopsStaleTimer();
};
