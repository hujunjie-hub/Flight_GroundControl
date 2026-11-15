#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QList>
#include <QtCore/QMutex>

class MockLink;

/// \brief Simulates MAVLink Gimbal Manager Protocol for MockLink.
///
/// Supports multiple Gimbal Managers, each with independently simulated gimbal devices.
/// Gimbal attitude is simulated with a slow oscillation when not under manual control.
///
/// Gimbal capabilities are configurable via MockLink config.
///
/// Supported MAVLink messages / commands:
///   MAV_CMD_REQUEST_MESSAGE (GIMBAL_MANAGER_INFORMATION)
///   MAV_CMD_SET_MESSAGE_INTERVAL (GIMBAL_MANAGER_STATUS, GIMBAL_DEVICE_ATTITUDE_STATUS)
///   MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW
///   MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE
///   MAVLINK_MSG_ID_GIMBAL_MANAGER_SET_ATTITUDE (rate control)
///   Periodic sending of GIMBAL_MANAGER_STATUS and GIMBAL_DEVICE_ATTITUDE_STATUS
///
class MockLinkGimbal
{
public:
    explicit MockLinkGimbal(
        MockLink *mockLink,
        bool hasRollAxis = true,
        bool hasPitchAxis = true,
        bool hasYawAxis = true,
        bool hasYawFollow = true,
        bool hasYawLock = true,
        bool hasRetract = true,
        bool hasNeutral = true,
        uint8_t gimbalDeviceId = MAV_COMP_ID_GIMBAL,
        uint8_t managerCompId = MAV_COMP_ID_AUTOPILOT1);
    ~MockLinkGimbal();

    /// Add an independently stateful device behind this manager. Device IDs 1-6
    /// use the manager component for attitude telemetry; IDs above 6 use their
    /// own component ID, as required by the MAVLink Gimbal protocol.
    bool addSimulatedDevice(
        uint8_t gimbalDeviceId,
        bool hasRollAxis,
        bool hasPitchAxis,
        bool hasYawAxis,
        bool hasYawFollow,
        bool hasYawLock,
        bool hasRetract,
        bool hasNeutral);

    /// Add a second independently addressable Gimbal Manager component.
    bool addSimulatedManager(
        uint8_t managerCompId,
        uint8_t gimbalDeviceId,
        bool hasRollAxis,
        bool hasPitchAxis,
        bool hasYawAxis,
        bool hasYawFollow,
        bool hasYawLock,
        bool hasRetract,
        bool hasNeutral);

    /// Test hook for simulating one device disconnecting and rediscovering.
    bool setSimulatedDeviceOnline(uint8_t gimbalDeviceId, bool online);
    bool setSimulatedDeviceOnline(uint8_t managerCompId, uint8_t gimbalDeviceId, bool online);

    /// Send periodic gimbal status messages (call from 1Hz tasks)
    void run1HzTasks();

    /// Handle all incoming MAVLink messages for gimbal.
    /// @return true if the message was handled by the gimbal
    bool handleMavlinkMessage(const mavlink_message_t &msg);

private:
    /// Handle MAV_CMD_SET_MESSAGE_INTERVAL for gimbal message IDs.
    /// @return true if the message interval was for a gimbal message
    bool _handleSetMessageInterval(const mavlink_command_long_t &request);

    /// Handle MAV_CMD_REQUEST_MESSAGE for GIMBAL_MANAGER_INFORMATION.
    /// @return true if the request was handled
    bool _handleRequestMessage(const mavlink_command_long_t &request);

    /// Handle MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW to control gimbal attitude.
    /// @return true if the command was handled
    bool _handleGimbalManagerPitchYaw(const mavlink_command_long_t &request,
                                      uint8_t sourceSysid, uint8_t sourceCompid);

    /// Handle GIMBAL_MANAGER_SET_ATTITUDE rate commands.
    bool _handleGimbalManagerSetAttitude(const mavlink_gimbal_manager_set_attitude_t &request,
                                         uint8_t sourceSysid, uint8_t sourceCompid);

    /// Handle MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE to configure gimbal manager.
    /// @return true if the command was handled
    bool _handleGimbalManagerConfigure(const mavlink_command_long_t &request,
                                       uint8_t sourceSysid, uint8_t sourceCompid);

    void _sendGimbalManagerStatus();
    void _sendGimbalDeviceAttitudeStatus();
    void _sendGimbalManagerInformation();
    void _sendCommandAck(uint16_t command, uint8_t result, uint8_t sourceCompId);

    static constexpr uint8_t kDefaultGimbalDeviceId = MAV_COMP_ID_GIMBAL;
    static constexpr int kDefaultIntervalUs = 1000000; // 1 Hz default

    MockLinkGimbal *_deviceForId(uint8_t gimbalDeviceId);
    MockLinkGimbal *_managerForTargetComponent(uint8_t targetComponent);

    MockLink *_mockLink = nullptr;
    uint8_t _gimbalDeviceId = kDefaultGimbalDeviceId;
    uint8_t _managerCompId = MAV_COMP_ID_AUTOPILOT1;
    uint8_t _deviceCompId = kDefaultGimbalDeviceId;
    QList<MockLinkGimbal*> _additionalDevices;
    QList<MockLinkGimbal*> _additionalManagers;
    int       _managerStatusIntervalUs = 0;          // 0 = disabled
    int       _deviceAttitudeStatusIntervalUs = 0;  // 0 = disabled by default
    qint64    _managerStatusLastSentMs = 0;
    qint64    _deviceAttitudeStatusLastSentMs = 0;

    // Simulated gimbal attitude (degrees)
    float     _roll = 0.0f;
    float     _pitch = 0.0f;
    float     _yaw = 0.0f;
    bool      _manualControl = false;  // true when under external control command
    bool      _deviceOnline = true;
    bool      _yawLock = false;
    bool      _retracted = false;
    bool      _rateControl = false;
    float     _pitchRateDegS = 0.0f;
    float     _yawRateDegS = 0.0f;
    /// Protects gimbal state and intervals from race conditions between:
    ///   - Main thread: _handleSetMessageInterval() and _handleGimbalManagerPitchYaw() modifying state
    ///   - Worker thread: run1HzTasks() reading intervals and auto-updating attitude every 1s
    ///   Race example: Main sets _manualControl=true, Worker reads false and overwrites manual commands
    QMutex    _stateMutex;

    // Gimbal manager configuration
    uint8_t   _gimbalManagerSysidPrimary = 0;    // System ID for primary control
    uint8_t   _gimbalManagerCompidPrimary = 0;   // Component ID for primary control
    uint8_t   _gimbalManagerSysidSecondary = 0;  // System ID for secondary control
    uint8_t   _gimbalManagerCompidSecondary = 0; // Component ID for secondary control

    // Gimbal capability flags
    bool _hasRollAxis = true;
    bool _hasPitchAxis = true;
    bool _hasYawAxis = true;
    bool _hasYawFollow = true;
    bool _hasYawLock = true;
    bool _hasRetract = true;
    bool _hasNeutral = true;
};
