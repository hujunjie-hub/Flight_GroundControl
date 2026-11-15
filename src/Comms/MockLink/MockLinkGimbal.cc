#include "MockLinkGimbal.h"
#include "MAVLinkLib.h"
#include "MockLink.h"
#include "MissionCommandTree.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtMath>
#include <utility>

QGC_LOGGING_CATEGORY(MockLinkGimbalLog, "Comms.MockLink.MockLinkGimbal")

MockLinkGimbal::MockLinkGimbal(
    MockLink *mockLink,
    bool hasRollAxis,
    bool hasPitchAxis,
    bool hasYawAxis,
    bool hasYawFollow,
    bool hasYawLock,
    bool hasRetract,
    bool hasNeutral,
    uint8_t gimbalDeviceId,
    uint8_t managerCompId)
    : _mockLink(mockLink)
    , _gimbalDeviceId(gimbalDeviceId)
    , _managerCompId(managerCompId)
    , _deviceCompId(gimbalDeviceId <= 6 ? managerCompId : gimbalDeviceId)
    , _hasRollAxis(hasRollAxis)
    , _hasPitchAxis(hasPitchAxis)
    , _hasYawAxis(hasYawAxis)
    , _hasYawFollow(hasYawFollow)
    , _hasYawLock(hasYawLock)
    , _hasRetract(hasRetract)
    , _hasNeutral(hasNeutral)
{
}

MockLinkGimbal::~MockLinkGimbal()
{
    for (MockLinkGimbal *device : std::as_const(_additionalDevices)) {
        delete device;
    }
}

bool MockLinkGimbal::addSimulatedDevice(
    uint8_t gimbalDeviceId,
    bool hasRollAxis,
    bool hasPitchAxis,
    bool hasYawAxis,
    bool hasYawFollow,
    bool hasYawLock,
    bool hasRetract,
    bool hasNeutral)
{
    if (_gimbalDeviceId != kDefaultGimbalDeviceId ||
        gimbalDeviceId == 0 ||
        gimbalDeviceId == _gimbalDeviceId ||
        _deviceForId(gimbalDeviceId)) {
        return false;
    }

    _additionalDevices.append(new MockLinkGimbal(
        _mockLink,
        hasRollAxis,
        hasPitchAxis,
        hasYawAxis,
        hasYawFollow,
        hasYawLock,
        hasRetract,
        hasNeutral,
        gimbalDeviceId,
        _managerCompId));
    return true;
}

MockLinkGimbal *MockLinkGimbal::_deviceForId(uint8_t gimbalDeviceId)
{
    if (gimbalDeviceId == 0 || gimbalDeviceId == _gimbalDeviceId) {
        return this;
    }

    for (MockLinkGimbal *device : std::as_const(_additionalDevices)) {
        if (device->_gimbalDeviceId == gimbalDeviceId) {
            return device;
        }
    }
    return nullptr;
}

void MockLinkGimbal::run1HzTasks()
{
    // Runs every 1s (1Hz on worker thread). Reads intervals and gimbal state main thread modifies.
    // Must serialize access to prevent:
    //   - Auto-movement overwriting manual control commands
    //   - Reading stale/inconsistent interval values for status transmission
    QMutexLocker locker(&_stateMutex);
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    // Send GIMBAL_MANAGER_STATUS if interval is set
    if (_managerStatusIntervalUs > 0) {
        const qint64 intervalMs = _managerStatusIntervalUs / 1000;
        if ((nowMs - _managerStatusLastSentMs) >= intervalMs) {
            _sendGimbalManagerStatus();
            _managerStatusLastSentMs = nowMs;
        }
    }

    // Send GIMBAL_DEVICE_ATTITUDE_STATUS if interval is set
    if (_deviceAttitudeStatusIntervalUs > 0) {
        const qint64 intervalMs = _deviceAttitudeStatusIntervalUs / 1000;
        if ((nowMs - _deviceAttitudeStatusLastSentMs) >= intervalMs) {
            if (_rateControl) {
                if (_hasPitchAxis) {
                    _pitch = qBound(-45.0f, _pitch + _pitchRateDegS, 45.0f);
                }
                if (_hasYawAxis) {
                    _yaw = qBound(-180.0f, _yaw + _yawRateDegS, 180.0f);
                }
            } else if (!_manualControl) {
                // Simulate simple gimbal movement (slow sine wave).
                const qint64 secondsSinceEpoch = nowMs / 1000;
                const float devicePhase = (static_cast<int>(_gimbalDeviceId) - static_cast<int>(kDefaultGimbalDeviceId)) * 0.5f;
                _pitch = 10.0f * qSin(secondsSinceEpoch * 0.1 + devicePhase);
                _yaw = 15.0f * qCos(secondsSinceEpoch * 0.15 + devicePhase);
            }

            _sendGimbalDeviceAttitudeStatus();
            _deviceAttitudeStatusLastSentMs = nowMs;
        }
    }

    // Every device owns its own intervals, attitude, rate state and owner pairs.
    for (MockLinkGimbal *device : std::as_const(_additionalDevices)) {
        device->run1HzTasks();
    }
}

bool MockLinkGimbal::handleMavlinkMessage(const mavlink_message_t &msg)
{
    if (msg.msgid == MAVLINK_MSG_ID_GIMBAL_MANAGER_SET_ATTITUDE) {
        mavlink_gimbal_manager_set_attitude_t request{};
        mavlink_msg_gimbal_manager_set_attitude_decode(&msg, &request);
        MockLinkGimbal *const device = _deviceForId(request.gimbal_device_id);
        return device && device->_handleGimbalManagerSetAttitude(request, msg.sysid, msg.compid);
    }

    if (msg.msgid != MAVLINK_MSG_ID_COMMAND_LONG) {
        return false;
    }

    mavlink_command_long_t request{};
    mavlink_msg_command_long_decode(&msg, &request);

    switch (request.command) {
    case MAV_CMD_SET_MESSAGE_INTERVAL: {
        if ((request.target_system != 0 && request.target_system != _mockLink->vehicleId())) {
            return false;
        }
        const int messageId = static_cast<int>(request.param1);
        bool handled = false;
        if (messageId == MAVLINK_MSG_ID_GIMBAL_MANAGER_STATUS) {
            if (request.target_component != 0 && request.target_component != _managerCompId) {
                return false;
            }
            // The manager publishes one status stream containing one frame per device.
            handled = _handleSetMessageInterval(request);
            for (MockLinkGimbal *device : std::as_const(_additionalDevices)) {
                handled = device->_handleSetMessageInterval(request) || handled;
            }
        } else if (messageId == MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS) {
            if (request.target_component == 0) {
                handled = _handleSetMessageInterval(request);
            } else if (request.target_component == _managerCompId) {
                // Device IDs 1-6 publish attitude status through the manager component.
                for (MockLinkGimbal *device : std::as_const(_additionalDevices)) {
                    if (device->_gimbalDeviceId <= 6) {
                        handled = device->_handleSetMessageInterval(request) || handled;
                    }
                }
                if (_gimbalDeviceId <= 6) {
                    handled = _handleSetMessageInterval(request) || handled;
                }
            } else {
                MockLinkGimbal *const device = _deviceForId(
                    request.target_component == _deviceCompId ? _gimbalDeviceId : request.target_component);
                if (device && device->_deviceCompId == request.target_component) {
                    handled = device->_handleSetMessageInterval(request);
                } else {
                    for (MockLinkGimbal *candidate : std::as_const(_additionalDevices)) {
                        if (candidate->_deviceCompId == request.target_component) {
                            handled = candidate->_handleSetMessageInterval(request);
                            break;
                        }
                    }
                }
            }
        }

        if (!handled) {
            return false;
        }
        _sendCommandAck(request.command, MAV_RESULT_ACCEPTED, request.target_component);
        return true;
    }

    case MAV_CMD_REQUEST_MESSAGE:
        if ((request.target_system != 0 && request.target_system != _mockLink->vehicleId()) ||
            (request.target_component != 0 && request.target_component != _managerCompId)) {
            return false;
        }
        if (!_handleRequestMessage(request)) {
            return false;
        }
        _sendCommandAck(request.command, MAV_RESULT_ACCEPTED, request.target_component);
        return true;

    case MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW: {
        if ((request.target_system != 0 && request.target_system != _mockLink->vehicleId()) ||
            (request.target_component != 0 && request.target_component != _managerCompId)) {
            return false;
        }
        MockLinkGimbal *device = this;
        if (qIsFinite(request.param7) && request.param7 >= 0.0f &&
            request.param7 <= 255.0f && qFloor(request.param7) == request.param7) {
            device = _deviceForId(static_cast<uint8_t>(request.param7));
        }
        const bool accepted = device &&
            device->_handleGimbalManagerPitchYaw(request, msg.sysid, msg.compid);
        _sendCommandAck(request.command, accepted ? MAV_RESULT_ACCEPTED : MAV_RESULT_DENIED,
                        request.target_component);
        return true;
    }

    case MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE: {
        if ((request.target_system != 0 && request.target_system != _mockLink->vehicleId()) ||
            (request.target_component != 0 && request.target_component != _managerCompId)) {
            return false;
        }
        MockLinkGimbal *device = this;
        if (qIsFinite(request.param7) && request.param7 >= 0.0f &&
            request.param7 <= 255.0f && qFloor(request.param7) == request.param7) {
            device = _deviceForId(static_cast<uint8_t>(request.param7));
        } else if (!qIsNaN(request.param7)) {
            device = nullptr;
        }
        const bool accepted = device &&
            device->_handleGimbalManagerConfigure(request, msg.sysid, msg.compid);
        _sendCommandAck(request.command, accepted ? MAV_RESULT_ACCEPTED : MAV_RESULT_DENIED,
                        request.target_component);
        return true;
    }

    default:
        return false;
    }
}

bool MockLinkGimbal::_handleGimbalManagerSetAttitude(
    const mavlink_gimbal_manager_set_attitude_t &request, uint8_t sourceSysid, uint8_t sourceCompid)
{
    if ((request.target_system != 0 && request.target_system != _mockLink->vehicleId()) ||
        (request.target_component != 0 && request.target_component != _managerCompId) ||
        (request.gimbal_device_id != 0 && request.gimbal_device_id != _gimbalDeviceId) ||
        (request.flags & ~0x7Cu) != 0) {
        return false;
    }

    // This MockLink path models rate control. The controller sends an all-NaN
    // quaternion and expresses pitch/yaw rates in angular_velocity_y/z.
    if (!qIsNaN(request.q[0]) || !qIsNaN(request.q[1]) ||
        !qIsNaN(request.q[2]) || !qIsNaN(request.q[3]) ||
        (!qIsNaN(request.angular_velocity_x) &&
         (!qIsFinite(request.angular_velocity_x) || !qFuzzyIsNull(request.angular_velocity_x))) ||
        (!qIsNaN(request.angular_velocity_y) && !qIsFinite(request.angular_velocity_y)) ||
        (!qIsNaN(request.angular_velocity_z) && !qIsFinite(request.angular_velocity_z))) {
        return false;
    }

    const bool updatePitchRate = qIsFinite(request.angular_velocity_y);
    const bool updateYawRate = qIsFinite(request.angular_velocity_z);
    const float pitchRateDegS = updatePitchRate ? qRadiansToDegrees(request.angular_velocity_y) : 0.0f;
    const float yawRateDegS = updateYawRate ? qRadiansToDegrees(request.angular_velocity_z) : 0.0f;
    const bool yawLockRequested = (request.flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK) != 0;
    if ((updatePitchRate && !_hasPitchAxis && !qFuzzyIsNull(pitchRateDegS)) ||
        (updateYawRate && !_hasYawAxis && !qFuzzyIsNull(yawRateDegS)) ||
        (updateYawRate && _hasYawAxis && yawLockRequested && !_hasYawLock) ||
        (updateYawRate && _hasYawAxis && !yawLockRequested && !_hasYawFollow)) {
        return false;
    }

    QMutexLocker locker(&_stateMutex);
    const bool validSource = sourceSysid != 0 && sourceCompid != 0;
    const bool isPrimaryController = validSource &&
                                     _gimbalManagerSysidPrimary == sourceSysid &&
                                     _gimbalManagerCompidPrimary == sourceCompid;
    const bool isSecondaryController = validSource &&
                                       _gimbalManagerSysidSecondary == sourceSysid &&
                                       _gimbalManagerCompidSecondary == sourceCompid;
    if (!isPrimaryController && !isSecondaryController) {
        qCDebug(MockLinkGimbalLog) << "GIMBAL_MANAGER_SET_ATTITUDE denied for non-owner"
                                   << sourceSysid << "/" << sourceCompid;
        return false;
    }

    if (updatePitchRate && _hasPitchAxis) {
        _pitchRateDegS = pitchRateDegS;
    }
    if (updateYawRate && _hasYawAxis) {
        _yawRateDegS = yawRateDegS;
        _yawLock = yawLockRequested;
    }
    _rateControl = !qFuzzyIsNull(_pitchRateDegS) || !qFuzzyIsNull(_yawRateDegS);
    _manualControl = true;
    return true;
}

void MockLinkGimbal::_sendCommandAck(uint16_t command, uint8_t result, uint8_t sourceCompId)
{
    QString commandName = MissionCommandTree::instance()->rawName(static_cast<MAV_CMD>(command));
    qCDebug(MockLinkGimbalLog) << "Sending command ACK -" << QString("%1(%2)").arg(commandName).arg(command) << "result:" << (result == MAV_RESULT_ACCEPTED ? "ACCEPTED" : result == MAV_RESULT_DENIED ? "DENIED" : QString::number(result));

    mavlink_message_t msg{};
    (void) mavlink_msg_command_ack_pack_chan(
        _mockLink->vehicleId(),
        sourceCompId,
        _mockLink->outgoingMavlinkChannel(),
        &msg,
        command,
        result,
        0,    // progress
        0,    // result_param2
        0,    // target_system
        0     // target_component
    );
    _mockLink->respondWithMavlinkMessage(msg);
}

bool MockLinkGimbal::_handleSetMessageInterval(const mavlink_command_long_t &request)
{
    const int msgId = static_cast<int>(request.param1);
    const int intervalUs = static_cast<int>(request.param2);
    const mavlink_message_info_t* info = mavlink_get_message_info_by_id(static_cast<uint32_t>(msgId));
    QString msgName = info ? info->name : QString::number(msgId);

    qCDebug(MockLinkGimbalLog) << "SET_MESSAGE_INTERVAL -" << QString("%1(%2)").arg(msgName).arg(msgId) << "intervalUs:" << intervalUs;

    // Handle interval values per MAVLink spec:
    // -1 = disable
    // 0 = default rate
    // >0 = interval in microseconds
    int effectiveInterval = intervalUs;
    if (intervalUs == 0) {
        effectiveInterval = kDefaultIntervalUs;
    } else if (intervalUs < -1) {
        // Invalid interval
        return false;
    }

    // Thread-safe access: Main thread writing interval while worker thread reads every 1s.
    // Serialize to avoid worker using stale interval for message transmission logic.
    QMutexLocker locker(&_stateMutex);
    if (msgId == MAVLINK_MSG_ID_GIMBAL_MANAGER_STATUS) {
        _managerStatusIntervalUs = effectiveInterval;
        qCDebug(MockLinkGimbalLog) << msgName << "interval set to" << effectiveInterval << "us";
        return true;
    } else if (msgId == MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS) {
        _deviceAttitudeStatusIntervalUs = effectiveInterval;
        qCDebug(MockLinkGimbalLog) << msgName << "interval set to" << effectiveInterval << "us";
        return true;
    }
    return false;
}

bool MockLinkGimbal::_handleRequestMessage(const mavlink_command_long_t &request)
{
    const int msgId = static_cast<int>(request.param1);
    const mavlink_message_info_t* info = mavlink_get_message_info_by_id(static_cast<uint32_t>(msgId));
    QString msgName = info ? info->name : QString::number(msgId);

    if (msgId == MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION) {
        qCDebug(MockLinkGimbalLog) << "REQUEST_MESSAGE -" << QString("%1(%2)").arg(msgName).arg(msgId);
        _sendGimbalManagerInformation();
        for (MockLinkGimbal *device : std::as_const(_additionalDevices)) {
            device->_sendGimbalManagerInformation();
        }
        return true;
    }

    return false;
}

bool MockLinkGimbal::_handleGimbalManagerPitchYaw(const mavlink_command_long_t &request,
                                                    uint8_t sourceSysid, uint8_t sourceCompid)
{
    // param1: pitch angle (degrees, positive: down, negative: up)
    // param2: yaw angle (degrees, positive: to the right, negative: to the left)
    // param3: pitch rate (deg/s)
    // param4: yaw rate (deg/s)
    // param5: flags (GIMBAL_MANAGER_FLAGS)
    // param7: gimbal device id

    const float requestedPitch = request.param1;
    const float requestedYaw = request.param2;

    // NaN means "leave this axis unchanged"; infinities and malformed target IDs are invalid.
    const float requestedDeviceId = request.param7;
    if ((!qIsNaN(requestedPitch) && !qIsFinite(requestedPitch)) ||
        (!qIsNaN(requestedYaw) && !qIsFinite(requestedYaw)) ||
        (!qIsNaN(request.param3) && !qIsFinite(request.param3)) ||
        (!qIsNaN(request.param4) && !qIsFinite(request.param4)) ||
        !qIsFinite(request.param5) || request.param5 < 0.0f ||
        request.param5 > 65535.0f || qFloor(request.param5) != request.param5 ||
        !qIsFinite(requestedDeviceId) || requestedDeviceId < 0.0f ||
        requestedDeviceId > 255.0f || qFloor(requestedDeviceId) != requestedDeviceId) {
        return false;
    }
    const uint32_t flags = static_cast<uint32_t>(request.param5);
    qCDebug(MockLinkGimbalLog) << "DO_GIMBAL_MANAGER_PITCHYAW - pitch:" << requestedPitch
                               << "yaw:" << requestedYaw << "flags:" << flags;

    const uint8_t targetDeviceId = static_cast<uint8_t>(requestedDeviceId);
    if (targetDeviceId != 0 && targetDeviceId != _gimbalDeviceId) {
        qCDebug(MockLinkGimbalLog) << "DO_GIMBAL_MANAGER_PITCHYAW denied for unknown device" << targetDeviceId;
        return false;
    }
    if ((flags & ~0x7Fu) != 0) {
        qCDebug(MockLinkGimbalLog) << "DO_GIMBAL_MANAGER_PITCHYAW denied for unknown flags" << flags;
        return false;
    }

    const bool updatePitch = qIsFinite(requestedPitch);
    const bool updateYaw = qIsFinite(requestedYaw);
    const bool retractRequested = (flags & GIMBAL_DEVICE_FLAGS_RETRACT) != 0;
    const bool yawLockRequested = (flags & GIMBAL_DEVICE_FLAGS_YAW_LOCK) != 0;
    const bool hasSupportedAxis = (updatePitch && _hasPitchAxis) || (updateYaw && _hasYawAxis);
    const bool retractOnlyCommand = !updatePitch && !updateYaw && _hasRetract;
    if (!hasSupportedAxis && !retractOnlyCommand) {
        qCDebug(MockLinkGimbalLog) << "DO_GIMBAL_MANAGER_PITCHYAW has no requested supported axis";
        return false;
    }
    if ((retractRequested && !_hasRetract) ||
        (updateYaw && _hasYawAxis && yawLockRequested && !_hasYawLock) ||
        (updateYaw && _hasYawAxis && !yawLockRequested && !_hasYawFollow)) {
        return false;
    }

    // Thread-safe access: Main thread setting manual control state that worker reads every 1s.
    // Without lock: Worker could read _manualControl=false and auto-update pitch/yaw after main
    // sets them to manual values, overwriting the manual control with auto-movement.
    QMutexLocker locker(&_stateMutex);
    const bool validSource = sourceSysid != 0 && sourceCompid != 0;
    const bool isPrimaryController = validSource &&
                                     _gimbalManagerSysidPrimary == sourceSysid &&
                                     _gimbalManagerCompidPrimary == sourceCompid;
    const bool isSecondaryController = validSource &&
                                       _gimbalManagerSysidSecondary == sourceSysid &&
                                       _gimbalManagerCompidSecondary == sourceCompid;
    if (!isPrimaryController && !isSecondaryController) {
        qCDebug(MockLinkGimbalLog) << "DO_GIMBAL_MANAGER_PITCHYAW denied for non-owner"
                                   << sourceSysid << "/" << sourceCompid;
        return false;
    }

    // A position command supersedes any active rate command.
    _rateControl = false;
    _pitchRateDegS = 0.0f;
    _yawRateDegS = 0.0f;

    // Apply limits based on gimbal capabilities
    if (updatePitch && _hasPitchAxis) {
        _pitch = qBound(-45.0f, requestedPitch, 45.0f);
        _manualControl = true;  // Switch to manual control mode
    }

    if (updateYaw && _hasYawAxis) {
        _yaw = qBound(-180.0f, requestedYaw, 180.0f);
        _yawLock = yawLockRequested;
        _manualControl = true;  // Switch to manual control mode
    }

    if (_hasRetract) {
        _retracted = retractRequested;
    }

    qCDebug(MockLinkGimbalLog) << "Gimbal commanded to pitch:" << _pitch << "yaw:" << _yaw
                               << "yawLock:" << _yawLock << "retracted:" << _retracted;
    return true;
}

bool MockLinkGimbal::_handleGimbalManagerConfigure(const mavlink_command_long_t &request,
                                                   uint8_t sourceSysid, uint8_t sourceCompid)
{
    // MAVLink Gimbal Manager Configure semantics (per parameter):
    //   0: no one in control; -1: leave unchanged; -2: set this sender in control;
    //   -3: release only if this sender currently owns the complete pair.
    // Positive integer values set an explicit system/component ID.
    // param7 selects the device; NaN is accepted for legacy callers targeting the default device.
    if (!qIsNaN(request.param7) &&
        (!qIsFinite(request.param7) || request.param7 < 0.0f ||
         request.param7 > 255.0f || qFloor(request.param7) != request.param7 ||
         (request.param7 != 0.0f && request.param7 != _gimbalDeviceId))) {
        return false;
    }
    QMutexLocker locker(&_stateMutex);

    const auto isRelease = [](float value) { return qIsFinite(value) && value == -3.0f; };
    const auto validIdValue = [](float value) {
        if (!qIsFinite(value)) {
            return false;
        }
        if (value == 0.0f || value == -1.0f || value == -2.0f || value == -3.0f) {
            return true;
        }
        return value > 0.0f && value <= 255.0f && qFloor(value) == value;
    };

    const bool primaryReleaseRequested = isRelease(request.param1) || isRelease(request.param2);
    const bool secondaryReleaseRequested = isRelease(request.param3) || isRelease(request.param4);
    const bool releasePrimary = isRelease(request.param1) && isRelease(request.param2);
    const bool releaseSecondary = isRelease(request.param3) && isRelease(request.param4);

    // Reject malformed parameter values and partial -3 release pairs before
    // changing either ownership slot.
    if (!validIdValue(request.param1) || !validIdValue(request.param2) ||
        !validIdValue(request.param3) || !validIdValue(request.param4) ||
        (primaryReleaseRequested && !releasePrimary) ||
        (secondaryReleaseRequested && !releaseSecondary) ||
        ((request.param1 == -2.0f || request.param3 == -2.0f) && sourceSysid == 0) ||
        ((request.param2 == -2.0f || request.param4 == -2.0f) && sourceCompid == 0)) {
        return false;
    }
    if (releasePrimary && (_gimbalManagerSysidPrimary != sourceSysid ||
                           _gimbalManagerCompidPrimary != sourceCompid)) {
        return false;
    }
    if (releaseSecondary && (_gimbalManagerSysidSecondary != sourceSysid ||
                             _gimbalManagerCompidSecondary != sourceCompid)) {
        return false;
    }

    const auto resultingId = [](float value, uint8_t sourceId, uint8_t currentId) {
        if (value == -1.0f) return currentId;
        if (value == 0.0f || value == -3.0f) return static_cast<uint8_t>(0);
        if (value == -2.0f) return sourceId;
        return static_cast<uint8_t>(value);
    };
    const uint8_t nextPrimarySysid = resultingId(request.param1, sourceSysid, _gimbalManagerSysidPrimary);
    const uint8_t nextPrimaryCompid = resultingId(request.param2, sourceCompid, _gimbalManagerCompidPrimary);
    const uint8_t nextSecondarySysid = resultingId(request.param3, sourceSysid, _gimbalManagerSysidSecondary);
    const uint8_t nextSecondaryCompid = resultingId(request.param4, sourceCompid, _gimbalManagerCompidSecondary);
    if ((nextPrimarySysid == 0) != (nextPrimaryCompid == 0) ||
        (nextSecondarySysid == 0) != (nextSecondaryCompid == 0)) {
        qCDebug(MockLinkGimbalLog) << "Gimbal manager rejected incomplete owner pair";
        return false;
    }

    const auto applyIdValue = [](float value, uint8_t sourceId, uint8_t &currentId) {
        if (value == -1.0f) {
            return; // Leave unchanged.
        }
        if (value == 0.0f) {
            currentId = 0; // No one in control.
        } else if (value == -2.0f) {
            currentId = sourceId; // Set this sender as controller.
        } else if (value > 0.0f) {
            currentId = static_cast<uint8_t>(value);
        }
        // -3 is handled as an ownership-checked pair below.
    };

    if (releasePrimary) {
        _gimbalManagerSysidPrimary = 0;
        _gimbalManagerCompidPrimary = 0;
    } else {
        applyIdValue(request.param1, sourceSysid, _gimbalManagerSysidPrimary);
        applyIdValue(request.param2, sourceCompid, _gimbalManagerCompidPrimary);
    }

    if (releaseSecondary) {
        _gimbalManagerSysidSecondary = 0;
        _gimbalManagerCompidSecondary = 0;
    } else {
        applyIdValue(request.param3, sourceSysid, _gimbalManagerSysidSecondary);
        applyIdValue(request.param4, sourceCompid, _gimbalManagerCompidSecondary);
    }

    qCDebug(MockLinkGimbalLog) << "Gimbal manager configured - Primary:" << _gimbalManagerSysidPrimary
                                << "/" << _gimbalManagerCompidPrimary
                                << "Secondary:" << _gimbalManagerSysidSecondary
                                << "/" << _gimbalManagerCompidSecondary;
    return true;
}

void MockLinkGimbal::_sendGimbalManagerStatus()
{
    qCDebug(MockLinkGimbalLog) << "Sending GIMBAL_MANAGER_STATUS";

    mavlink_message_t msg{};
    (void) mavlink_msg_gimbal_manager_status_pack_chan(
        _mockLink->vehicleId(),
        _managerCompId,
        _mockLink->outgoingMavlinkChannel(),
        &msg,
        0, // time_boot_ms
        0, // flags
        _gimbalDeviceId, // gimbal_device_id
        _gimbalManagerSysidPrimary,
        _gimbalManagerCompidPrimary,
        _gimbalManagerSysidSecondary,
        _gimbalManagerCompidSecondary);
    _mockLink->respondWithMavlinkMessage(msg);
}

void MockLinkGimbal::_sendGimbalDeviceAttitudeStatus()
{
    qCDebug(MockLinkGimbalLog) << "Sending GIMBAL_DEVICE_ATTITUDE_STATUS - pitch:" << _pitch << "yaw:" << _yaw << "manual:" << _manualControl;

    // Convert Euler angles (degrees) to quaternion
    const float rollRad = qDegreesToRadians(_roll);
    const float pitchRad = qDegreesToRadians(_pitch);
    const float yawRad = qDegreesToRadians(_yaw);

    const float cy = qCos(yawRad * 0.5f);
    const float sy = qSin(yawRad * 0.5f);
    const float cp = qCos(pitchRad * 0.5f);
    const float sp = qSin(pitchRad * 0.5f);
    const float cr = qCos(rollRad * 0.5f);
    const float sr = qSin(rollRad * 0.5f);

    float q[4];
    q[0] = cr * cp * cy + sr * sp * sy;  // w
    q[1] = sr * cp * cy - cr * sp * sy;  // x
    q[2] = cr * sp * cy + sr * cp * sy;  // y
    q[3] = cr * cp * sy - sr * sp * cy;  // z

    mavlink_message_t msg{};
    uint32_t deviceFlags = GIMBAL_DEVICE_FLAGS_NEUTRAL |
                           GIMBAL_DEVICE_FLAGS_ROLL_LOCK |
                           GIMBAL_DEVICE_FLAGS_PITCH_LOCK;
    if (_yawLock) {
        deviceFlags |= GIMBAL_DEVICE_FLAGS_YAW_LOCK;
    }
    if (_retracted) {
        deviceFlags |= GIMBAL_DEVICE_FLAGS_RETRACT;
    }

    (void) mavlink_msg_gimbal_device_attitude_status_pack_chan(
        _mockLink->vehicleId(),
        _deviceCompId,
        _mockLink->outgoingMavlinkChannel(),
        &msg,
        0, 0,   // target system, component
        0,      // time_boot_ms
        deviceFlags,
        q,
        0.0f, 0.0f, 0.0f,   // angular_velocity_x, y, z
        0,                  // failure flags
        _pitch, _yaw,       // delta_pitch, delta_yaw (Euler angles in degrees)
        _gimbalDeviceId <= 6 ? _gimbalDeviceId : 0); // Device IDs above 6 use their component ID
    _mockLink->respondWithMavlinkMessage(msg);
}

void MockLinkGimbal::_sendGimbalManagerInformation()
{
    // Build capability flags bitmask based on configuration
    uint32_t capFlags = 0;
    if (_hasRollAxis) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_ROLL_AXIS;
    if (_hasPitchAxis) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_PITCH_AXIS;
    if (_hasYawAxis) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_AXIS;
    if (_hasYawFollow) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_FOLLOW;
    if (_hasYawLock) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_LOCK;
    if (_hasRetract) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_RETRACT;
    if (_hasNeutral) capFlags |= GIMBAL_MANAGER_CAP_FLAGS_HAS_NEUTRAL;

    qCDebug(MockLinkGimbalLog) << "Sending GIMBAL_MANAGER_INFORMATION - capFlags:" << QString::number(capFlags, 16);

    mavlink_message_t msg{};
    (void) mavlink_msg_gimbal_manager_information_pack_chan(
        _mockLink->vehicleId(),
        _managerCompId,
        _mockLink->outgoingMavlinkChannel(),
        &msg,
        0, // time_boot_ms
        capFlags,
        _gimbalDeviceId,
        -45, 45,
        -45, 45,
        -180, 180);
    _mockLink->respondWithMavlinkMessage(msg);
}
