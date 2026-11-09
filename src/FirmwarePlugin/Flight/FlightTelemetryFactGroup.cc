#include <cmath>

#include <QtCore/QByteArray>
#include <QtCore/QVariantMap>

#include "FlightTelemetryFactGroup.h"
#include "Vehicle.h"

FlightTelemetryFactGroup::FlightTelemetryFactGroup(QObject *parent)
    : FactGroup(0, parent)
{
    for (Fact *fact : {
        &_opTimeBootUs,
        &_opBootId,
        &_opProfileId,
        &_authorityEpoch,
        &_directiveEpoch,
        &_permitEpoch,
        &_requestId,
        &_requestSessionId,
        &_operationReasonCode,
        &_requestSourceId,
        &_armState,
        &_flightMode,
        &_authority,
        &_failsafeLevel,
        &_missionState,
        &_navTimeBootUs,
        &_navBootId,
        &_navProfileId,
        &_navStateEpoch,
        &_originId,
        &_navigationCapabilities,
        &_positionStdM,
        &_velocityStdMps,
        &_headingStdRad,
        &_navigationAgeUs,
        &_navigationReasonCode,
        &_prearmTimeBootUs,
        &_prearmBootId,
        &_prearmEvidenceTimeUs,
        &_prearmProfileId,
        &_prearmCheckEpoch,
        &_prearmModeMask,
        &_prearmReasonCode,
        &_prearmCheckId,
        &_prearmReasonNamespace,
        &_prearmRecoveryCondition,
        &_prearmCheckState,
        &_operationAvailable,
        &_navigationAvailable,
        &_sensorHealthAvailable,
        &_controlAvailable,
        &_actuatorAvailable,
        &_linkAvailable,
        &_storageAvailable,
        &_resourceAvailable,
        &_timeAvailable,
        &_eventAvailable,
        &_prearmAvailable,
        &_rcInputAvailable,
        &_buildInfoAvailable,
        &_sensorTimeBootUs,
        &_sensorBootId,
        &_sensorProfileId,
        &_sensorInstanceId,
        &_sensorPolicyEpoch,
        &_sensorReasonCode,
        &_sensorGapCount,
        &_sensorRejectCount,
        &_sensorHealthState,
        &_sensorAction,
        &_controlTimeBootUs,
        &_controlBootId,
        &_controlProfileId,
        &_controlSequence,
        &_controlReferenceId,
        &_controlReferenceEpoch,
        &_controllableDofMask,
        &_degradedDofMask,
        &_controlSolveTimeUs,
        &_controlReasonCode,
        &_solverStatus,
        &_actuatorTimeBootUs,
        &_actuatorBootId,
        &_actuatorProfileId,
        &_actuatorControlSequence,
        &_actuatorPermitEpoch,
        &_actuatorAvailableMask,
        &_actuatorSaturatedMask,
        &_actuatorReasonCode,
        &_actuatorOutputPolicyId,
        &_actuatorFeedbackCapability,
        &_linkTimeBootUs,
        &_linkBootId,
        &_linkProfileId,
        &_linkInstanceId,
        &_linkValidityMask,
        &_linkPacketLossPpm,
        &_linkAgeUs,
        &_linkReasonCode,
        &_linkKind,
        &_linkState,
        &_linkRssiDbm,
        &_linkQualityPermille,
        &_storageTimeBootUs,
        &_storageBootId,
        &_storageProfileId,
        &_storageValidityMask,
        &_storageSettingsGeneration,
        &_storageCriticalEventDrops,
        &_storageLogDrops,
        &_storageFreeKib,
        &_storageReasonCode,
        &_sdState,
        &_norState,
        &_resourceTimeBootUs,
        &_resourceBootId,
        &_resourceProfileId,
        &_resourceValidityMask,
        &_resourceTaskId,
        &_resourceWcetUs,
        &_resourceStackFreeBytes,
        &_resourceDeadlineMissCount,
        &_resourceAxiFreeBytes,
        &_resourceCpuLoadPermille,
        &_timeStatusTimeBootUs,
        &_timeStatusBootId,
        &_timeStatusUnixTimeUs,
        &_timeStatusProfileId,
        &_timeEpoch,
        &_sourceMappingEpoch,
        &_eventUncertaintyUs,
        &_utcUncertaintyUs,
        &_timeQuality,
        &_timeDisciplineState,
        &_utcValid,
        &_eventTimeBootUs,
        &_eventBootId,
        &_eventProfileId,
        &_eventId,
        &_eventReasonCode,
        &_eventAuthorityEpoch,
        &_eventDirectiveEpoch,
        &_eventPermitEpoch,
        &_eventSourceId,
        &_eventReasonNamespace,
        &_eventSeverity,
        &_eventSequence,
        &_rcTimeBootUs,
        &_rcBootId,
        &_rcProfileId,
        &_rcValidAxesMask,
        &_rcRollNorm,
        &_rcPitchNorm,
        &_rcYawNorm,
        &_rcThrottleNorm,
        &_rcAgeUs,
        &_rcReceiverFailsafe,
        &_rcLinkState,
        &_buildTimeBootUs,
        &_buildBootId,
        &_buildProfileId,
        &_buildProductRevision,
        &_buildPortRevision,
        &_buildSchemaMajor,
        &_buildSchemaMinor,
        &_buildDigestKind,
        &_firmwareSha256,
        &_coreParametersSha256,
        &_profileSha256,
        &_abiSha256,
        &_sourceCommit,
        &_sourceCommitValid
    }) {
        _addFact(fact);
    }
}

QVariantList FlightTelemetryFactGroup::recentEvents() const
{
    return _recentEvents;
}

bool FlightTelemetryFactGroup::_acceptTelemetryMessage(
    quint64 bootId, quint64 timeBootUs, quint64 &lastTimeBootUs)
{
    if (bootId == 0 || timeBootUs == 0) {
        return false;
    }
    if (_activeBootId == 0) {
        _activeBootId = bootId;
    }
    if (bootId != _activeBootId) {
        return false;
    }
    if (timeBootUs <= lastTimeBootUs) {
        return false;
    }
    lastTimeBootUs = timeBootUs;
    return true;
}

void FlightTelemetryFactGroup::_activateBoot(quint64 bootId)
{
    if (bootId == 0 || bootId == _activeBootId) {
        return;
    }
    _activeBootId = bootId;
    _resetMessageTimestamps();
    _resetTelemetryAvailability();
}

void FlightTelemetryFactGroup::_resetMessageTimestamps()
{
    _lastOperationTimeBootUs = 0;
    _lastNavigationTimeBootUs = 0;
    _lastSensorTimeBootUs = 0;
    _lastControlTimeBootUs = 0;
    _lastActuatorTimeBootUs = 0;
    _lastLinkTimeBootUs = 0;
    _lastStorageTimeBootUs = 0;
    _lastResourceTimeBootUs = 0;
    _lastTimeStatusTimeBootUs = 0;
    _lastEventTimeBootUs = 0;
    _eventIdsAtLastTimestamp.clear();
    _eventIdsAtLastTimestampOrder.clear();
    _lastPrearmTimeBootUs = 0;
    _lastRcInputTimeBootUs = 0;
    _lastBuildInfoTimeBootUs = 0;
    _lastBuildInfoDigestMask = 0;
    _eventSequenceValue = 0;
}

void FlightTelemetryFactGroup::_resetTelemetryAvailability()
{
    _operationAvailable.setRawValue(false);
    _navigationAvailable.setRawValue(false);
    _sensorHealthAvailable.setRawValue(false);
    _controlAvailable.setRawValue(false);
    _actuatorAvailable.setRawValue(false);
    _linkAvailable.setRawValue(false);
    _storageAvailable.setRawValue(false);
    _resourceAvailable.setRawValue(false);
    _timeAvailable.setRawValue(false);
    _eventAvailable.setRawValue(false);
    if (!_recentEvents.isEmpty()) {
        _recentEvents.clear();
        emit recentEventsChanged();
    }
    _prearmAvailable.setRawValue(false);
    _rcInputAvailable.setRawValue(false);
    _buildInfoAvailable.setRawValue(false);
    _firmwareSha256.setRawValue(QString());
    _coreParametersSha256.setRawValue(QString());
    _profileSha256.setRawValue(QString());
    _abiSha256.setRawValue(QString());
    _sourceCommit.setRawValue(QString());
    _sourceCommitValid.setRawValue(false);
    _eventSequence.setRawValue(0);
    _setTelemetryAvailable(false);
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
        if (value.boot_id == 0 || value.time_boot_us == 0 || value.arm_state > 3 ||
            value.flight_mode > 8 || value.authority > 3 || value.failsafe_level > 3 ||
            value.mission_state > 5) {
            return;
        }
        _activateBoot(value.boot_id);
        if (!_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastOperationTimeBootUs)) {
            return;
        }
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
        _operationAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_NAV_STATUS: {
        mavlink_fcu_nav_status_t value{};
        mavlink_msg_fcu_nav_status_decode(&message, &value);
        if (!std::isfinite(value.position_std_m) || value.position_std_m < 0.0F ||
            !std::isfinite(value.velocity_std_mps) || value.velocity_std_mps < 0.0F ||
            !std::isfinite(value.heading_std_rad) || value.heading_std_rad < 0.0F ||
            !_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastNavigationTimeBootUs)) {
            return;
        }
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
        _navigationAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_SENSOR_HEALTH: {
        mavlink_fcu_sensor_health_t value{};
        mavlink_msg_fcu_sensor_health_decode(&message, &value);
        if (value.health_state > 7 ||
            !_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastSensorTimeBootUs)) {
            return;
        }
        _sensorTimeBootUs.setRawValue(value.time_boot_us);
        _sensorBootId.setRawValue(value.boot_id);
        _sensorProfileId.setRawValue(value.profile_id);
        _sensorInstanceId.setRawValue(value.sensor_instance_id);
        _sensorPolicyEpoch.setRawValue(value.policy_epoch);
        _sensorReasonCode.setRawValue(value.reason_code);
        _sensorGapCount.setRawValue(value.gap_count);
        _sensorRejectCount.setRawValue(value.reject_count);
        _sensorHealthState.setRawValue(value.health_state);
        _sensorAction.setRawValue(value.action);
        _sensorHealthAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_CONTROL_STATUS: {
        mavlink_fcu_control_status_t value{};
        mavlink_msg_fcu_control_status_decode(&message, &value);
        if (value.solver_status > 8 ||
            !_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastControlTimeBootUs)) {
            return;
        }
        _controlTimeBootUs.setRawValue(value.time_boot_us);
        _controlBootId.setRawValue(value.boot_id);
        _controlProfileId.setRawValue(value.profile_id);
        _controlSequence.setRawValue(value.control_sequence);
        _controlReferenceId.setRawValue(value.reference_id);
        _controlReferenceEpoch.setRawValue(value.reference_epoch);
        _controllableDofMask.setRawValue(value.controllable_dof_mask);
        _degradedDofMask.setRawValue(value.degraded_dof_mask);
        _controlSolveTimeUs.setRawValue(value.solve_time_us);
        _controlReasonCode.setRawValue(value.reason_code);
        _solverStatus.setRawValue(value.solver_status);
        _controlAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_ACTUATOR_STATUS: {
        mavlink_fcu_actuator_status_t value{};
        mavlink_msg_fcu_actuator_status_decode(&message, &value);
        if (!_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastActuatorTimeBootUs)) {
            return;
        }
        _actuatorTimeBootUs.setRawValue(value.time_boot_us);
        _actuatorBootId.setRawValue(value.boot_id);
        _actuatorProfileId.setRawValue(value.profile_id);
        _actuatorControlSequence.setRawValue(value.control_sequence);
        _actuatorPermitEpoch.setRawValue(value.permit_epoch);
        _actuatorAvailableMask.setRawValue(value.available_mask);
        _actuatorSaturatedMask.setRawValue(value.saturated_mask);
        _actuatorReasonCode.setRawValue(value.reason_code);
        _actuatorOutputPolicyId.setRawValue(value.output_policy_id);
        _actuatorFeedbackCapability.setRawValue(value.feedback_capability);
        _actuatorAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_LINK_STATUS: {
        mavlink_fcu_link_status_t value{};
        mavlink_msg_fcu_link_status_decode(&message, &value);
        const bool linkQualityValid = (value.validity_mask & (1U << 9)) != 0;
        const bool packetLossValid = (value.validity_mask & (1U << 10)) != 0;
        if (value.link_kind < 1 || value.link_kind > 4 || value.link_state > 5 ||
            (linkQualityValid && value.link_quality_permille > 1000) ||
            (packetLossValid && value.packet_loss_ppm > 1000000U) ||
            !_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastLinkTimeBootUs)) {
            return;
        }
        _linkTimeBootUs.setRawValue(value.time_boot_us);
        _linkBootId.setRawValue(value.boot_id);
        _linkProfileId.setRawValue(value.profile_id);
        _linkInstanceId.setRawValue(value.link_instance_id);
        _linkValidityMask.setRawValue(value.validity_mask);
        _linkPacketLossPpm.setRawValue(value.packet_loss_ppm);
        _linkAgeUs.setRawValue(value.age_us);
        _linkReasonCode.setRawValue(value.reason_code);
        _linkKind.setRawValue(value.link_kind);
        _linkState.setRawValue(value.link_state);
        _linkRssiDbm.setRawValue(value.rssi_dbm);
        _linkQualityPermille.setRawValue(value.link_quality_permille);
        _linkAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_STORAGE_STATUS: {
        mavlink_fcu_storage_status_t value{};
        mavlink_msg_fcu_storage_status_decode(&message, &value);
        if (value.sd_state > 3 || value.nor_state > 3 ||
            !_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastStorageTimeBootUs)) {
            return;
        }
        _storageTimeBootUs.setRawValue(value.time_boot_us);
        _storageBootId.setRawValue(value.boot_id);
        _storageProfileId.setRawValue(value.profile_id);
        _storageValidityMask.setRawValue(value.validity_mask);
        _storageSettingsGeneration.setRawValue(value.settings_generation);
        _storageCriticalEventDrops.setRawValue(value.critical_event_drops);
        _storageLogDrops.setRawValue(value.log_drops);
        _storageFreeKib.setRawValue(value.free_kib);
        _storageReasonCode.setRawValue(value.reason_code);
        _sdState.setRawValue(value.sd_state);
        _norState.setRawValue(value.nor_state);
        _storageAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_RESOURCE_STATUS: {
        mavlink_fcu_resource_status_t value{};
        mavlink_msg_fcu_resource_status_decode(&message, &value);
        const bool cpuLoadValid = (value.validity_mask & (1U << 2)) != 0;
        if ((cpuLoadValid && value.cpu_load_permille > 1000) ||
            !_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastResourceTimeBootUs)) {
            return;
        }
        _resourceTimeBootUs.setRawValue(value.time_boot_us);
        _resourceBootId.setRawValue(value.boot_id);
        _resourceProfileId.setRawValue(value.profile_id);
        _resourceValidityMask.setRawValue(value.validity_mask);
        _resourceTaskId.setRawValue(value.task_id);
        _resourceWcetUs.setRawValue(value.wcet_us);
        _resourceStackFreeBytes.setRawValue(value.stack_free_bytes);
        _resourceDeadlineMissCount.setRawValue(value.deadline_miss_count);
        _resourceAxiFreeBytes.setRawValue(value.axi_free_bytes);
        _resourceCpuLoadPermille.setRawValue(value.cpu_load_permille);
        _resourceAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_TIME_STATUS: {
        mavlink_fcu_time_status_t value{};
        mavlink_msg_fcu_time_status_decode(&message, &value);
        if (value.utc_valid > 1 || value.time_quality > 5 || value.discipline_state > 5 ||
            !_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastTimeStatusTimeBootUs)) {
            return;
        }
        _timeStatusTimeBootUs.setRawValue(value.time_boot_us);
        _timeStatusBootId.setRawValue(value.boot_id);
        _timeStatusUnixTimeUs.setRawValue(value.unix_time_us);
        _timeStatusProfileId.setRawValue(value.profile_id);
        _timeEpoch.setRawValue(value.time_epoch);
        _sourceMappingEpoch.setRawValue(value.source_mapping_epoch);
        _eventUncertaintyUs.setRawValue(value.event_uncertainty_us);
        _utcUncertaintyUs.setRawValue(value.utc_uncertainty_us);
        _timeQuality.setRawValue(value.time_quality);
        _timeDisciplineState.setRawValue(value.discipline_state);
        _utcValid.setRawValue(value.utc_valid == 1);
        _timeAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_EVENT: {
        mavlink_fcu_event_t value{};
        mavlink_msg_fcu_event_decode(&message, &value);
        if (value.boot_id == 0 || value.time_boot_us == 0 || value.severity > 3 ||
            value.reason_namespace > 3 ||
            (_activeBootId != 0 && value.boot_id != _activeBootId) ||
            value.time_boot_us < _lastEventTimeBootUs) {
            return;
        }
        if (_activeBootId == 0) {
            _activeBootId = value.boot_id;
        }
        if (value.time_boot_us > _lastEventTimeBootUs) {
            _lastEventTimeBootUs = value.time_boot_us;
            _eventIdsAtLastTimestamp.clear();
            _eventIdsAtLastTimestampOrder.clear();
        }
        if (_eventIdsAtLastTimestamp.contains(value.event_id)) {
            return;
        }
        _eventIdsAtLastTimestamp.insert(value.event_id);
        _eventIdsAtLastTimestampOrder.enqueue(value.event_id);
        while (_eventIdsAtLastTimestampOrder.size() > _recentEventHistoryCapacity) {
            _eventIdsAtLastTimestamp.remove(_eventIdsAtLastTimestampOrder.dequeue());
        }
        _eventTimeBootUs.setRawValue(value.time_boot_us);
        _eventBootId.setRawValue(value.boot_id);
        _eventProfileId.setRawValue(value.profile_id);
        _eventId.setRawValue(value.event_id);
        _eventReasonCode.setRawValue(value.reason_code);
        _eventAuthorityEpoch.setRawValue(value.authority_epoch);
        _eventDirectiveEpoch.setRawValue(value.directive_epoch);
        _eventPermitEpoch.setRawValue(value.permit_epoch);
        _eventSourceId.setRawValue(value.source_id);
        _eventReasonNamespace.setRawValue(value.reason_namespace);
        _eventSeverity.setRawValue(value.severity);
        ++_eventSequenceValue;
        if (_eventSequenceValue == 0) {
            ++_eventSequenceValue;
        }
        _eventSequence.setRawValue(_eventSequenceValue);

        QVariantMap eventRecord;
        eventRecord.insert(QStringLiteral("sequence"), _eventSequenceValue);
        eventRecord.insert(QStringLiteral("eventId"), value.event_id);
        eventRecord.insert(QStringLiteral("bootId"), value.boot_id);
        eventRecord.insert(QStringLiteral("profileId"), value.profile_id);
        eventRecord.insert(QStringLiteral("timeBootUs"), QVariant::fromValue<qulonglong>(value.time_boot_us));
        eventRecord.insert(QStringLiteral("sourceId"), value.source_id);
        eventRecord.insert(QStringLiteral("reasonNamespace"), value.reason_namespace);
        eventRecord.insert(QStringLiteral("reasonCode"), value.reason_code);
        eventRecord.insert(QStringLiteral("severity"), value.severity);
        eventRecord.insert(QStringLiteral("authorityEpoch"), value.authority_epoch);
        eventRecord.insert(QStringLiteral("directiveEpoch"), value.directive_epoch);
        eventRecord.insert(QStringLiteral("permitEpoch"), value.permit_epoch);
        _recentEvents.prepend(eventRecord);
        while (_recentEvents.size() > _recentEventHistoryCapacity) {
            _recentEvents.removeLast();
        }
        emit recentEventsChanged();

        _eventAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_PREARM_STATUS: {
        mavlink_fcu_prearm_status_t value{};
        mavlink_msg_fcu_prearm_status_decode(&message, &value);
        if (value.check_id > 14 || value.check_state > 3 || value.reason_namespace > 3 ||
            value.recovery_condition > 4 ||
            !_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastPrearmTimeBootUs)) {
            return;
        }
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
        _prearmAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_RC_INPUT_STATUS: {
        mavlink_fcu_rc_input_status_t value{};
        mavlink_msg_fcu_rc_input_status_decode(&message, &value);
        const auto validAxisValue = [](bool valid, float axis, float minimum, float maximum) {
            return !valid || (std::isfinite(axis) && axis >= minimum && axis <= maximum);
        };
        if (value.valid_axes_mask > 0x0F || value.receiver_failsafe > 1 || value.link_state > 5 ||
            !validAxisValue((value.valid_axes_mask & (1U << 0)) != 0, value.roll_norm, -1.0F, 1.0F) ||
            !validAxisValue((value.valid_axes_mask & (1U << 1)) != 0, value.pitch_norm, -1.0F, 1.0F) ||
            !validAxisValue((value.valid_axes_mask & (1U << 2)) != 0, value.yaw_norm, -1.0F, 1.0F) ||
            !validAxisValue((value.valid_axes_mask & (1U << 3)) != 0, value.throttle_norm, 0.0F, 1.0F) ||
            !_acceptTelemetryMessage(value.boot_id, value.time_boot_us, _lastRcInputTimeBootUs)) {
            return;
        }
        _rcTimeBootUs.setRawValue(value.time_boot_us);
        _rcBootId.setRawValue(value.boot_id);
        _rcProfileId.setRawValue(value.profile_id);
        _rcValidAxesMask.setRawValue(value.valid_axes_mask);
        _rcRollNorm.setRawValue(value.roll_norm);
        _rcPitchNorm.setRawValue(value.pitch_norm);
        _rcYawNorm.setRawValue(value.yaw_norm);
        _rcThrottleNorm.setRawValue(value.throttle_norm);
        _rcAgeUs.setRawValue(value.age_us);
        _rcReceiverFailsafe.setRawValue(value.receiver_failsafe == 1);
        _rcLinkState.setRawValue(value.link_state);
        _rcInputAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    case MAVLINK_MSG_ID_FCU_BUILD_INFO: {
        mavlink_fcu_build_info_t value{};
        mavlink_msg_fcu_build_info_decode(&message, &value);
        if (value.boot_id == 0 || value.time_boot_us == 0 || value.digest_kind > 3 ||
            value.source_commit_valid > 1 ||
            (_activeBootId != 0 && value.boot_id != _activeBootId) ||
            value.time_boot_us < _lastBuildInfoTimeBootUs) {
            return;
        }
        if (value.source_commit_valid == 0) {
            for (const uint8_t byte : value.source_commit) {
                if (byte != 0) {
                    return;
                }
            }
        }
        if (_activeBootId == 0) {
            _activeBootId = value.boot_id;
        }
        if (value.time_boot_us > _lastBuildInfoTimeBootUs) {
            _lastBuildInfoTimeBootUs = value.time_boot_us;
            _lastBuildInfoDigestMask = 0;
        }
        const quint8 digestBit = static_cast<quint8>(1U << value.digest_kind);
        if ((_lastBuildInfoDigestMask & digestBit) != 0) {
            return;
        }
        _lastBuildInfoDigestMask |= digestBit;
        _buildTimeBootUs.setRawValue(value.time_boot_us);
        _buildBootId.setRawValue(value.boot_id);
        _buildProfileId.setRawValue(value.profile_id);
        _buildProductRevision.setRawValue(value.product_revision);
        _buildPortRevision.setRawValue(value.port_revision);
        _buildSchemaMajor.setRawValue(value.schema_major);
        _buildSchemaMinor.setRawValue(value.schema_minor);
        _buildDigestKind.setRawValue(value.digest_kind);

        const QByteArray digest(reinterpret_cast<const char *>(value.sha256),
                                static_cast<qsizetype>(sizeof(value.sha256)));
        const QString digestHex = QString::fromLatin1(digest.toHex());
        switch (value.digest_kind) {
        case 0: _firmwareSha256.setRawValue(digestHex); break;
        case 1: _coreParametersSha256.setRawValue(digestHex); break;
        case 2: _profileSha256.setRawValue(digestHex); break;
        case 3: _abiSha256.setRawValue(digestHex); break;
        default: return;
        }

        _sourceCommitValid.setRawValue(value.source_commit_valid == 1);
        if (value.source_commit_valid == 1) {
            const QByteArray commit(reinterpret_cast<const char *>(value.source_commit),
                                    static_cast<qsizetype>(sizeof(value.source_commit)));
            _sourceCommit.setRawValue(QString::fromLatin1(commit.toHex()));
        } else {
            _sourceCommit.setRawValue(QString());
        }
        _buildInfoAvailable.setRawValue(true);
        _setTelemetryAvailable(true);
        break;
    }
    default:
        break;
    }
}
