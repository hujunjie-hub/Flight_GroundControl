#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class FirmwarePlugin;
class Vehicle;

class VehicleSupports : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit VehicleSupports(Vehicle *vehicle);

    Q_PROPERTY(bool throttleModeCenterZero          READ throttleModeCenterZero         CONSTANT)
    Q_PROPERTY(bool negativeThrust                  READ negativeThrust                 CONSTANT)
    Q_PROPERTY(bool jsButton                        READ jsButton                       CONSTANT)
    Q_PROPERTY(bool radio                           READ radio                          CONSTANT)
    Q_PROPERTY(bool motorInterference               READ motorInterference              CONSTANT)
    Q_PROPERTY(bool smartRTL                        READ smartRTL                       CONSTANT)
    Q_PROPERTY(bool terrainFrame                    READ terrainFrame                   NOTIFY terrainFrameChanged)
    Q_PROPERTY(bool guidedMode                      READ guidedMode                     CONSTANT)
    Q_PROPERTY(bool pauseVehicle                    READ pauseVehicle                   CONSTANT)
    Q_PROPERTY(bool orbitMode                       READ orbitMode                      CONSTANT)
    Q_PROPERTY(bool roiMode                         READ roiMode                        CONSTANT)
    Q_PROPERTY(bool takeoffMissionCommand           READ takeoffMissionCommand          CONSTANT)
    Q_PROPERTY(bool guidedTakeoffWithAltitude       READ guidedTakeoffWithAltitude      CONSTANT)
    Q_PROPERTY(bool guidedTakeoffWithoutAltitude    READ guidedTakeoffWithoutAltitude   CONSTANT)
    Q_PROPERTY(bool changeHeading                   READ changeHeading                  CONSTANT)
    Q_PROPERTY(bool gotoLocation                    READ gotoLocation                   CONSTANT)
    Q_PROPERTY(bool changeAltitude                  READ changeAltitude                 CONSTANT)
    Q_PROPERTY(bool changeSpeed                     READ changeSpeed                    CONSTANT)
    Q_PROPERTY(bool changeLoiterRadius              READ changeLoiterRadius             CONSTANT)
    Q_PROPERTY(bool setHome                         READ setHome                        NOTIFY planActionsChanged)
    Q_PROPERTY(bool emergencyStop                   READ emergencyStop                  CONSTANT)
    Q_PROPERTY(bool landAbort                       READ landAbort                      CONSTANT)
    Q_PROPERTY(bool forceArm                        READ forceArm                       CONSTANT)
    Q_PROPERTY(bool estimatorOrigin                 READ estimatorOrigin                CONSTANT)
    Q_PROPERTY(bool firmwareUpgrade                 READ firmwareUpgrade                CONSTANT)
    Q_PROPERTY(bool cameraSection                   READ cameraSection                  CONSTANT)
    Q_PROPERTY(bool flightConfigurationLayout      READ flightConfigurationLayout     CONSTANT)
    Q_PROPERTY(double missionAltitudeSliderMaximum READ missionAltitudeSliderMaximum CONSTANT)
    Q_PROPERTY(bool missionRelativeAltitudeOnly     READ missionRelativeAltitudeOnly    CONSTANT)
    Q_PROPERTY(bool missionClear                    READ missionClear                   NOTIFY planActionsChanged)

    bool throttleModeCenterZero() const;
    bool negativeThrust() const;
    bool jsButton() const;
    bool radio() const;
    bool motorInterference() const;
    bool smartRTL() const;
    bool terrainFrame() const;
    bool guidedMode() const;
    bool pauseVehicle() const;
    bool orbitMode() const;
    bool roiMode() const;
    bool takeoffMissionCommand() const;
    bool guidedTakeoffWithAltitude() const;
    bool guidedTakeoffWithoutAltitude() const;
    bool changeHeading() const;
    bool gotoLocation() const;
    bool changeAltitude() const;
    bool changeSpeed() const;
    bool changeLoiterRadius() const;
    bool setHome() const;
    bool emergencyStop() const;
    bool landAbort() const;
    bool forceArm() const;
    bool estimatorOrigin() const;
    bool firmwareUpgrade() const;
    bool cameraSection() const;
    bool flightConfigurationLayout() const;
    double missionAltitudeSliderMaximum() const;
    bool missionRelativeAltitudeOnly() const;
    bool missionClear() const;

signals:
    void terrainFrameChanged();
    void planActionsChanged();

private:
    Vehicle *_vehicle = nullptr;
};
