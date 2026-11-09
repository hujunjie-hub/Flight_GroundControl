#include "FlightTelemetryFactGroup.h"

#include "Vehicle.h"

FlightTelemetryFactGroup::FlightTelemetryFactGroup(QObject *parent)
    : FactGroup(0, parent)
{
    for (Fact *fact : {
        &_opTimeBootUs, &_opBootId, &_opProfileId, &_authorityEpoch, &_directiveEpoch,
        &_permitEpoch, &_requestId, &_requestSessionId, &_operationReasonCode,
        &_requestSourceId, &_armState, &_flightMode, &_authority, &_failsafeLevel, &_missionState,
        &_navTimeBootUs, &_navBootId, &_navProfileId, &_navStateEpoch, &_originId,
        &_navigationCapabilities, &_positionStdM, &_velocityStdMps, &_headingStdRad,
        &_navigationAgeUs, &_navigationReasonCode,
        &_prearmTimeBootUs, &_prearmBootId, &_prearmEvidenceTimeUs, &_prearmProfileId,
        &_prearmCheckEpoch, &_prearmModeMask, &_prearmReasonCode, &_prearmCheckId,
        &_prearmReasonNamespace, &_prearmRecoveryCondition, &_prearmCheckState
    }) {
        _addFact(fact);
    }
}

void FlightTelemetryFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    if (!vehicle || message.sysid != vehicle->id() || message.compid != vehicle->compId()) {
        return;
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_FCU_OPERATION_STATUS: {
        mavlink_fcu_operation_status_t value{};
        mavlink_msg_fcu_operation_status_decode(&message, &value);
        _opTimeBootUs.setRawValue(value.time_boot_us);
        _opBootId.setRawValue(value.boot_id);
        _opProfileId.setRawValue(value.profile_id);
        _authorityEpoch.setRawValue(value.authority_epoch);
        _directiveEpoch.setRawValue(value.directive_epoch);
        _permitEpoch.setRawValue(value.permit_epoch);
        _requestId.setRawValue(value.request_id);
        _requestSessionId.setRawValue(value.request_session_id);
        _operationReasonCode.setRawValue(value.reason_code);
        _requestSourceId.setRawValue(value.request_source_id);
        _armState.setRawValue(value.arm_state);
        _flightMode.setRawValue(value.flight_mode);
        _authority.setRawValue(value.authority);
        _failsafeLevel.setRawValue(value.failsafe_level);
        _missionState.setRawValue(value.mission_state);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_NAV_STATUS: {
        mavlink_fcu_nav_status_t value{};
        mavlink_msg_fcu_nav_status_decode(&message, &value);
        _navTimeBootUs.setRawValue(value.time_boot_us);
        _navBootId.setRawValue(value.boot_id);
        _navProfileId.setRawValue(value.profile_id);
        _navStateEpoch.setRawValue(value.state_epoch);
        _originId.setRawValue(value.origin_id);
        _navigationCapabilities.setRawValue(value.capability_mask);
        _positionStdM.setRawValue(value.position_std_m);
        _velocityStdMps.setRawValue(value.velocity_std_mps);
        _headingStdRad.setRawValue(value.heading_std_rad);
        _navigationAgeUs.setRawValue(value.age_us);
        _navigationReasonCode.setRawValue(value.reason_code);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_PREARM_STATUS: {
        mavlink_fcu_prearm_status_t value{};
        mavlink_msg_fcu_prearm_status_decode(&message, &value);
        _prearmTimeBootUs.setRawValue(value.time_boot_us);
        _prearmBootId.setRawValue(value.boot_id);
        _prearmEvidenceTimeUs.setRawValue(value.evidence_time_us);
        _prearmProfileId.setRawValue(value.profile_id);
        _prearmCheckEpoch.setRawValue(value.check_epoch);
        _prearmModeMask.setRawValue(value.mode_mask);
        _prearmReasonCode.setRawValue(value.reason_code);
        _prearmCheckId.setRawValue(value.check_id);
        _prearmReasonNamespace.setRawValue(value.reason_namespace);
        _prearmRecoveryCondition.setRawValue(value.recovery_condition);
        _prearmCheckState.setRawValue(value.check_state);
        _setTelemetryAvailable(true);
        break;
    }
    default:
        break;
    }
}
