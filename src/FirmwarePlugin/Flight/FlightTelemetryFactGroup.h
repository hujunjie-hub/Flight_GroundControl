#pragma once

#include "FactGroup.h"
#include "Fact.h"
#include "FactMetaData.h"

/// Per-vehicle Flight FCU telemetry cache.
///
/// Values are informational only. They never grant command authority and are not
/// used by QGC to infer that a safety prerequisite has passed.
class FlightTelemetryFactGroup final : public FactGroup
{
    Q_OBJECT
public:
    explicit FlightTelemetryFactGroup(QObject *parent = nullptr);
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) override;

private:
    Fact _opTimeBootUs{0, QStringLiteral("opTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _opBootId{0, QStringLiteral("opBootId"), FactMetaData::valueTypeUint64, this};
    Fact _opProfileId{0, QStringLiteral("opProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _authorityEpoch{0, QStringLiteral("authorityEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _directiveEpoch{0, QStringLiteral("directiveEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _permitEpoch{0, QStringLiteral("permitEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _requestId{0, QStringLiteral("requestId"), FactMetaData::valueTypeUint32, this};
    Fact _requestSessionId{0, QStringLiteral("requestSessionId"), FactMetaData::valueTypeUint32, this};
    Fact _operationReasonCode{0, QStringLiteral("operationReasonCode"), FactMetaData::valueTypeUint32, this};
    Fact _requestSourceId{0, QStringLiteral("requestSourceId"), FactMetaData::valueTypeUint16, this};
    Fact _armState{0, QStringLiteral("armState"), FactMetaData::valueTypeUint8, this};
    Fact _flightMode{0, QStringLiteral("flightMode"), FactMetaData::valueTypeUint8, this};
    Fact _authority{0, QStringLiteral("authority"), FactMetaData::valueTypeUint8, this};
    Fact _failsafeLevel{0, QStringLiteral("failsafeLevel"), FactMetaData::valueTypeUint8, this};
    Fact _missionState{0, QStringLiteral("missionState"), FactMetaData::valueTypeUint8, this};

    Fact _navTimeBootUs{0, QStringLiteral("navTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _navBootId{0, QStringLiteral("navBootId"), FactMetaData::valueTypeUint64, this};
    Fact _navProfileId{0, QStringLiteral("navProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _navStateEpoch{0, QStringLiteral("navStateEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _originId{0, QStringLiteral("originId"), FactMetaData::valueTypeUint32, this};
    Fact _navigationCapabilities{0, QStringLiteral("navigationCapabilities"), FactMetaData::valueTypeUint32, this};
    Fact _positionStdM{0, QStringLiteral("positionStdM"), FactMetaData::valueTypeFloat, this};
    Fact _velocityStdMps{0, QStringLiteral("velocityStdMps"), FactMetaData::valueTypeFloat, this};
    Fact _headingStdRad{0, QStringLiteral("headingStdRad"), FactMetaData::valueTypeFloat, this};
    Fact _navigationAgeUs{0, QStringLiteral("navigationAgeUs"), FactMetaData::valueTypeUint32, this};
    Fact _navigationReasonCode{0, QStringLiteral("navigationReasonCode"), FactMetaData::valueTypeUint32, this};

    Fact _prearmTimeBootUs{0, QStringLiteral("prearmTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _prearmBootId{0, QStringLiteral("prearmBootId"), FactMetaData::valueTypeUint64, this};
    Fact _prearmEvidenceTimeUs{0, QStringLiteral("prearmEvidenceTimeUs"), FactMetaData::valueTypeUint64, this};
    Fact _prearmProfileId{0, QStringLiteral("prearmProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _prearmCheckEpoch{0, QStringLiteral("prearmCheckEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _prearmModeMask{0, QStringLiteral("prearmModeMask"), FactMetaData::valueTypeUint32, this};
    Fact _prearmReasonCode{0, QStringLiteral("prearmReasonCode"), FactMetaData::valueTypeUint32, this};
    Fact _prearmCheckId{0, QStringLiteral("prearmCheckId"), FactMetaData::valueTypeUint16, this};
    Fact _prearmReasonNamespace{0, QStringLiteral("prearmReasonNamespace"), FactMetaData::valueTypeUint16, this};
    Fact _prearmRecoveryCondition{0, QStringLiteral("prearmRecoveryCondition"), FactMetaData::valueTypeUint16, this};
    Fact _prearmCheckState{0, QStringLiteral("prearmCheckState"), FactMetaData::valueTypeUint8, this};
};
