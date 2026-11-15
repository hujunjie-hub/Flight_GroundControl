#pragma once

#include <QtCore/QElapsedTimer>

#include "FactGroup.h"
#include "MAVLinkEnums.h"

class GimbalController;

class Gimbal : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact     *absoluteRoll           READ absoluteRoll               CONSTANT)
    Q_PROPERTY(Fact     *absolutePitch          READ absolutePitch              CONSTANT)
    Q_PROPERTY(Fact     *bodyYaw                READ bodyYaw                    CONSTANT)
    Q_PROPERTY(Fact     *absoluteYaw            READ absoluteYaw                CONSTANT)
    Q_PROPERTY(Fact     *deviceId               READ deviceId                   CONSTANT)
    Q_PROPERTY(Fact     *managerCompid          READ managerCompid              CONSTANT)
    Q_PROPERTY(float    pitchRate               READ pitchRate                  NOTIFY pitchRateChanged)
    Q_PROPERTY(float    yawRate                 READ yawRate                    NOTIFY yawRateChanged)
    Q_PROPERTY(bool     yawLock                 READ yawLock                    NOTIFY yawLockChanged)
    Q_PROPERTY(bool     retracted               READ retracted                  NOTIFY retractedChanged)
    Q_PROPERTY(bool     gimbalHaveControl       READ gimbalHaveControl          NOTIFY gimbalHaveControlChanged)
    Q_PROPERTY(bool     gimbalOthersHaveControl READ gimbalOthersHaveControl    NOTIFY gimbalOthersHaveControlChanged)
    Q_PROPERTY(bool     ownershipStatusValid    READ ownershipStatusValid       NOTIFY ownershipStatusChanged)
    Q_PROPERTY(bool     supportsPitchAxis       READ supportsPitchAxis          NOTIFY capabilityFlagsChanged)
    Q_PROPERTY(bool     supportsYawAxis         READ supportsYawAxis             NOTIFY capabilityFlagsChanged)
    Q_PROPERTY(bool     supportsRetract         READ supportsRetract             NOTIFY capabilityFlagsChanged)
    Q_PROPERTY(bool     supportsYawLock         READ supportsYawLock             NOTIFY capabilityFlagsChanged)
    Q_PROPERTY(qint64   managerStatusAgeMs       READ managerStatusAgeMs          NOTIFY managerStatusFreshnessChanged)
    Q_PROPERTY(qint64   attitudeStatusAgeMs      READ attitudeStatusAgeMs         NOTIFY attitudeStatusFreshnessChanged)

    friend class GimbalController;

public:
    Gimbal(GimbalController *parent);
    Gimbal(const Gimbal &other);
    const Gimbal &operator=(const Gimbal &other);
    ~Gimbal();

    Fact *absoluteRoll() { return &_absoluteRollFact; }
    Fact *absolutePitch() { return &_absolutePitchFact; }
    Fact *bodyYaw() { return &_bodyYawFact; }
    Fact *absoluteYaw() { return &_absoluteYawFact; }
    Fact *deviceId() { return &_deviceIdFact; }
    Fact *managerCompid() { return &_managerCompidFact; }

    float pitchRate() const { return _pitchRate; }
    float yawRate() const { return _yawRate; }
    bool yawLock() const { return _yawLock; }
    bool retracted() const { return _retracted; }
    bool gimbalHaveControl() const { return _haveControl; }
    bool gimbalOthersHaveControl() const { return _othersHaveControl; }
    bool ownershipStatusValid() const { return _ownershipStatusValid; }

    static constexpr qint64 kManagerStatusFreshnessThresholdMs = 6500;
    static constexpr qint64 kAttitudeStatusFreshnessThresholdMs = 3500;
    qint64 managerStatusAgeMs() const
    {
        return _managerStatusValid && _managerStatusReceiveClock.isValid()
                   ? _managerStatusReceiveClock.elapsed() : -1;
    }
    qint64 attitudeStatusAgeMs() const
    {
        return _attitudeStatusValid && _attitudeStatusReceiveClock.isValid()
                   ? _attitudeStatusReceiveClock.elapsed() : -1;
    }
    bool managerStatusFresh() const
    {
        const qint64 age = managerStatusAgeMs();
        return age >= 0 && age <= kManagerStatusFreshnessThresholdMs;
    }
    bool attitudeStatusFresh() const
    {
        const qint64 age = attitudeStatusAgeMs();
        return age >= 0 && age <= kAttitudeStatusFreshnessThresholdMs;
    }
    void invalidateStatus()
    {
        _managerStatusValid = false;
        _attitudeStatusValid = false;
        setOwnershipStatusValid(false);
        emit managerStatusFreshnessChanged();
        emit attitudeStatusFreshnessChanged();
    }
    void setOwnershipStatusValid(bool valid)
    {
        if (_ownershipStatusValid != valid) {
            _ownershipStatusValid = valid;
            emit ownershipStatusChanged();
        }
    }
    void markManagerStatusReceived()
    {
        _managerStatusValid = true;
        if (_managerStatusReceiveClock.isValid()) {
            _managerStatusReceiveClock.restart();
        } else {
            _managerStatusReceiveClock.start();
        }
        emit managerStatusFreshnessChanged();
    }
    void markAttitudeStatusReceived()
    {
        _attitudeStatusValid = true;
        if (_attitudeStatusReceiveClock.isValid()) {
            _attitudeStatusReceiveClock.restart();
        } else {
            _attitudeStatusReceiveClock.start();
        }
        emit attitudeStatusFreshnessChanged();
    }

    void setAbsoluteRoll(float absRoll) { absoluteRoll()->setRawValue(absRoll); }
    void setAbsolutePitch(float absPitch) { absolutePitch()->setRawValue(absPitch); }
    void setBodyYaw(float yaw) { bodyYaw()->setRawValue(yaw); }
    void setAbsoluteYaw(float absYaw) { absoluteYaw()->setRawValue(absYaw); }
    void setDeviceId(uint id) { deviceId()->setRawValue(id); }
    void setManagerCompid(uint id) { managerCompid()->setRawValue(id); }

    void setPitchRate(float pitchRate) { if (pitchRate != _pitchRate) { _pitchRate = pitchRate; emit pitchRateChanged(); } }
    void setYawRate(float yawRate) { if (yawRate != _yawRate) { _yawRate = yawRate; emit yawRateChanged(); } }
    void setYawLock(bool yawLock) { if (yawLock != _yawLock) { _yawLock = yawLock; emit yawLockChanged(); } }
    void setRetracted(bool retracted) { if (retracted != _retracted) { _retracted = retracted; emit retractedChanged(); } }
    void setGimbalHaveControl(bool set) { if (set != _haveControl) { _haveControl = set; emit gimbalHaveControlChanged(); } }
    void setGimbalOthersHaveControl(bool set) { if (set != _othersHaveControl) { _othersHaveControl = set; emit gimbalOthersHaveControlChanged(); } }

    void setCapabilityFlags(uint32_t flags);
    bool supportsPitchAxis() const { return (_capabilityFlags & GIMBAL_MANAGER_CAP_FLAGS_HAS_PITCH_AXIS) != 0; }
    bool supportsYawAxis() const { return (_capabilityFlags & GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_AXIS) != 0; }
    bool supportsRetract() const { return (_capabilityFlags & GIMBAL_MANAGER_CAP_FLAGS_HAS_RETRACT) != 0; }
    bool supportsYawLock() const { return (_capabilityFlags & GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_LOCK) != 0; }

signals:
    void pitchRateChanged();
    void yawRateChanged();
    void yawLockChanged();
    void retractedChanged();
    void gimbalHaveControlChanged();
    void gimbalOthersHaveControlChanged();
    void capabilityFlagsChanged();
    void ownershipStatusChanged();
    void managerStatusFreshnessChanged();
    void attitudeStatusFreshnessChanged();

private:
    void _initFacts();

    unsigned _requestInformationRetries = 3;
    unsigned _requestStatusRetries = 6;
    unsigned _requestAttitudeRetries = 3;
    bool _receivedGimbalManagerInformation = false;
    bool _receivedGimbalManagerStatus = false;
    bool _receivedGimbalDeviceAttitudeStatus = false;
    bool _isComplete = false;
    bool _neutral = false;
    uint32_t _capabilityFlags = 0; // GIMBAL_MANAGER_CAP_FLAGS
    QElapsedTimer _managerStatusReceiveClock;
    QElapsedTimer _attitudeStatusReceiveClock;
    bool _managerStatusValid = false;
    bool _attitudeStatusValid = false;
    bool _ownershipStatusValid = false;

    Fact _absoluteRollFact = Fact(0, QStringLiteral("gimbalRoll"), FactMetaData::valueTypeFloat);
    Fact _absolutePitchFact = Fact(0, QStringLiteral("gimbalPitch"), FactMetaData::valueTypeFloat);
    Fact _bodyYawFact = Fact(0, QStringLiteral("gimbalYaw"), FactMetaData::valueTypeFloat);
    Fact _absoluteYawFact = Fact(0, QStringLiteral("gimbalAzimuth"), FactMetaData::valueTypeFloat);
    Fact _deviceIdFact = Fact(0, QStringLiteral("deviceId"), FactMetaData::valueTypeUint8); ///< Component ID of gimbal device (or 1-6 for non-MAVLink gimbal)
    Fact _managerCompidFact = Fact(0, QStringLiteral("managerCompid"), FactMetaData::valueTypeUint8);

    float _pitchRate = 0.f;
    float _yawRate = 0.f;
    bool _yawLock = false;
    bool _retracted = false;
    bool _haveControl = false;
    bool _othersHaveControl = false;
};
