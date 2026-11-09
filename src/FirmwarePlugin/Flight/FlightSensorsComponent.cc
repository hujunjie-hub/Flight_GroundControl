#include "FlightSensorsComponent.h"

#include "Vehicle.h"
#include "VehicleLinkManager.h"

namespace {
bool canCalibrateVehicle(Vehicle *vehicle)
{
    return vehicle && !vehicle->armed() && !vehicle->flying() &&
           vehicle->vehicleLinkManager() &&
           !vehicle->vehicleLinkManager()->communicationLost();
}

constexpr const char *kSensorsQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightSensors.qml";
} // namespace

FlightSensorsComponent::FlightSensorsComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot,
                                               QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownSensorsVehicleComponent, parent)
{
    if (vehicle) {
        connect(vehicle, &Vehicle::mavlinkMessageReceived,
                this, &FlightSensorsComponent::_handleCalibrationMessage);
        connect(vehicle, &Vehicle::mavCommandResult, this,
                [this](int, int component, int command, int result, int) {
            if (!_capturePending || component != MAV_COMP_ID_AUTOPILOT1 || command != MAV_CMD_USER_2)
                return;

            _capturePending = false;
            if (result != MAV_RESULT_ACCEPTED && _detail.isEmpty()) {
                _detail = result == MAV_RESULT_DENIED
                    ? tr("采样被拒绝。请按当前面的提示摆放，保持静止后重试。")
                    : tr("未能完成本次采样，请检查飞控连接后重试。");
            }
            emit calibrationStatusChanged();
        });
    }
    _statusTimeout.setSingleShot(true);
    _statusTimeout.setInterval(5000);
    connect(&_statusTimeout, &QTimer::timeout, this, [this] {
        _calibrationState = 3;
        _capturePending = false;
        _detail = tr("Calibration status was lost. Check the connection and try again.");
        emit calibrationStatusChanged();
    });
}

void FlightSensorsComponent::startCalibration(int kind)
{
    if (!canCalibrateVehicle(_vehicle) || _calibrationState == 1 || kind < 1 || kind > 5)
        return;

    const bool needsAccel = kind == 3 || kind == 4;
    if (needsAccel && !(_completedMask & (1 << 1)))
        return;

    _detail.clear();
    emit calibrationStatusChanged();
    _vehicle->sendCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_PREFLIGHT_CALIBRATION, true,
                          kind == 1, kind == 3, kind == 5, 0,
                          kind == 4 ? 2 : kind == 2 ? 1 : 0);
}

void FlightSensorsComponent::calibrationAction(int action)
{
    if (!canCalibrateVehicle(_vehicle) || _calibrationState != 1 || _capturePending)
        return;

    if ((action == 1 && _calibrationKind == 2 && _capturedFaces < 6) || action == 2) {
        _detail.clear();
        _capturePending = action == 1;
        emit calibrationStatusChanged();
        _vehicle->sendCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_USER_2, action != 1,
                              action == 1 ? _capturedFaces : 0, action);
    }
}

void FlightSensorsComponent::_handleCalibrationMessage(const mavlink_message_t &message)
{
    if (message.msgid != MAVLINK_MSG_ID_STATUSTEXT || message.compid != MAV_COMP_ID_AUTOPILOT1)
        return;

    mavlink_statustext_t packet{};
    mavlink_msg_statustext_decode(&message, &packet);
    const QString text = QString::fromLatin1(packet.text, sizeof(packet.text)).split(QChar(u'\0')).first();
    if (!text.startsWith(QStringLiteral("FCAL "))) {
        if (text == QStringLiteral("calibration cancelled"))
            _detail = tr("Calibration cancelled.");
        else if (text == QStringLiteral("orientation does not match this face"))
            _detail = tr("当前摆放方向与提示不符，请调整方向后重试。");
        else if (text == QStringLiteral("hold the face still for a moment"))
            _detail = tr("Hold the requested orientation still, then try again.");
        else
            return;
        emit calibrationStatusChanged();
        return;
    }

    const QStringList fields = text.split(u' ');
    if (fields.size() != 5)
        return;
    const int kind = fields[1].toInt();
    const int state = fields[2].toInt();
    const int faceMask = fields[3].toInt();
    const int progress = fields[4].toInt();
    if (kind < 1 || kind > 5 || state < 1 || state > 3 || faceMask < 0 || faceMask > 63 ||
        progress < 0 || progress > 100)
        return;

    if (_calibrationKind != kind || _calibrationState != state)
        _detail.clear();
    _calibrationKind = kind;
    _calibrationState = state;
    _capturedFaces = 0;
    for (int bit = 0; bit < 6; ++bit)
        _capturedFaces += (faceMask >> bit) & 1;
    _progress = progress;
    if (state == 1) {
        _statusTimeout.start();
    } else {
        _statusTimeout.stop();
        if (state == 2)
            _completedMask |= 1 << (kind - 1);
    }
    emit calibrationStatusChanged();
}

FlightSensorsComponent::~FlightSensorsComponent() = default;

QString FlightSensorsComponent::name() const
{
    return tr("传感器");
}

QString FlightSensorsComponent::description() const
{
    return tr("校准陀螺、加速度计、磁罗盘、水平姿态与地面气压。");
}

QString FlightSensorsComponent::iconResource() const
{
    return QStringLiteral("/qmlimages/SensorsComponentIcon.png");
}

bool FlightSensorsComponent::requiresSetup() const
{
    // Calibration is mandatory before a real flight, but the bench image also
    // connects without sensor hardware.  Report the per-sensor flags on the
    // page instead of blocking every connection with QGC's global "setup
    // remaining" dialog.
    return false;
}

bool FlightSensorsComponent::setupComplete() const
{
    return true;
}

QStringList FlightSensorsComponent::setupCompleteChangedTriggerList() const
{
    return {};
}

QUrl FlightSensorsComponent::setupSource() const
{
    return QUrl::fromUserInput(QString::fromLatin1(kSensorsQml));
}

QUrl FlightSensorsComponent::summaryQmlSource() const
{
    return QUrl();
}

