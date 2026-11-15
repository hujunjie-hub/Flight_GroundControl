#include "QmlObjectListModel.h"
#include "ParameterEditorController.h"
#include "AppMessages.h"
#include "ParameterManager.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "VehicleLinkManager.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"

#include <algorithm>
#include <cmath>

QGC_LOGGING_CATEGORY(ParameterEditorControllerLog, "QMLControls.ParameterEditorController")

ParameterTableModel::ParameterTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{

}

ParameterTableModel::~ParameterTableModel()
{

}

int ParameterTableModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _tableData.count();
}

int ParameterTableModel::columnCount(const QModelIndex & /*parent*/) const
{
    return _tableViewColCount;
}

QVariant ParameterTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() < 0 || index.row() >= _tableData.count()) {
        return QVariant();
    }
    if (index.column() < 0 || index.column() >= _tableViewColCount) {
        return QVariant();
    }

    switch (role) {
        case Qt::DisplayRole:
            return QVariant::fromValue(_tableData[index.row()][index.column()]);
        case FactRole:
            return QVariant::fromValue(_tableData[index.row()][ValueColumn]);
        default:
            return QVariant();
    }
}

QVariant ParameterTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
    case FavColumn:         return tr("Fav");
    case NameColumn:        return tr("Name");
    case ValueColumn:       return tr("Value");
    case UnitsColumn:       return tr("Units");
    case DescriptionColumn: return tr("Description");
    default:                return QVariant();
    }
}

QHash<int, QByteArray> ParameterTableModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {FactRole, "fact"}
    };
}

void ParameterTableModel::clear()
{
    beginReset();
    _tableData.clear();
    endReset();
}

void ParameterTableModel::append(Fact* fact)
{
    insert(rowCount(), fact);
}

void ParameterTableModel::insert(int row, Fact* fact)
{
    if (row < 0 || row > rowCount()) {
        qWarning() << "Invalid row row:rowCount" << row << rowCount() << Q_FUNC_INFO;
        row = qMax(qMin(row, rowCount()), 0);
    }

    ColumnData colData(_tableViewColCount, QString());
    colData[FavColumn] = QString();
    colData[NameColumn] = fact->name();
    colData[ValueColumn] = QVariant::fromValue(fact);
    colData[UnitsColumn] = fact->cookedUnits();
    colData[DescriptionColumn] = fact->shortDescription();

    if (!_isResetting()) {
        beginInsertRows(QModelIndex(), row, row);
    }
    _tableData.insert(row, colData);
    if (!_isResetting()) {
        endInsertRows();
        emit rowCountChanged(rowCount());
    }
}

void ParameterTableModel::beginReset()
{
    _resetNestingCount++;

    if (_resetNestingCount == 1) {
        beginResetModel();
    }
}

void ParameterTableModel::endReset()
{
    if (_resetNestingCount == 0) {
        qWarning() << "ParameterTableModel::endReset called without prior beginReset";
        return;
    }
    _resetNestingCount--;
    if (_resetNestingCount == 0) {
        endResetModel();
        emit rowCountChanged(rowCount());
    }
}

Fact* ParameterTableModel::factAt(int row) const
{
    if (row < 0 || row >= _tableData.count()) {
        qWarning() << "Invalid row row:rowCount" << row << _tableData.count() << Q_FUNC_INFO;
        return nullptr;
    }

    return _tableData[row][ValueColumn].value<Fact*>();
}


ParameterEditorGroup::ParameterEditorGroup(QObject* parent)
    : QObject(parent)
{

}

ParameterEditorController::ParameterEditorController(QObject *parent)
    : FactPanelController(parent)
    , _parameterMgr(_vehicle->parameterManager())
{
    // qCDebug(ParameterEditorControllerLog) << Q_FUNC_INFO << this;

    _buildLists();

    _searchTimer.setSingleShot(true);
    _searchTimer.setInterval(300);

    connect(this, &ParameterEditorController::currentCategoryChanged,   this, &ParameterEditorController::_currentCategoryChanged);
    connect(this, &ParameterEditorController::currentGroupChanged,      this, &ParameterEditorController::_currentGroupChanged);
    connect(this, &ParameterEditorController::searchTextChanged,        this, &ParameterEditorController::_searchTextChanged);
    connect(this, &ParameterEditorController::categoryFilterChanged,    this, &ParameterEditorController::_searchTextChanged);
    connect(this, &ParameterEditorController::showModifiedOnlyChanged,  this, &ParameterEditorController::_searchTextChanged);
    connect(this, &ParameterEditorController::showFavoritesOnlyChanged, this, &ParameterEditorController::_searchTextChanged);
    connect(this, &ParameterEditorController::hideReadOnlyChanged,     this, &ParameterEditorController::_hideReadOnlyChanged);
    connect(&_searchTimer, &QTimer::timeout,                            this, &ParameterEditorController::_performSearch);
    connect(_parameterMgr, &ParameterManager::factAdded,                this, &ParameterEditorController::_factAdded);
    connect(_parameterMgr, &ParameterManager::pendingWritesChanged, this, [this](bool) {
        emit tuningStateChanged();
    });
    connect(_parameterMgr, &ParameterManager::parametersReadyChanged, this, [this](bool) {
        emit tuningStateChanged();
    });
    connect(_parameterMgr, &ParameterManager::missingParametersChanged, this, [this](bool) {
        emit tuningStateChanged();
    });
    connect(_parameterMgr, &ParameterManager::parameterDownloadSkippedChanged, this, [this]() {
        emit tuningStateChanged();
    });
    connect(_parameterMgr, &ParameterManager::_paramSetSuccess,
            this, &ParameterEditorController::_parameterWriteSucceeded);
    connect(_parameterMgr, &ParameterManager::_paramSetFailure,
            this, &ParameterEditorController::_parameterWriteFailed);
    connect(_parameterMgr, &ParameterManager::_paramRequestReadSuccess,
            this, &ParameterEditorController::_parameterReadSucceeded);
    connect(_parameterMgr, &ParameterManager::_paramRequestReadFailure,
            this, &ParameterEditorController::_parameterReadFailed);

    if (_vehicle) {
        connect(_vehicle, &Vehicle::armedChanged, this, [this](bool) { emit tuningStateChanged(); });
        connect(_vehicle, &Vehicle::flyingChanged, this, [this](bool) { emit tuningStateChanged(); });
        connect(_vehicle, &Vehicle::firmwareVersionChanged, this, [this]() { emit tuningStateChanged(); });
        connect(_vehicle, &Vehicle::gitHashChanged, this, [this](const QString &) { emit tuningStateChanged(); });
        if (_vehicle->vehicleLinkManager()) {
            connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged,
                    this, [this](bool) { emit tuningStateChanged(); });
        }
    }
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged,
            this, [this](Vehicle *) { emit tuningStateChanged(); });

    _loadFavorites();

    ParameterEditorCategory* category = _categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr;
    setCurrentCategory(category);
}

ParameterEditorController::~ParameterEditorController()
{
    // qCDebug(ParameterEditorControllerLog) << Q_FUNC_INFO << this;
}

QString ParameterEditorController::_selectedParameterKey() const
{
    return _selectedFact
        ? QStringLiteral("%1:%2").arg(_selectedFact->componentId()).arg(_selectedFact->name())
        : QString();
}

bool ParameterEditorController::_flightParameterCenterSafeMode() const
{
    return _vehicle && _vehicle->firmwarePlugin() &&
           _vehicle->firmwarePlugin()->flightConfigurationLayout();
}

bool ParameterEditorController::setSelectedModelProfileId(const QString &profileId)
{
    if (!_flightParameterCenterSafeMode() || !_vehicle ||
        _tuningBusy || _readbackPending || _writeFailureNeedsReadback ||
        (_parameterMgr && _parameterMgr->pendingWrites())) {
        return false;
    }

    const auto rejectProfile = [this](const QString &reason) {
        if (_selectedFact) {
            disconnect(_selectedFact, &Fact::vehicleUpdated,
                       this, &ParameterEditorController::_selectedFactVehicleUpdated);
        }
        _selectedFact = nullptr;
        _confirmedValue.clear();
        _previousValue.clear();
        _pendingOriginalValue.clear();
        _pendingParameterName.clear();
        _pendingComponentId = -1;
        _previousValues.clear();
        _hasPreviousValue = false;
        _selectedModelProfileId.clear();
        _selectedModelParameterNames.clear();
        _selectedModelParameterDomains.clear();
        _tuningStatus = tr("机型配置校验失败，参数编辑已锁定");
        _tuningError = reason;
        if (_parameterMgr) {
            _buildLists();
            if (!_searchText.isEmpty() || _showModifiedOnly || _showFavoritesOnly) {
                _performSearch();
            }
        }
        emit selectedFactChanged();
        emit tuningStateChanged();
        return false;
    };

    QVariantMap selectedProfile;
    QStringList parameterNames;
    QStringList parameterDomains;

    if (!profileId.isEmpty()) {
        if (!_parameterMgr || !_parameterMgr->parametersReady() ||
            _parameterMgr->missingParameters() || _parameterMgr->parameterDownloadSkipped()) {
            return rejectProfile(tr("飞控参数列表尚未完整就绪，不能绑定机型参数清单。"));
        }

        for (const QVariant &value : _vehicle->vehicleModelProfiles()) {
            const QVariantMap candidate = value.toMap();
            if (candidate.value(QStringLiteral("id")).toString() != profileId) {
                continue;
            }
            if (candidate.value(QStringLiteral("status")).toString() != QStringLiteral("SIM_ONLY") ||
                candidate.value(QStringLiteral("hardwareEligible")).toBool() ||
                candidate.value(QStringLiteral("parameterWriteScope")).toString() != QStringLiteral("SIL_ONLY")) {
                return rejectProfile(tr("机型配置不满足 SIL-only 写入资格。"));
            }
            selectedProfile = candidate;
            break;
        }
        if (selectedProfile.isEmpty()) {
            return rejectProfile(tr("当前飞行器未注册所选机型配置。"));
        }

        static const QSet<QString> supportedDomains {
            QStringLiteral("VEHICLE_MODEL"),
            QStringLiteral("FMS"),
            QStringLiteral("CONTROL"),
            QStringLiteral("INS"),
            QStringLiteral("PLANT"),
            QStringLiteral("ACTUATOR"),
            QStringLiteral("SIMULATION")
        };

        QSet<QString> uniqueParameterNames;
        for (const QVariant &value : selectedProfile.value(QStringLiteral("parameterNames")).toList()) {
            const QString name = value.toString().trimmed();
            if (name.isEmpty() || uniqueParameterNames.contains(name)) {
                qCWarning(ParameterEditorControllerLog)
                    << "Rejecting Flight model profile with an empty or duplicate parameter name:"
                    << profileId << name;
                return rejectProfile(tr("机型参数清单包含空参数名或重复参数名。"));
            }
            uniqueParameterNames.insert(name);
            parameterNames.append(name);
        }

        QSet<QString> uniqueParameterDomains;
        for (const QVariant &value : selectedProfile.value(QStringLiteral("parameterDomains")).toList()) {
            const QString domain = value.toString().trimmed();
            if (domain.isEmpty() || !supportedDomains.contains(domain) ||
                uniqueParameterDomains.contains(domain)) {
                qCWarning(ParameterEditorControllerLog)
                    << "Rejecting Flight model profile with an empty, duplicate or unsupported domain:"
                    << profileId << domain;
                return rejectProfile(tr("机型参数清单包含未登记或重复的参数域。"));
            }
            uniqueParameterDomains.insert(domain);
            parameterDomains.append(domain);
        }

        if (parameterNames.isEmpty() || parameterDomains.isEmpty()) {
            return rejectProfile(tr("机型参数清单为空，不能启用参数编辑。"));
        }

        // A profile is valid only when every registered parameter is present in the
        // downloaded active-vehicle list and its live Fact metadata belongs to one
        // of the profile's declared domains. This catches stale or mismatched catalogs.
        QSet<QString> resolvedParameterNames;
        for (const int componentId : _parameterMgr->componentIds()) {
            for (const QString &liveName : _parameterMgr->parameterNames(componentId)) {
                if (!uniqueParameterNames.contains(liveName)) {
                    continue;
                }
                Fact *const liveFact = _parameterMgr->getParameter(componentId, liveName);
                if (!liveFact || !uniqueParameterDomains.contains(liveFact->category())) {
                    qCWarning(ParameterEditorControllerLog)
                        << "Rejecting Flight model profile because a live parameter has an undeclared domain:"
                        << profileId << liveName << (liveFact ? liveFact->category() : QString());
                    return rejectProfile(tr("飞控参数 %1 的实际参数域与机型注册清单不一致。").arg(liveName));
                }
                resolvedParameterNames.insert(liveName);
            }
        }

        if (resolvedParameterNames != uniqueParameterNames) {
            const QStringList missingNames = (uniqueParameterNames - resolvedParameterNames).values();
            qCWarning(ParameterEditorControllerLog)
                << "Rejecting Flight model profile because one or more registered parameters are missing:"
                << profileId << missingNames;
            return rejectProfile(tr("飞控缺少机型注册清单中的参数：%1").arg(missingNames.join(QStringLiteral(", "))));
        }
    }

    if (_selectedModelProfileId == profileId &&
        _selectedModelParameterNames == parameterNames &&
        _selectedModelParameterDomains == parameterDomains) {
        return true;
    }

    if (_selectedFact) {
        disconnect(_selectedFact, &Fact::vehicleUpdated,
                   this, &ParameterEditorController::_selectedFactVehicleUpdated);
    }
    _selectedFact = nullptr;
    _confirmedValue.clear();
    _previousValue.clear();
    _pendingOriginalValue.clear();
    _previousValues.clear();
    _hasPreviousValue = false;
    _selectedModelProfileId = profileId;
    _selectedModelParameterNames = parameterNames;
    _selectedModelParameterDomains = parameterDomains;
    _tuningStatus = profileId.isEmpty()
        ? tr("未选择机型调参配置")
        : tr("已绑定机型配置：%1").arg(selectedProfile.value(QStringLiteral("displayName")).toString());
    _tuningError.clear();

    if (_parameterMgr) {
        _buildLists();
        if (!_searchText.isEmpty() || _showModifiedOnly || _showFavoritesOnly) {
            _performSearch();
        }
    }
    emit selectedFactChanged();
    emit tuningStateChanged();
    return true;
}

bool ParameterEditorController::tuningBusy() const
{
    return _tuningBusy || _readbackPending || _writeFailureNeedsReadback ||
           (_parameterMgr && _parameterMgr->pendingWrites());
}

bool ParameterEditorController::modelProfileBound() const
{
    return !_flightParameterCenterSafeMode() ||
           (!_selectedModelProfileId.isEmpty() &&
            !_selectedModelParameterNames.isEmpty() &&
            !_selectedModelParameterDomains.isEmpty());
}

bool ParameterEditorController::_profileAllowsSelectedParameter() const
{
    if (!_vehicle || !_selectedFact || !_parameterMgr || _selectedModelProfileId.isEmpty() ||
        !_selectedModelParameterNames.contains(_selectedFact->name()) ||
        !_selectedModelParameterDomains.contains(_selectedFact->category())) {
        return false;
    }

    QVariantMap selectedProfile;
    for (const QVariant &value : _vehicle->vehicleModelProfiles()) {
        const QVariantMap candidate = value.toMap();
        if (candidate.value(QStringLiteral("id")).toString() == _selectedModelProfileId) {
            selectedProfile = candidate;
            break;
        }
    }
    if (selectedProfile.isEmpty() ||
        selectedProfile.value(QStringLiteral("status")).toString() != QStringLiteral("SIM_ONLY") ||
        selectedProfile.value(QStringLiteral("hardwareEligible")).toBool() ||
        selectedProfile.value(QStringLiteral("parameterWriteScope")).toString() != QStringLiteral("SIL_ONLY")) {
        return false;
    }

    return true;
}

QString ParameterEditorController::_tuningBlockReason() const
{
    if (!_selectedFact) {
        return tr("请先在参数表中选择一个参数。");
    }
    if (_writeFailureNeedsReadback) {
        return tr("上一次写入尚未完成回读确认，请先回读当前参数。");
    }
    if (!_vehicle || MultiVehicleManager::instance()->activeVehicle() != _vehicle) {
        return tr("当前参数编辑器不属于活动飞行器，已禁止写入。");
    }
    if (!_vehicle->vehicleLinkManager() || _vehicle->vehicleLinkManager()->communicationLost()) {
        return tr("通信链路异常，已禁止参数写入。");
    }
    if (!_parameterMgr || !_parameterMgr->parametersReady() ||
        _parameterMgr->missingParameters() || _parameterMgr->parameterDownloadSkipped()) {
        return tr("参数列表尚未完整就绪。");
    }
    if (_parameterMgr->pendingWrites()) {
        return tr("参数管理器仍有未完成的写入。");
    }
    if (_vehicle->armed() || _vehicle->flying()) {
        return tr("在线调参仅允许飞行器解除锁定且未飞行时执行。");
    }
    if (_selectedFact->readOnly()) {
        return tr("该参数标记为只读，仅允许查看和回读，不允许在线写入。");
    }
    if (_selectedFact->writeOnly()) {
        return tr("该参数标记为只写，无法完成 PARAM_VALUE 回读确认，因此禁止在线调参。");
    }
    if (!_profileAllowsSelectedParameter()) {
        return tr("所选机型配置未通过 SIL-only 注册检查；实机写入保持锁定。");
    }
    return {};
}

bool ParameterEditorController::canApplySelectedValue() const
{
    return !_tuningBusy && !_readbackPending && !_writeFailureNeedsReadback &&
           _tuningBlockReason().isEmpty();
}

bool ParameterEditorController::canReadbackSelectedValue() const
{
    return _selectedFact && !_selectedFact->writeOnly() &&
           !_tuningBusy && !_readbackPending && _vehicle &&
           MultiVehicleManager::instance()->activeVehicle() == _vehicle &&
           _vehicle->vehicleLinkManager() && !_vehicle->vehicleLinkManager()->communicationLost() &&
           _parameterMgr && _parameterMgr->parametersReady() && !_parameterMgr->missingParameters() &&
           !_parameterMgr->parameterDownloadSkipped() && !_parameterMgr->pendingWrites() &&
           _profileAllowsSelectedParameter();
}

bool ParameterEditorController::canRestorePreviousValue() const
{
    return _hasPreviousValue && canApplySelectedValue();
}

bool ParameterEditorController::selectParameterFact(QObject *factObject)
{
    Fact *const fact = qobject_cast<Fact *>(factObject);
    if (!fact || !_parameterMgr || _tuningBusy || _readbackPending || _writeFailureNeedsReadback ||
        !_parameterMgr->parameterExists(fact->componentId(), fact->name()) ||
        _parameterMgr->getParameter(fact->componentId(), fact->name()) != fact) {
        return false;
    }
    if (_selectedFact == fact) {
        return true;
    }

    if (_selectedFact) {
        disconnect(_selectedFact, &Fact::vehicleUpdated,
                   this, &ParameterEditorController::_selectedFactVehicleUpdated);
    }
    _selectedFact = fact;
    connect(_selectedFact, &Fact::vehicleUpdated,
            this, &ParameterEditorController::_selectedFactVehicleUpdated);

    _confirmedValue = _selectedFact->cookedValue();
    const QString key = _selectedParameterKey();
    _hasPreviousValue = _previousValues.contains(key);
    _previousValue = _hasPreviousValue ? _previousValues.value(key) : QVariant();
    _tuningStatus = tr("已选择参数；仅允许 QUAD_X4 SIL 在解除锁定状态下应用。");
    _tuningError.clear();
    emit selectedFactChanged();
    emit tuningStateChanged();
    return true;
}

bool ParameterEditorController::applySelectedValue(const QVariant &cookedValue)
{
    if (!canApplySelectedValue()) {
        _tuningStatus = tr("当前条件不允许应用参数");
        _tuningError = _tuningBlockReason();
        emit tuningStateChanged();
        return false;
    }

    const QString valueText = cookedValue.toString().trimmed();
    const QString validationError = _selectedFact->validate(valueText, false);
    if (!validationError.isEmpty()) {
        _tuningStatus = tr("参数校验未通过");
        _tuningError = validationError;
        emit tuningStateChanged();
        return false;
    }

    bool targetOk = false;
    const double targetValue = valueText.toDouble(&targetOk);
    if (!targetOk || !std::isfinite(targetValue)) {
        _tuningStatus = tr("参数校验未通过");
        _tuningError = tr("请输入有效的有限数值。");
        emit tuningStateChanged();
        return false;
    }

    bool currentOk = false;
    const double currentValue = _selectedFact->cookedValue().toDouble(&currentOk);
    if (currentOk) {
        const double scale = std::max(1.0, std::max(std::abs(targetValue), std::abs(currentValue)));
        if (std::abs(targetValue - currentValue) <= 1e-9 * scale) {
            _tuningStatus = tr("目标值与当前值一致");
            _tuningError.clear();
            emit tuningStateChanged();
            return false;
        }
    }

    _pendingOriginalValue = _confirmedValue.isValid() ? _confirmedValue : _selectedFact->cookedValue();
    _pendingParameterName = _selectedFact->name();
    _pendingComponentId = _selectedFact->componentId();
    _tuningBusy = true;
    _tuningError.clear();
    _tuningStatus = tr("正在等待飞控 PARAM_VALUE 确认…");
    emit tuningStateChanged();

    // Reuse Fact -> ParameterManager -> MAVLink PARAM_SET. No parallel write queue is created.
    _selectedFact->setCookedValue(targetValue);
    bool appliedOk = false;
    const double appliedValue = _selectedFact->cookedValue().toDouble(&appliedOk);
    const double appliedScale = appliedOk
        ? std::max(1.0, std::max(std::abs(targetValue), std::abs(appliedValue)))
        : 1.0;
    if (!appliedOk || std::abs(appliedValue - targetValue) > 1e-9 * appliedScale) {
        _tuningBusy = false;
        _tuningStatus = tr("参数值未能进入现有 Fact 写入路径");
        _tuningError = tr("本地参数值转换失败，未确认写入请求。");
        emit tuningStateChanged();
        return false;
    }
    return true;
}

bool ParameterEditorController::refreshSelectedValue()
{
    if (!canReadbackSelectedValue()) {
        _tuningStatus = tr("当前条件不允许回读参数");
        _tuningError = tr("请确认活动飞行器、通信链路、参数状态和机型注册信息。");
        emit tuningStateChanged();
        return false;
    }

    _readbackPending = true;
    _tuningError.clear();
    _tuningStatus = tr("正在请求飞控参数回读…");
    emit tuningStateChanged();
    _parameterMgr->refreshParameter(_selectedFact->componentId(), _selectedFact->name());
    return true;
}

bool ParameterEditorController::restorePreviousValue()
{
    if (!canRestorePreviousValue()) {
        _tuningStatus = tr("当前条件不允许恢复参数");
        _tuningError = _tuningBlockReason();
        emit tuningStateChanged();
        return false;
    }
    return applySelectedValue(_previousValue);
}

void ParameterEditorController::_selectedFactVehicleUpdated(const QVariant &rawValue)
{
    Q_UNUSED(rawValue);
    if (_selectedFact) {
        // vehicleUpdated is emitted only for values received from the vehicle, not optimistic UI edits.
        _confirmedValue = _selectedFact->cookedValue();
        emit tuningStateChanged();
    }
}

void ParameterEditorController::_parameterWriteSucceeded(int componentId, const QString &paramName)
{
    if (!_tuningBusy || !_selectedFact || componentId != _pendingComponentId ||
        paramName != _pendingParameterName) {
        return;
    }

    _previousValues.insert(_selectedParameterKey(), _pendingOriginalValue);
    _previousValue = _pendingOriginalValue;
    _hasPreviousValue = true;
    _confirmedValue = _selectedFact->cookedValue();
    _pendingParameterName.clear();
    _pendingComponentId = -1;
    _tuningBusy = false;
    _writeFailureNeedsReadback = false;
    _tuningStatus = tr("已应用 · 已收到飞控 PARAM_VALUE 确认");
    _tuningError.clear();
    emit tuningStateChanged();
}

void ParameterEditorController::_parameterWriteFailed(int componentId, const QString &paramName)
{
    if (!_tuningBusy || !_selectedFact || componentId != _pendingComponentId ||
        paramName != _pendingParameterName) {
        return;
    }

    _pendingParameterName.clear();
    _pendingComponentId = -1;
    _tuningBusy = false;
    _writeFailureNeedsReadback = true;
    _tuningStatus = tr("写入失败 · 飞控未确认目标值");
    _tuningError = tr("请执行回读验证，并以飞控实际返回值为准。");
    emit tuningStateChanged();
}

void ParameterEditorController::_parameterReadSucceeded(int componentId, const QString &paramName, int paramIndex)
{
    Q_UNUSED(paramIndex);
    if (!_selectedFact || componentId != _selectedFact->componentId() ||
        paramName != _selectedFact->name() || (!_readbackPending && !_writeFailureNeedsReadback)) {
        return;
    }

    _readbackPending = false;
    _writeFailureNeedsReadback = false;
    _confirmedValue = _selectedFact->cookedValue();
    _tuningStatus = tr("回读验证完成");
    _tuningError.clear();
    emit tuningStateChanged();
}

void ParameterEditorController::_parameterReadFailed(int componentId, const QString &paramName, int paramIndex)
{
    Q_UNUSED(paramIndex);
    if (!_selectedFact || componentId != _selectedFact->componentId() ||
        paramName != _selectedFact->name() || (!_readbackPending && !_writeFailureNeedsReadback)) {
        return;
    }

    _readbackPending = false;
    _tuningStatus = tr("回读失败");
    _tuningError = tr("飞控未返回该参数的有效回读值。");
    emit tuningStateChanged();
}

void ParameterEditorController::_buildListsForComponent(int compId)
{
    if (_flightParameterCenterSafeMode() && _selectedModelProfileId.isEmpty()) {
        return;
    }

    for (const QString& factName: _parameterMgr->parameterNames(compId)) {
        if (!_selectedModelParameterNames.isEmpty() && !_selectedModelParameterNames.contains(factName)) {
            continue;
        }
        Fact* fact = _parameterMgr->getParameter(compId, factName);

        if (_hideReadOnly && fact->readOnly()) {
            continue;
        }

        ParameterEditorCategory* category = nullptr;
        if (_mapCategoryName2Category.contains(fact->category())) {
            category = _mapCategoryName2Category[fact->category()];
        } else {
            category        = new ParameterEditorCategory(this);
            category->name  = fact->category();
            _mapCategoryName2Category[fact->category()] = category;
            _categories.append(category);
        }

        ParameterEditorGroup* group = nullptr;
        if (category->mapGroupName2Group.contains(fact->group())) {
            group = category->mapGroupName2Group[fact->group()];
        } else {
            group               = new ParameterEditorGroup(this);
            group->componentId  = compId;
            group->name         = fact->group();
            category->mapGroupName2Group[fact->group()] = group;
            category->groups.append(group);
        }

        group->facts.append(fact);
    }
}

void ParameterEditorController::_buildLists(void)
{
    _currentCategory = nullptr;
    _currentGroup = nullptr;
    _parameters = nullptr;
    _mapCategoryName2Category.clear();
    _categories.clearAndDeleteContents();
    emit parametersChanged();

    // Autopilot component should always be first list
    _buildListsForComponent(MAV_COMP_ID_AUTOPILOT1);

    // "Standard" category should always be first
    for (int i=0; i<_categories.count(); i++) {
        ParameterEditorCategory* category = _categories.value<ParameterEditorCategory*>(i);
        if (category->name == "Standard" && i != 0) {
            _categories.removeAt(i);
            _categories.insert(0, category);
            break;
        }
    }

    // Default category should always be last
    for (int i=0; i<_categories.count(); i++) {
        ParameterEditorCategory* category = _categories.value<ParameterEditorCategory*>(i);
        if (category->name == FactMetaData::kDefaultCategory) {
            if (i != _categories.count() - 1) {
                _categories.removeAt(i);
                _categories.append(category);
            }
            break;
        }
    }

    // Now add other random components
    for (int compId: _parameterMgr->componentIds()) {
        if (compId != MAV_COMP_ID_AUTOPILOT1) {
            _buildListsForComponent(compId);
        }
    }

    // Default group should always be last
    for (int i=0; i<_categories.count(); i++) {
        ParameterEditorCategory* category = _categories.value<ParameterEditorCategory*>(i);
        for (int j=0; j<category->groups.count(); j++) {
            ParameterEditorGroup* group = category->groups.value<ParameterEditorGroup*>(j);
            if (group->name == FactMetaData::kDefaultGroup) {
                if (j != _categories.count() - 1) {
                    category->groups.removeAt(j);
                    category->groups.append(group);
                }
                break;
            }
        }
    }
}

void ParameterEditorController::_factAdded(int compId, Fact* fact)
{
    if (_flightParameterCenterSafeMode() && _selectedModelProfileId.isEmpty()) {
        return;
    }
    if (!_selectedModelParameterNames.isEmpty() && !_selectedModelParameterNames.contains(fact->name())) {
        return;
    }
    if (_hideReadOnly && fact->readOnly()) {
        return;
    }

    bool                        inserted = false;
    ParameterEditorCategory*    category = nullptr;

    if (_mapCategoryName2Category.contains(fact->category())) {
        category = _mapCategoryName2Category[fact->category()];
    } else {
        category        = new ParameterEditorCategory(this);
        category->name  = fact->category();
        _mapCategoryName2Category[fact->category()] = category;

        // Insert in sorted order
        inserted = false;
        for (int i=0; i<_categories.count(); i++) {
            if (_categories.value<ParameterEditorCategory*>(i)->name > category->name) {
                _categories.insert(i, category);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            _categories.append(category);
        }
    }

    ParameterEditorGroup* group = nullptr;
    if (category->mapGroupName2Group.contains(fact->group())) {
        group = category->mapGroupName2Group[fact->group()];
    } else {
        group               = new ParameterEditorGroup(this);
        group->componentId  = compId;
        group->name         = fact->group();
        category->mapGroupName2Group[fact->group()] = group;

        // Insert in sorted order
        QmlObjectListModel& groups = category->groups;
        inserted = false;
        for (int i=0; i<groups.count(); i++) {
            if (groups.value<ParameterEditorGroup*>(i)->name > group->name) {
                groups.insert(i, group);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            groups.append(group);
        }
    }

    // Insert in sorted order
    auto& facts = group->facts;
    for (int i=0; i<facts.rowCount(); i++) {
        if (facts.factAt(i)->name() > fact->name()) {
            facts.insert(i, fact);
            return;
        }
    }
    facts.append(fact);
}

void ParameterEditorController::saveToFile(const QString& filename)
{
    if (!filename.isEmpty()) {
        QString parameterFilename = filename;
        if (!QFileInfo(filename).fileName().contains(".")) {
            parameterFilename += QString(".%1").arg(AppSettings::parameterFileExtension);
        }

        QFile file(parameterFilename);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCWarning(ParameterEditorControllerLog) << "saveToFile: unable to create file" << parameterFilename;
            QGC::showAppMessage(tr("Unable to create file: %1").arg(parameterFilename));
            return;
        }

        qCDebug(ParameterEditorControllerLog) << "saveToFile:" << parameterFilename;
        QTextStream stream(&file);
        _parameterMgr->writeParametersToStream(stream);
        file.close();
    }
}

template <typename T, typename SignalFn>
void ParameterEditorController::_setDiffProperty(T& member, const T& value, SignalFn changedSignal)
{
    if (member == value) {
        return;
    }
    member = value;
    emit (this->*changedSignal)(member);
}

void ParameterEditorController::clearDiff(void)
{
    _diffList.clearAndDeleteContents();
    _setDiffProperty(_diffOtherVehicle,         false,          &ParameterEditorController::diffOtherVehicleChanged);
    _setDiffProperty(_diffMultipleComponents,   false,          &ParameterEditorController::diffMultipleComponentsChanged);
    _setDiffProperty(_diffParsedCount,          0,              &ParameterEditorController::diffParsedCountChanged);
    _setDiffProperty(_diffUnchangedCount,       0,              &ParameterEditorController::diffUnchangedCountChanged);
    _setDiffProperty(_diffReadOnlyCount,        0,              &ParameterEditorController::diffReadOnlyCountChanged);
    _setDiffProperty(_diffNoVehicleCount,       0,              &ParameterEditorController::diffNoVehicleCountChanged);
    _setDiffProperty(_diffSendableCount,        0,              &ParameterEditorController::diffSendableCountChanged);
    _setDiffProperty(_diffSelectedCount,        0,              &ParameterEditorController::diffSelectedCountChanged);
    _setDiffProperty(_diffMissingParams,        QStringList(),  &ParameterEditorController::diffMissingParamsChanged);
}

// Recomputed whenever a diff row's load checkbox changes. Drives the dialog's Ok button enabled state.
void ParameterEditorController::_updateDiffSelectedCount()
{
    int selectedCount = 0;
    for (int i=0; i<_diffList.count(); i++) {
        const ParameterEditorDiff* paramDiff = _diffList.value<ParameterEditorDiff*>(i);
        if (paramDiff->load && !paramDiff->cannotSend) {
            selectedCount++;
        }
    }
    _setDiffProperty(_diffSelectedCount, selectedCount, &ParameterEditorController::diffSelectedCountChanged);
}

void ParameterEditorController::sendDiff(void)
{
    if (_flightParameterCenterSafeMode()) {
        QGC::showAppMessage(tr("Flight 参数中心禁止批量参数写入，请使用受控的单参数在线调参。"));
        return;
    }

    int sentCount = 0;
    int uncheckedCount = 0;

    for (int i=0; i<_diffList.count(); i++) {
        ParameterEditorDiff* paramDiff = _diffList.value<ParameterEditorDiff*>(i);

        if (paramDiff->cannotSend) {
            qCDebug(ParameterEditorControllerLog) << "sendDiff: skipped (cannot send, not on vehicle) -" << paramDiff->name;
            continue;
        }

        if (paramDiff->load) {
            sentCount++;
            if (paramDiff->noVehicleValue) {
                qCDebug(ParameterEditorControllerLog) << "sendDiff: PARAM_SET new param -" << paramDiff->name
                    << "componentId:" << paramDiff->componentId << "value:" << paramDiff->fileValueVar;
                _parameterMgr->_mavlinkParamSet(paramDiff->componentId, paramDiff->name, paramDiff->valueType, paramDiff->fileValueVar);
            } else {
                qCDebug(ParameterEditorControllerLog) << "sendDiff: fact write -" << paramDiff->name
                    << "componentId:" << paramDiff->componentId << "value:" << paramDiff->fileValueVar;
                Fact* fact = _parameterMgr->getParameter(paramDiff->componentId, paramDiff->name);
                fact->setRawValue(paramDiff->fileValueVar);
            }
        } else {
            uncheckedCount++;
            qCDebug(ParameterEditorControllerLog) << "sendDiff: skipped (unchecked) -" << paramDiff->name;
        }
    }

    qCDebug(ParameterEditorControllerLog) << "sendDiff summary - sent:" << sentCount << "unchecked:" << uncheckedCount;
}

bool ParameterEditorController::buildDiffFromFile(const QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(ParameterEditorControllerLog) << "buildDiffFromFile: unable to open file" << filename;
        QGC::showAppMessage(tr("Unable to open file: %1").arg(filename));
        return false;
    }

    clearDiff();

    QStringList invalidValues;
    qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile:" << filename;

    QTextStream stream(&file);

    // Accumulate in locals; property setters at the end emit only for values that changed.
    bool        diffOtherVehicle        = false;
    bool        diffMultipleComponents  = false;
    int         diffParsedCount         = 0;
    int         diffUnchangedCount      = 0;
    int         diffReadOnlyCount       = 0;
    int         diffNoVehicleCount      = 0;
    int         diffSendableCount       = 0;
    QStringList diffMissingParams;

    int firstComponentId = -1;
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (!line.startsWith("#") && !line.trimmed().isEmpty()) {
            QStringList wpParams = line.trimmed().split(QRegularExpression("[\\t ,]+"));

            int         componentId     = -1;
            QString     paramName;
            QString     fileValueStr;
            int         mavParamType    = -1;
            bool        isMPFormat      = false;

            if (wpParams.size() == 5) {
                // QGC tab-delimited: VehicleId ComponentId Name Value Type
                int vehicleId   = wpParams.at(0).toInt();
                componentId     = wpParams.at(1).toInt();
                paramName       = wpParams.at(2);
                fileValueStr    = wpParams.at(3);
                mavParamType    = wpParams.at(4).toInt();

                if (_vehicle->id() != vehicleId) {
                    if (!diffOtherVehicle) {
                        qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: file is from other vehicle - file vehicleId:" << vehicleId << "connected vehicleId:" << _vehicle->id();
                    }
                    diffOtherVehicle = true;
                }
                if (firstComponentId == -1) {
                    firstComponentId = componentId;
                } else if (firstComponentId != componentId) {
                    if (!diffMultipleComponents) {
                        qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: file contains multiple componentIds:" << firstComponentId << componentId;
                    }
                    diffMultipleComponents = true;
                }
            } else if (wpParams.size() == 2) {
                // Mission Planner 2-column: Name Value
                paramName       = wpParams.at(0);
                fileValueStr    = wpParams.at(1);
                componentId     = ParameterManager::defaultComponentId;
                isMPFormat      = true;
            } else {
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: skipping unparseable line:" << line.trimmed().left(80);
                continue;
            }

            diffParsedCount++;

            QString     vehicleValueStr;
            QString     units;
            QVariant    fileValueVar    = fileValueStr;
            bool        noVehicleValue  = false;
            bool        readOnly        = false;

            if (_parameterMgr->parameterExists(componentId, paramName)) {
                Fact*           vehicleFact         = _parameterMgr->getParameter(componentId, paramName);
                FactMetaData*   vehicleFactMetaData = vehicleFact->metaData();
                Fact            fileFact(vehicleFact->componentId(), vehicleFact->name(), vehicleFact->type());

                if (mavParamType == -1) {
                    mavParamType = ParameterManager::factTypeToMavType(vehicleFact->type());
                }

                QVariant typedValue;
                QString conversionError;
                if (!vehicleFactMetaData->convertAndValidateRaw(fileValueStr, true, typedValue, conversionError)) {
                    invalidValues.append(paramName);
                    continue;
                }

                // Turn off reboot messaging before setting value in fileFact
                bool vehicleRebootRequired = vehicleFactMetaData->vehicleRebootRequired();
                vehicleFactMetaData->setVehicleRebootRequired(false);
                fileFact.setMetaData(vehicleFact->metaData());
                fileFact.setRawValue(typedValue);
                vehicleFactMetaData->setVehicleRebootRequired(vehicleRebootRequired);
                readOnly = vehicleFact->readOnly();

                if (vehicleFact->rawValue() == fileFact.rawValue()) {
                    diffUnchangedCount++;
                    continue;
                }
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: changed -" << paramName
                    << "vehicle:" << vehicleFact->rawValue() << "file:" << fileFact.rawValue();
                fileValueStr    = fileFact.enumOrValueString();
                fileValueVar    = fileFact.rawValue();
                vehicleValueStr = vehicleFact->enumOrValueString();
                units           = vehicleFact->cookedUnits();
            } else if (isMPFormat) {
                // MP format: param not on vehicle and file carries no type info, so it can never
                // be sent. Show it in the diff list as a disabled (cannot-send) row.
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: not on vehicle, cannot send (MP format) -" << paramName;
                diffMissingParams.append(paramName);

                ParameterEditorDiff* paramDiff = new ParameterEditorDiff(this);

                paramDiff->componentId  = componentId;
                paramDiff->name         = paramName;
                paramDiff->valueType    = FactMetaData::valueTypeFloat;    // Unknown - never sent
                paramDiff->fileValue    = fileValueStr;
                paramDiff->fileValueVar = fileValueVar;
                paramDiff->cannotSend   = true;
                paramDiff->load         = false;

                _diffList.append(paramDiff);
                continue;
            } else {
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: not on vehicle, will send as new (QGC format) -" << paramName << "value:" << fileValueStr;
                noVehicleValue = true;

                // fileValueVar is still a QString variant. Convert it to the typed variant matching the
                // file's param type, otherwise the PARAM_VALUE ack type check in _mavlinkParamSet fails
                // and the send would retry/time out.
                const FactMetaData metaData(ParameterManager::mavTypeToFactType(static_cast<MAV_PARAM_TYPE>(mavParamType)));
                QVariant    typedValue;
                QString     errorString;
                if (metaData.convertAndValidateRaw(fileValueVar, true /* convertOnly */, typedValue, errorString)) {
                    fileValueVar = typedValue;
                } else {
                    qCWarning(ParameterEditorControllerLog) << "buildDiffFromFile: value conversion failed, skipping -" << paramName
                        << "value:" << fileValueStr << "error:" << errorString;
                    diffParsedCount--;
                    continue;
                }
            }

            if (!readOnly) {
                ParameterEditorDiff* paramDiff = new ParameterEditorDiff(this);

                paramDiff->componentId      = componentId;
                paramDiff->name             = paramName;
                paramDiff->valueType        = ParameterManager::mavTypeToFactType(static_cast<MAV_PARAM_TYPE>(mavParamType));
                paramDiff->fileValue        = fileValueStr;
                paramDiff->fileValueVar     = fileValueVar;
                paramDiff->vehicleValue     = vehicleValueStr;
                paramDiff->noVehicleValue   = noVehicleValue;
                paramDiff->units            = units;

                (void) connect(paramDiff, &ParameterEditorDiff::loadChanged, this, &ParameterEditorController::_updateDiffSelectedCount);

                _diffList.append(paramDiff);
                diffSendableCount++;

                if (noVehicleValue) {
                    diffNoVehicleCount++;
                }
            } else {
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: skipping read-only param -" << paramName;
                diffReadOnlyCount++;
            }
        }
    }

    file.close();
    if (!invalidValues.isEmpty()) {
        clearDiff();
        QGC::showAppMessage(tr("Invalid values in the parameter file: %1").arg(invalidValues.join(QStringLiteral(", "))));
        return false;
    }

    if (diffParsedCount == 0) {
        QGC::showAppMessage(tr("No valid parameters found in file. Check that the file is in QGC or Mission Planner format."));
        return false;
    }

    qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile summary -"
        << "parsed:" << diffParsedCount
        << "changed:" << diffSendableCount
        << "unchanged:" << diffUnchangedCount
        << "readOnly:" << diffReadOnlyCount
        << "noVehicleValue:" << diffNoVehicleCount
        << "missing:" << diffMissingParams.count()
        << (diffMissingParams.isEmpty() ? QString() : diffMissingParams.join(QStringLiteral(", ")));

    _setDiffProperty(_diffOtherVehicle,         diffOtherVehicle,       &ParameterEditorController::diffOtherVehicleChanged);
    _setDiffProperty(_diffMultipleComponents,   diffMultipleComponents, &ParameterEditorController::diffMultipleComponentsChanged);
    _setDiffProperty(_diffParsedCount,          diffParsedCount,        &ParameterEditorController::diffParsedCountChanged);
    _setDiffProperty(_diffUnchangedCount,       diffUnchangedCount,     &ParameterEditorController::diffUnchangedCountChanged);
    _setDiffProperty(_diffReadOnlyCount,        diffReadOnlyCount,      &ParameterEditorController::diffReadOnlyCountChanged);
    _setDiffProperty(_diffNoVehicleCount,       diffNoVehicleCount,     &ParameterEditorController::diffNoVehicleCountChanged);
    _setDiffProperty(_diffSendableCount,        diffSendableCount,      &ParameterEditorController::diffSendableCountChanged);
    _setDiffProperty(_diffSelectedCount,        diffSendableCount,      &ParameterEditorController::diffSelectedCountChanged); // All sendable rows start checked
    _setDiffProperty(_diffMissingParams,        diffMissingParams,      &ParameterEditorController::diffMissingParamsChanged);

    return true;
}

void ParameterEditorController::refresh(void)
{
    _parameterMgr->refreshAllParameters();
}

void ParameterEditorController::resetAllToDefaults(void)
{
    if (_flightParameterCenterSafeMode()) {
        QGC::showAppMessage(tr("Flight 参数中心禁止批量恢复默认值。"));
        return;
    }
    _parameterMgr->resetAllParametersToDefaults();
    refresh();
}

void ParameterEditorController::resetAllToVehicleConfiguration(void)
{
    if (_flightParameterCenterSafeMode()) {
        QGC::showAppMessage(tr("Flight 参数中心禁止批量恢复机型默认值。"));
        return;
    }
    _parameterMgr->resetAllToVehicleConfiguration();
    refresh();
}

bool ParameterEditorController::_shouldShow(Fact* fact) const
{
    if (_flightParameterCenterSafeMode() && _selectedModelProfileId.isEmpty()) {
        return false;
    }
    if (!_selectedModelParameterNames.isEmpty() && !_selectedModelParameterNames.contains(fact->name())) {
        return false;
    }
    if (_hideReadOnly && fact->readOnly()) {
        return false;
    }
    if (_showModifiedOnly) {
        if (!fact->defaultValueAvailable() || fact->valueEqualsDefault()) {
            return false;
        }
    }
    if (_showFavoritesOnly) {
        if (!_favoriteNames.contains(fact->name())) {
            return false;
        }
    }
    return true;
}

void ParameterEditorController::_searchTextChanged(void)
{
    _searchTimer.start();
}

void ParameterEditorController::_hideReadOnlyChanged(void)
{
    _buildLists();

    ParameterEditorCategory* category = _categoryFilter.isEmpty()
        ? (_categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr)
        : _mapCategoryName2Category.value(_categoryFilter, nullptr);
    setCurrentCategory(category);

    // Re-trigger filtered results if search, modified or favorites view is active.
    if (!_searchText.isEmpty() || _showModifiedOnly || _showFavoritesOnly) {
        _performSearch();
    }
}

void ParameterEditorController::_performSearch(void)
{
    QObjectList newParameterList;

    QStringList rgSearchStrings = _searchText.split(' ', Qt::SkipEmptyParts);

    if (rgSearchStrings.isEmpty() && !_showModifiedOnly && !_showFavoritesOnly) {
        ParameterEditorCategory* category = _categoryFilter.isEmpty()
            ? (_categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr)
            : _mapCategoryName2Category.value(_categoryFilter, nullptr);
        setCurrentCategory(category);
        _searchParameters.clear();
    } else {
        QVector<QRegularExpression> regexList;
        regexList.reserve(rgSearchStrings.size());
        for (const QString &searchItem : rgSearchStrings) {
            QRegularExpression re(searchItem, QRegularExpression::CaseInsensitiveOption);
            regexList.append(re.isValid() ? re : QRegularExpression());
        }

        _searchParameters.beginReset();
        _searchParameters.clear();

        for (int compId : _parameterMgr->componentIds()) {
            for (const QString &paraName: _parameterMgr->parameterNames(compId)) {
                Fact* fact = _parameterMgr->getParameter(compId, paraName);
                if (!_categoryFilter.isEmpty() && fact->category() != _categoryFilter) {
                    continue;
                }
                bool matched = _shouldShow(fact);
                // All of the search items must match in order for the parameter to be added to the list
                if (matched) {
                    for (int i = 0; i < rgSearchStrings.size(); ++i) {
                        const QRegularExpression &re = regexList.at(i);
                        if (re.isValid()) {
                            if (!fact->name().contains(re) &&
                                    !fact->shortDescription().contains(re) &&
                                    !fact->longDescription().contains(re)) {
                                matched = false;
                            }
                        } else {
                            const QString &searchItem = rgSearchStrings.at(i);
                            if (!fact->name().contains(searchItem, Qt::CaseInsensitive) &&
                                    !fact->shortDescription().contains(searchItem, Qt::CaseInsensitive) &&
                                    !fact->longDescription().contains(searchItem, Qt::CaseInsensitive)) {
                                matched = false;
                            }
                        }
                    }
                }
                if (matched) {
                    _searchParameters.append(fact);
                }
            }
        }

        _searchParameters.endReset();

        if (_parameters != &_searchParameters) {
            _parameters = &_searchParameters;
            emit parametersChanged();

            _currentCategory    = nullptr;
            _currentGroup       = nullptr;
        }
    }
}

void ParameterEditorController::_currentCategoryChanged(void)
{
    ParameterEditorGroup* group = nullptr;
    if (_currentCategory) {
        // Select first group when category changes
        group = _currentCategory->groups.value<ParameterEditorGroup*>(0);
    } else {
        group = nullptr;
    }
    setCurrentGroup(group);
}

void ParameterEditorController::_currentGroupChanged(void)
{
    _parameters = _currentGroup ? &_currentGroup->facts : nullptr;
    emit parametersChanged();
}

void ParameterEditorController::setCurrentCategory(QObject* currentCategory)
{
    ParameterEditorCategory* category = qobject_cast<ParameterEditorCategory*>(currentCategory);
    if (category != _currentCategory) {
        _currentCategory = category;
        emit currentCategoryChanged();
    }
}

void ParameterEditorController::setCurrentGroup(QObject* currentGroup)
{
    ParameterEditorGroup* group = qobject_cast<ParameterEditorGroup*>(currentGroup);
    if (group != _currentGroup) {
        _currentGroup = group;
        emit currentGroupChanged();
    }
}

QStringList ParameterEditorController::favoriteParameterNames(void) const
{
    QStringList list(_favoriteNames.begin(), _favoriteNames.end());
    list.sort();
    return list;
}

void ParameterEditorController::toggleFavorite(const QString& paramName)
{
    if (_favoriteNames.contains(paramName)) {
        _favoriteNames.remove(paramName);
    } else {
        _favoriteNames.insert(paramName);
    }
    _saveFavorites();
    emit favoritesChanged();

    if (_showFavoritesOnly) {
        _performSearch();
    }
}

bool ParameterEditorController::isFavorite(const QString& paramName) const
{
    return _favoriteNames.contains(paramName);
}

void ParameterEditorController::clearAllFavorites(void)
{
    _favoriteNames.clear();
    _saveFavorites();
    emit favoritesChanged();

    if (_showFavoritesOnly) {
        _performSearch();
    }
}

void ParameterEditorController::_loadFavorites()
{
    Fact* fact = SettingsManager::instance()->appSettings()->favoriteParameters();
    const QStringList list = fact->rawValue().toString().split(",", Qt::SkipEmptyParts);
    _favoriteNames = QSet<QString>(list.begin(), list.end());
}

void ParameterEditorController::_saveFavorites()
{
    QStringList list(_favoriteNames.begin(), _favoriteNames.end());
    list.sort();
    Fact* fact = SettingsManager::instance()->appSettings()->favoriteParameters();
    fact->setRawValue(list.join(","));
}
