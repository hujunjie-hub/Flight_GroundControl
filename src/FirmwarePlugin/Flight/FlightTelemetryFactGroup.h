#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QQueue>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>

#include "Fact.h"
#include "FactGroup.h"
#include "FactMetaData.h"
#include "QGCMAVLink.h"

/// Per-vehicle Flight FCU telemetry cache.
///
/// Values are informational only. They never grant command authority and are not
/// used by QGC to infer that a safety prerequisite has passed.
class FlightTelemetryFactGroup final : public FactGroup
{
    Q_OBJECT
public:
    Q_PROPERTY(QVariantList recentEvents READ recentEvents NOTIFY recentEventsChanged)
    Q_PROPERTY(QVariantList prearmChecks READ prearmChecks NOTIFY prearmChecksChanged)
    Q_PROPERTY(QVariantList sensorHealthStatuses READ sensorHealthStatuses NOTIFY sensorHealthStatusesChanged)
    Q_PROPERTY(QVariantList linkStatuses READ linkStatuses NOTIFY linkStatusesChanged)
    Q_PROPERTY(QVariantList resourceStatuses READ resourceStatuses NOTIFY resourceStatusesChanged)
    Q_PROPERTY(QVariantMap familyReceiveAgeMs READ familyReceiveAgeMs NOTIFY familyReceiveAgeMsChanged)
    /// Per-family freshness: true when the family has not produced an accepted frame for
    /// longer than familyStaleThresholdMs. Ageing uses _receiveClock (QElapsedTimer, monotonic);
    /// the wall clock is never consulted, so a system time change cannot flip freshness.
    Q_PROPERTY(QVariantMap familyStale READ familyStale NOTIFY familyReceiveAgeMsChanged)

    explicit FlightTelemetryFactGroup(QObject *parent = nullptr);
    ~FlightTelemetryFactGroup() override;
    QVariantList recentEvents() const;
    QVariantList prearmChecks() const;
    QVariantList sensorHealthStatuses() const;
    QVariantList linkStatuses() const;
    QVariantList resourceStatuses() const;
    QVariantMap familyReceiveAgeMs() const;
    QVariantMap familyStale() const;

    /// Staleness threshold in milliseconds (default 3000). Exposed so tests can drive STALE
    /// without waiting seconds; production keeps the default.
    static constexpr int kDefaultFamilyStaleThresholdMs = 3000;
    int familyStaleThresholdMs() const { return _familyStaleThresholdMs; }
    void setFamilyStaleThresholdMs(int ms);
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) override;

signals:
    void recentEventsChanged();
    void prearmChecksChanged();
    void sensorHealthStatusesChanged();
    void linkStatusesChanged();
    void resourceStatusesChanged();
    void familyReceiveAgeMsChanged();

private:
    Fact _opTimeBootUs{0, QStringLiteral("opTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _opBootId{0, QStringLiteral("opBootId"), FactMetaData::valueTypeUint64, this};
    Fact _opBootIdHex{0, QStringLiteral("opBootIdHex"), FactMetaData::valueTypeString, this};
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

    Fact _operationAvailable{0, QStringLiteral("operationAvailable"), FactMetaData::valueTypeBool, this};
    Fact _navigationAvailable{0, QStringLiteral("navigationAvailable"), FactMetaData::valueTypeBool, this};
    Fact _sensorHealthAvailable{0, QStringLiteral("sensorHealthAvailable"), FactMetaData::valueTypeBool, this};
    Fact _controlAvailable{0, QStringLiteral("controlAvailable"), FactMetaData::valueTypeBool, this};
    Fact _actuatorAvailable{0, QStringLiteral("actuatorAvailable"), FactMetaData::valueTypeBool, this};
    Fact _linkAvailable{0, QStringLiteral("linkAvailable"), FactMetaData::valueTypeBool, this};
    Fact _storageAvailable{0, QStringLiteral("storageAvailable"), FactMetaData::valueTypeBool, this};
    Fact _resourceAvailable{0, QStringLiteral("resourceAvailable"), FactMetaData::valueTypeBool, this};
    Fact _timeAvailable{0, QStringLiteral("timeAvailable"), FactMetaData::valueTypeBool, this};
    Fact _eventAvailable{0, QStringLiteral("eventAvailable"), FactMetaData::valueTypeBool, this};
    Fact _prearmAvailable{0, QStringLiteral("prearmAvailable"), FactMetaData::valueTypeBool, this};
    Fact _rcInputAvailable{0, QStringLiteral("rcInputAvailable"), FactMetaData::valueTypeBool, this};
    Fact _buildInfoAvailable{0, QStringLiteral("buildInfoAvailable"), FactMetaData::valueTypeBool, this};
    Fact _sensorTimeBootUs{0, QStringLiteral("sensorTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _sensorBootId{0, QStringLiteral("sensorBootId"), FactMetaData::valueTypeUint64, this};
    Fact _sensorProfileId{0, QStringLiteral("sensorProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _sensorInstanceId{0, QStringLiteral("sensorInstanceId"), FactMetaData::valueTypeUint32, this};
    Fact _sensorPolicyEpoch{0, QStringLiteral("sensorPolicyEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _sensorReasonCode{0, QStringLiteral("sensorReasonCode"), FactMetaData::valueTypeUint32, this};
    Fact _sensorGapCount{0, QStringLiteral("sensorGapCount"), FactMetaData::valueTypeUint32, this};
    Fact _sensorRejectCount{0, QStringLiteral("sensorRejectCount"), FactMetaData::valueTypeUint32, this};
    Fact _sensorHealthState{0, QStringLiteral("sensorHealthState"), FactMetaData::valueTypeUint16, this};
    Fact _sensorAction{0, QStringLiteral("sensorAction"), FactMetaData::valueTypeUint16, this};
    Fact _controlTimeBootUs{0, QStringLiteral("controlTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _controlBootId{0, QStringLiteral("controlBootId"), FactMetaData::valueTypeUint64, this};
    Fact _controlProfileId{0, QStringLiteral("controlProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _controlSequence{0, QStringLiteral("controlSequence"), FactMetaData::valueTypeUint32, this};
    Fact _controlReferenceId{0, QStringLiteral("controlReferenceId"), FactMetaData::valueTypeUint32, this};
    Fact _controlReferenceEpoch{0, QStringLiteral("controlReferenceEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _controllableDofMask{0, QStringLiteral("controllableDofMask"), FactMetaData::valueTypeUint32, this};
    Fact _degradedDofMask{0, QStringLiteral("degradedDofMask"), FactMetaData::valueTypeUint32, this};
    Fact _controlSolveTimeUs{0, QStringLiteral("controlSolveTimeUs"), FactMetaData::valueTypeUint32, this};
    Fact _controlReasonCode{0, QStringLiteral("controlReasonCode"), FactMetaData::valueTypeUint32, this};
    Fact _solverStatus{0, QStringLiteral("solverStatus"), FactMetaData::valueTypeUint16, this};
    Fact _actuatorTimeBootUs{0, QStringLiteral("actuatorTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _actuatorBootId{0, QStringLiteral("actuatorBootId"), FactMetaData::valueTypeUint64, this};
    Fact _actuatorProfileId{0, QStringLiteral("actuatorProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _actuatorControlSequence{0, QStringLiteral("actuatorControlSequence"), FactMetaData::valueTypeUint32, this};
    Fact _actuatorPermitEpoch{0, QStringLiteral("actuatorPermitEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _actuatorAvailableMask{0, QStringLiteral("actuatorAvailableMask"), FactMetaData::valueTypeUint32, this};
    Fact _actuatorSaturatedMask{0, QStringLiteral("actuatorSaturatedMask"), FactMetaData::valueTypeUint32, this};
    Fact _actuatorReasonCode{0, QStringLiteral("actuatorReasonCode"), FactMetaData::valueTypeUint32, this};
    Fact _actuatorOutputPolicyId{0, QStringLiteral("actuatorOutputPolicyId"), FactMetaData::valueTypeUint16, this};
    Fact _actuatorFeedbackCapability{
        0, QStringLiteral("actuatorFeedbackCapability"), FactMetaData::valueTypeUint16, this};
    Fact _linkTimeBootUs{0, QStringLiteral("linkTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _linkBootId{0, QStringLiteral("linkBootId"), FactMetaData::valueTypeUint64, this};
    Fact _linkProfileId{0, QStringLiteral("linkProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _linkInstanceId{0, QStringLiteral("linkInstanceId"), FactMetaData::valueTypeUint32, this};
    Fact _linkValidityMask{0, QStringLiteral("linkValidityMask"), FactMetaData::valueTypeUint32, this};
    Fact _linkPacketLossPpm{0, QStringLiteral("linkPacketLossPpm"), FactMetaData::valueTypeUint32, this};
    Fact _linkAgeUs{0, QStringLiteral("linkAgeUs"), FactMetaData::valueTypeUint32, this};
    Fact _linkReasonCode{0, QStringLiteral("linkReasonCode"), FactMetaData::valueTypeUint32, this};
    Fact _linkKind{0, QStringLiteral("linkKind"), FactMetaData::valueTypeUint16, this};
    Fact _linkState{0, QStringLiteral("linkState"), FactMetaData::valueTypeUint16, this};
    Fact _linkRssiDbm{0, QStringLiteral("linkRssiDbm"), FactMetaData::valueTypeInt16, this};
    Fact _linkQualityPermille{0, QStringLiteral("linkQualityPermille"), FactMetaData::valueTypeUint16, this};
    Fact _storageTimeBootUs{0, QStringLiteral("storageTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _storageBootId{0, QStringLiteral("storageBootId"), FactMetaData::valueTypeUint64, this};
    Fact _storageProfileId{0, QStringLiteral("storageProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _storageValidityMask{0, QStringLiteral("storageValidityMask"), FactMetaData::valueTypeUint32, this};
    Fact _storageSettingsGeneration{
        0, QStringLiteral("storageSettingsGeneration"), FactMetaData::valueTypeUint32, this};
    Fact _storageCriticalEventDrops{
        0, QStringLiteral("storageCriticalEventDrops"), FactMetaData::valueTypeUint32, this};
    Fact _storageLogDrops{0, QStringLiteral("storageLogDrops"), FactMetaData::valueTypeUint32, this};
    Fact _storageFreeKib{0, QStringLiteral("storageFreeKib"), FactMetaData::valueTypeUint32, this};
    Fact _storageReasonCode{0, QStringLiteral("storageReasonCode"), FactMetaData::valueTypeUint32, this};
    Fact _sdState{0, QStringLiteral("sdState"), FactMetaData::valueTypeUint16, this};
    Fact _norState{0, QStringLiteral("norState"), FactMetaData::valueTypeUint16, this};
    Fact _resourceTimeBootUs{0, QStringLiteral("resourceTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _resourceBootId{0, QStringLiteral("resourceBootId"), FactMetaData::valueTypeUint64, this};
    Fact _resourceProfileId{0, QStringLiteral("resourceProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _resourceValidityMask{0, QStringLiteral("resourceValidityMask"), FactMetaData::valueTypeUint32, this};
    Fact _resourceTaskId{0, QStringLiteral("resourceTaskId"), FactMetaData::valueTypeUint32, this};
    Fact _resourceWcetUs{0, QStringLiteral("resourceWcetUs"), FactMetaData::valueTypeUint32, this};
    Fact _resourceStackFreeBytes{0, QStringLiteral("resourceStackFreeBytes"), FactMetaData::valueTypeUint32, this};
    Fact _resourceDeadlineMissCount{
        0, QStringLiteral("resourceDeadlineMissCount"), FactMetaData::valueTypeUint32, this};
    Fact _resourceAxiFreeBytes{0, QStringLiteral("resourceAxiFreeBytes"), FactMetaData::valueTypeUint32, this};
    Fact _resourceCpuLoadPermille{0, QStringLiteral("resourceCpuLoadPermille"), FactMetaData::valueTypeUint16, this};
    Fact _timeStatusTimeBootUs{0, QStringLiteral("timeStatusTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _timeStatusBootId{0, QStringLiteral("timeStatusBootId"), FactMetaData::valueTypeUint64, this};
    Fact _timeStatusUnixTimeUs{0, QStringLiteral("timeStatusUnixTimeUs"), FactMetaData::valueTypeUint64, this};
    Fact _timeStatusProfileId{0, QStringLiteral("timeStatusProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _timeEpoch{0, QStringLiteral("timeEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _sourceMappingEpoch{0, QStringLiteral("sourceMappingEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _eventUncertaintyUs{0, QStringLiteral("eventUncertaintyUs"), FactMetaData::valueTypeUint32, this};
    Fact _utcUncertaintyUs{0, QStringLiteral("utcUncertaintyUs"), FactMetaData::valueTypeUint32, this};
    Fact _timeQuality{0, QStringLiteral("timeQuality"), FactMetaData::valueTypeUint16, this};
    Fact _timeDisciplineState{0, QStringLiteral("timeDisciplineState"), FactMetaData::valueTypeUint16, this};
    Fact _utcValid{0, QStringLiteral("utcValid"), FactMetaData::valueTypeBool, this};
    Fact _eventTimeBootUs{0, QStringLiteral("eventTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _eventBootId{0, QStringLiteral("eventBootId"), FactMetaData::valueTypeUint64, this};
    Fact _eventProfileId{0, QStringLiteral("eventProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _eventId{0, QStringLiteral("eventId"), FactMetaData::valueTypeUint32, this};
    Fact _eventReasonCode{0, QStringLiteral("eventReasonCode"), FactMetaData::valueTypeUint32, this};
    Fact _eventAuthorityEpoch{0, QStringLiteral("eventAuthorityEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _eventDirectiveEpoch{0, QStringLiteral("eventDirectiveEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _eventPermitEpoch{0, QStringLiteral("eventPermitEpoch"), FactMetaData::valueTypeUint32, this};
    Fact _eventSourceId{0, QStringLiteral("eventSourceId"), FactMetaData::valueTypeUint16, this};
    Fact _eventReasonNamespace{0, QStringLiteral("eventReasonNamespace"), FactMetaData::valueTypeUint16, this};
    Fact _eventSeverity{0, QStringLiteral("eventSeverity"), FactMetaData::valueTypeUint8, this};
    Fact _eventSequence{0, QStringLiteral("eventSequence"), FactMetaData::valueTypeUint32, this};
    Fact _rcTimeBootUs{0, QStringLiteral("rcTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _rcBootId{0, QStringLiteral("rcBootId"), FactMetaData::valueTypeUint64, this};
    Fact _rcProfileId{0, QStringLiteral("rcProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _rcValidAxesMask{0, QStringLiteral("rcValidAxesMask"), FactMetaData::valueTypeUint32, this};
    Fact _rcRollNorm{0, QStringLiteral("rcRollNorm"), FactMetaData::valueTypeFloat, this};
    Fact _rcPitchNorm{0, QStringLiteral("rcPitchNorm"), FactMetaData::valueTypeFloat, this};
    Fact _rcYawNorm{0, QStringLiteral("rcYawNorm"), FactMetaData::valueTypeFloat, this};
    Fact _rcThrottleNorm{0, QStringLiteral("rcThrottleNorm"), FactMetaData::valueTypeFloat, this};
    Fact _rcAgeUs{0, QStringLiteral("rcAgeUs"), FactMetaData::valueTypeUint32, this};
    Fact _rcReceiverFailsafe{0, QStringLiteral("rcReceiverFailsafe"), FactMetaData::valueTypeBool, this};
    Fact _rcLinkState{0, QStringLiteral("rcLinkState"), FactMetaData::valueTypeUint8, this};
    Fact _buildTimeBootUs{0, QStringLiteral("buildTimeBootUs"), FactMetaData::valueTypeUint64, this};
    Fact _buildBootId{0, QStringLiteral("buildBootId"), FactMetaData::valueTypeUint64, this};
    Fact _buildProfileId{0, QStringLiteral("buildProfileId"), FactMetaData::valueTypeUint32, this};
    Fact _buildProductRevision{0, QStringLiteral("buildProductRevision"), FactMetaData::valueTypeUint32, this};
    Fact _buildPortRevision{0, QStringLiteral("buildPortRevision"), FactMetaData::valueTypeUint32, this};
    Fact _buildSchemaMajor{0, QStringLiteral("buildSchemaMajor"), FactMetaData::valueTypeUint16, this};
    Fact _buildSchemaMinor{0, QStringLiteral("buildSchemaMinor"), FactMetaData::valueTypeUint16, this};
    Fact _buildDigestKind{0, QStringLiteral("buildDigestKind"), FactMetaData::valueTypeUint8, this};
    Fact _firmwareSha256{0, QStringLiteral("firmwareSha256"), FactMetaData::valueTypeString, this};
    Fact _coreParametersSha256{0, QStringLiteral("coreParametersSha256"), FactMetaData::valueTypeString, this};
    Fact _profileSha256{0, QStringLiteral("profileSha256"), FactMetaData::valueTypeString, this};
    Fact _abiSha256{0, QStringLiteral("abiSha256"), FactMetaData::valueTypeString, this};
    Fact _sourceCommit{0, QStringLiteral("sourceCommit"), FactMetaData::valueTypeString, this};
    Fact _sourceCommitValid{0, QStringLiteral("sourceCommitValid"), FactMetaData::valueTypeBool, this};

    bool _acceptTelemetryEpoch(quint64 bootId);
    bool _acceptTelemetryMessage(quint64 bootId, quint64 timeBootUs, quint64 &lastTimeBootUs);
    bool _acceptTelemetryMessage(quint64 bootId, quint64 timeBootUs, quint32 instanceId,
                                 QHash<quint32, quint64> &lastTimeByInstance);
    bool _activateBoot(quint64 bootId);
    bool _acceptTelemetryProfile(quint64 bootId, quint32 profileId) const;
    void _commitTelemetryProfile(quint32 profileId);
    void _resetMessageTimestamps();
    void _resetTelemetryAvailability();
    void _markFamilyReceived(const QString &family);
    /// Recomputes the nearest stale deadline and arms the single staleness timer.
    void _rescheduleStaleDeadline();
    /// Emits familyReceiveAgeMsChanged only when the set of stale families actually changed.
    void _notifyIfStaleSetChanged();

    QElapsedTimer _receiveClock;
    int _familyStaleThresholdMs = kDefaultFamilyStaleThresholdMs;
    /// One timer serves every family (never one timer per family): only the nearest deadline is
    /// armed, and on expiry all families are re-evaluated. Single-shot, so an idle vehicle arms
    /// nothing and there is no polling.
    QTimer _staleTimer;
    QSet<QString> _lastNotifiedStaleFamilies;
    QHash<QString, qint64> _lastReceiveElapsedMs;
    quint64 _activeBootId = 0;
    quint32 _activeProfileId = 0;
    // Reject delayed packets from recently retired FCU boot sessions.
    QSet<quint64> _retiredBootIds;
    QQueue<quint64> _retiredBootIdOrder;
    static constexpr int _retiredBootIdCapacity = 8;
    quint64 _lastOperationTimeBootUs = 0;
    quint64 _lastNavigationTimeBootUs = 0;
    QHash<quint32, quint64> _lastSensorTimeByInstance;
    quint64 _lastControlTimeBootUs = 0;
    quint64 _lastActuatorTimeBootUs = 0;
    QHash<quint32, quint64> _lastLinkTimeByInstance;
    quint64 _lastStorageTimeBootUs = 0;
    QHash<quint32, quint64> _lastResourceTimeByTask;
    quint64 _lastTimeStatusTimeBootUs = 0;
    quint64 _lastEventTimeBootUs = 0;
    QSet<quint32> _eventIdsAtLastTimestamp;
    QQueue<quint32> _eventIdsAtLastTimestampOrder;
    QVariantList _recentEvents;
    QMap<quint8, QVariantMap> _prearmChecksById;
    QMap<quint32, QVariantMap> _sensorHealthByInstance;
    QMap<quint32, QVariantMap> _linkStatusesByInstance;
    QMap<quint32, QVariantMap> _resourceStatusesByTask;
    static constexpr int _resourceTaskCapacity = 64;
    quint32 _prearmCheckEpochValue = 0;
    bool _hasPrearmCheckEpoch = false;
    static constexpr int _recentEventHistoryCapacity = 64;
    quint64 _lastPrearmTimeBootUs = 0;
    QHash<quint32, quint64> _lastPrearmTimeByCheckId;
    quint64 _lastRcInputTimeBootUs = 0;
    quint64 _lastBuildInfoTimeBootUs = 0;
    quint8 _lastBuildInfoDigestMask = 0;
    bool _hasBuildSnapshotMetadata = false;
    quint32 _buildSnapshotProfileId = 0;
    quint32 _buildSnapshotProductRevision = 0;
    quint32 _buildSnapshotPortRevision = 0;
    quint16 _buildSnapshotSchemaMajor = 0;
    quint16 _buildSnapshotSchemaMinor = 0;
    quint8 _buildSnapshotSourceCommitValid = 0;
    QByteArray _buildSnapshotSourceCommit;
    quint32 _eventSequenceValue = 0;
};
