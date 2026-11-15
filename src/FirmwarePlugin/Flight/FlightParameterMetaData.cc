#include "FlightParameterMetaData.h"

#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QStringList>

#include <algorithm>
#include <cmath>

QGC_LOGGING_CATEGORY(FlightParameterMetaDataLog, "FirmwarePlugin.FlightParameterMetaData")

namespace {

const QStringList &supportedParameterDomains()
{
    static const QStringList domains {
        QStringLiteral("VEHICLE_MODEL"),
        QStringLiteral("FMS"),
        QStringLiteral("CONTROL"),
        QStringLiteral("INS"),
        QStringLiteral("PLANT"),
        QStringLiteral("ACTUATOR"),
        QStringLiteral("SIMULATION")
    };
    return domains;
}

const QStringList &supportedTuningPolicies()
{
    static const QStringList policies {
        QStringLiteral("READ_ONLY"),
        QStringLiteral("GROUND_ADJUSTABLE"),
        QStringLiteral("ONLINE_ADJUSTABLE"),
        QStringLiteral("FLIGHT_RESTRICTED")
    };
    return policies;
}

const QStringList &supportedUpdatePolicies()
{
    static const QStringList policies {
        QStringLiteral("IMMEDIATE"),
        QStringLiteral("FIRMWARE_CONFIRMATION"),
        QStringLiteral("REBOOT_REQUIRED"),
        QStringLiteral("OFFLINE_ONLY")
    };
    return policies;
}

bool readUniqueStringList(const QJsonValue &value, QStringList &strings, bool allowEmpty = false)
{
    if (!value.isArray()) {
        return false;
    }

    QSet<QString> seen;
    for (const QJsonValue &entry : value.toArray()) {
        if (!entry.isString()) {
            return false;
        }
        const QString raw = entry.toString();
        const QString normalized = raw.trimmed();
        if (normalized.isEmpty() || normalized != raw || seen.contains(normalized)) {
            return false;
        }
        seen.insert(normalized);
        strings.append(normalized);
    }
    return allowEmpty || !strings.isEmpty();
}

bool validSourceMetadata(const QJsonObject &entry)
{
    const QString sourceFile = entry.value(QStringLiteral("sourceFile")).toString();
    const QString sourcePath = entry.value(QStringLiteral("sourcePath")).toString();
    const QString sourceKind = entry.value(QStringLiteral("sourceKind")).toString();
    static const QStringList sourceKinds {
        QStringLiteral("scalar"),
        QStringLiteral("vector"),
        QStringLiteral("matrix"),
        QStringLiteral("stored")
    };
    return !sourceFile.isEmpty() && sourceFile == sourceFile.trimmed() &&
           !sourcePath.isEmpty() && sourcePath == sourcePath.trimmed() &&
           sourceKinds.contains(sourceKind);
}

bool validRuleArray(const QJsonValue &value, QVariantList &rules)
{
    if (!value.isArray()) {
        return false;
    }

    static const QSet<QString> numericRuleTypes {
        QStringLiteral("GREATER_THAN"),
        QStringLiteral("GREATER_THAN_OR_EQUAL"),
        QStringLiteral("LESS_THAN"),
        QStringLiteral("LESS_THAN_OR_EQUAL"),
        QStringLiteral("MULTIPLE_OF")
    };

    for (const QJsonValue &entry : value.toArray()) {
        if (!entry.isObject()) {
            return false;
        }
        const QJsonObject rule = entry.toObject();
        const QString type = rule.value(QStringLiteral("type")).toString();
        const QJsonValue ruleValue = rule.value(QStringLiteral("value"));
        if (numericRuleTypes.contains(type)) {
            if (rule.size() != 2 || !ruleValue.isDouble() || !std::isfinite(ruleValue.toDouble()) ||
                (type == QStringLiteral("MULTIPLE_OF") && ruleValue.toDouble() <= 0.0)) {
                return false;
            }
        } else if (type == QStringLiteral("REQUIRES_PARAMETER_VALUE")) {
            const QString parameter = rule.value(QStringLiteral("parameter")).toString();
            if (rule.size() != 3 || parameter.isEmpty() || parameter != parameter.trimmed() ||
                !ruleValue.isDouble() || !std::isfinite(ruleValue.toDouble())) {
                return false;
            }
        } else {
            return false;
        }
        rules.append(rule.toVariantMap());
    }
    return true;
}

bool validNumericField(const QJsonObject &entry, const QString &fieldName)
{
    if (!entry.contains(fieldName)) {
        return true;
    }
    const QJsonValue value = entry.value(fieldName);
    return value.isDouble() && std::isfinite(value.toDouble());
}

} // namespace

FlightParameterMetaData::FlightParameterMetaData(QObject *parent)
    : ParameterMetaData(parent)
{
}

FlightParameterMetaData::~FlightParameterMetaData()
{
}

void FlightParameterMetaData::parseParameterJson(const QJsonObject &json)
{
    if (json.value(QStringLiteral("version")).toInt(-1) != 2) {
        qCWarning(FlightParameterMetaDataLog) << "Unsupported Flight parameter metadata schema version";
        return;
    }

    QStringList registeredProfiles;
    if (!readUniqueStringList(json.value(QStringLiteral("modelProfiles")), registeredProfiles)) {
        qCWarning(FlightParameterMetaDataLog) << "Flight parameter metadata has no valid model profile list";
        return;
    }

    const QJsonArray parameters = json.value(QStringLiteral("parameters")).toArray();

    // Reject every occurrence of an ambiguous name, not merely the second one.
    QHash<QString, int> nameOccurrences;
    QSet<QString> declaredParameterNames;
    for (const QJsonValue &value : parameters) {
        if (value.isObject()) {
            const QString name = value.toObject().value(QStringLiteral("name")).toString().trimmed();
            if (!name.isEmpty()) {
                ++nameOccurrences[name];
                declaredParameterNames.insert(name);
            }
        }
    }

    for (const QJsonValue &value : parameters) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject entry = value.toObject();
        const QString name = entry.value(QStringLiteral("name")).toString().trimmed();
        if (name.isEmpty()) {
            continue;
        }
        if (nameOccurrences.value(name) > 1) {
            qCWarning(FlightParameterMetaDataLog)
                << "Ignoring duplicate Flight parameter metadata name:" << name;
            continue;
        }

        const QString domain = entry.value(QStringLiteral("domain")).toString();
        if (!supportedParameterDomains().contains(domain)) {
            qCWarning(FlightParameterMetaDataLog)
                << "Ignoring parameter metadata with missing or unsupported domain:" << name << domain;
            continue;
        }

        // The current Flight parameter contract is float-only. Do not silently
        // reinterpret a future integer/string parameter as a float.
        if (entry.value(QStringLiteral("type")).toString() != QStringLiteral("float")) {
            qCWarning(FlightParameterMetaDataLog)
                << "Ignoring parameter metadata with unsupported type:" << name
                << entry.value(QStringLiteral("type"));
            continue;
        }

        const QString vehicleFamily = entry.value(QStringLiteral("vehicleFamily")).toString();
        const QString controlLaw = entry.value(QStringLiteral("controlLaw")).toString();
        const QString tuningPolicy = entry.value(QStringLiteral("tuningPolicy")).toString();
        const QString updatePolicy = entry.value(QStringLiteral("updatePolicy")).toString();
        QStringList modelVariants;
        QStringList modelProfiles;
        QStringList dependencies;
        QVariantList validationRules;
        const bool validDependencyList =
            readUniqueStringList(entry.value(QStringLiteral("dependencies")), dependencies, true) &&
            !dependencies.contains(name) &&
            std::all_of(dependencies.cbegin(), dependencies.cend(), [&declaredParameterNames](const QString &dependency) {
                return declaredParameterNames.contains(dependency);
            });
        const bool validRules = validRuleArray(entry.value(QStringLiteral("validationRules")), validationRules) &&
            std::all_of(validationRules.cbegin(), validationRules.cend(),
                         [&dependencies, &declaredParameterNames](const QVariant &ruleValue) {
                const QVariantMap rule = ruleValue.toMap();
                if (rule.value(QStringLiteral("type")).toString() != QStringLiteral("REQUIRES_PARAMETER_VALUE")) {
                    return true;
                }
                const QString parameter = rule.value(QStringLiteral("parameter")).toString();
                return declaredParameterNames.contains(parameter) && dependencies.contains(parameter);
            });
        const bool validApplicability =
            !vehicleFamily.trimmed().isEmpty() && vehicleFamily == vehicleFamily.trimmed() &&
            !controlLaw.trimmed().isEmpty() && controlLaw == controlLaw.trimmed() &&
            validSourceMetadata(entry) &&
            readUniqueStringList(entry.value(QStringLiteral("modelVariants")), modelVariants) &&
            readUniqueStringList(entry.value(QStringLiteral("modelProfiles")), modelProfiles) &&
            validDependencyList && validRules;
        const bool validPolicy =
            supportedTuningPolicies().contains(tuningPolicy) &&
            supportedUpdatePolicies().contains(updatePolicy) &&
            entry.value(QStringLiteral("readOnly")).isBool() &&
            entry.value(QStringLiteral("readOnly")).toBool() == (tuningPolicy == QStringLiteral("READ_ONLY"));
        const bool hasRebootRequirement = entry.contains(QStringLiteral("rebootRequired"));
        const QJsonValue rebootRequirement = entry.value(QStringLiteral("rebootRequired"));
        const bool validRebootPolicy = hasRebootRequirement
            ? (rebootRequirement.isBool() &&
               rebootRequirement.toBool() == (updatePolicy == QStringLiteral("REBOOT_REQUIRED")))
            : (updatePolicy != QStringLiteral("REBOOT_REQUIRED"));

        if (!validApplicability || !validPolicy || !validRebootPolicy ||
            !validNumericField(entry, QStringLiteral("min")) ||
            !validNumericField(entry, QStringLiteral("max")) ||
            !validNumericField(entry, QStringLiteral("default")) ||
            (entry.contains(QStringLiteral("writeOnly")) &&
             (!entry.value(QStringLiteral("writeOnly")).isBool() ||
              (entry.value(QStringLiteral("writeOnly")).toBool() &&
               entry.value(QStringLiteral("readOnly")).toBool()))) ||
            (entry.contains(QStringLiteral("decimalPlaces")) &&
             (!entry.value(QStringLiteral("decimalPlaces")).isDouble() ||
              entry.value(QStringLiteral("decimalPlaces")).toInt(-1) < 0 ||
              entry.value(QStringLiteral("decimalPlaces")).toInt(-1) > 9))) {
            qCWarning(FlightParameterMetaDataLog)
                << "Ignoring parameter metadata with invalid applicability, policy, numeric or boolean fields:" << name;
            continue;
        }

        bool profilesRegistered = true;
        for (const QString &profile : modelProfiles) {
            if (!registeredProfiles.contains(profile)) {
                profilesRegistered = false;
                qCWarning(FlightParameterMetaDataLog)
                    << "Ignoring parameter metadata referencing an unregistered model profile:" << name << profile;
                break;
            }
        }
        if (!profilesRegistered) {
            continue;
        }

        if (entry.contains(QStringLiteral("rebootRequired")) &&
            !entry.value(QStringLiteral("rebootRequired")).isBool()) {
            qCWarning(FlightParameterMetaDataLog)
                << "Ignoring parameter metadata with invalid rebootRequired type:" << name;
            continue;
        }

        if (entry.contains(QStringLiteral("min")) && entry.contains(QStringLiteral("max")) &&
            entry.value(QStringLiteral("min")).toDouble() > entry.value(QStringLiteral("max")).toDouble()) {
            qCWarning(FlightParameterMetaDataLog)
                << "Ignoring parameter metadata with inverted range:" << name;
            continue;
        }

        if (entry.contains(QStringLiteral("default")) &&
            ((entry.contains(QStringLiteral("min")) &&
              entry.value(QStringLiteral("default")).toDouble() < entry.value(QStringLiteral("min")).toDouble()) ||
             (entry.contains(QStringLiteral("max")) &&
              entry.value(QStringLiteral("default")).toDouble() > entry.value(QStringLiteral("max")).toDouble()))) {
            qCWarning(FlightParameterMetaDataLog)
                << "Ignoring parameter metadata with default outside its declared range:" << name;
            continue;
        }

        if (_cachedMetaData.contains(name)) {
            qCWarning(FlightParameterMetaDataLog)
                << "Ignoring duplicate Flight parameter metadata entry:" << name;
            continue;
        }

        auto *metaData = new FactMetaData(FactMetaData::valueTypeFloat, this);
        metaData->setName(name);
        metaData->setGroup(entry.value(QStringLiteral("group")).toString());
        metaData->setCategory(domain);
        metaData->setShortDescription(entry.value(QStringLiteral("shortDescription")).toString());
        metaData->setLongDescription(entry.value(QStringLiteral("longDescription")).toString());
        metaData->setRawUnits(entry.value(QStringLiteral("units")).toString());
        if (entry.contains(QStringLiteral("min"))) {
            metaData->setRawMin(entry.value(QStringLiteral("min")).toDouble());
        }
        if (entry.contains(QStringLiteral("max"))) {
            metaData->setRawMax(entry.value(QStringLiteral("max")).toDouble());
        }
        if (entry.contains(QStringLiteral("default"))) {
            metaData->setRawDefaultValue(entry.value(QStringLiteral("default")).toDouble());
        }
        if (entry.contains(QStringLiteral("decimalPlaces"))) {
            metaData->setDecimalPlaces(entry.value(QStringLiteral("decimalPlaces")).toInt());
        }
        if (entry.contains(QStringLiteral("rebootRequired"))) {
            metaData->setVehicleRebootRequired(entry.value(QStringLiteral("rebootRequired")).toBool());
        } else {
            metaData->setVehicleRebootRequiredKnown(false);
        }
        metaData->setVehicleFamily(vehicleFamily);
        metaData->setModelVariants(modelVariants);
        metaData->setControlLaw(controlLaw);
        metaData->setTuningPolicy(tuningPolicy);
        metaData->setUpdatePolicy(updatePolicy);
        metaData->setDependencies(dependencies);
        metaData->setValidationRules(validationRules);
        metaData->setSourceFile(entry.value(QStringLiteral("sourceFile")).toString());
        metaData->setSourcePath(entry.value(QStringLiteral("sourcePath")).toString());
        metaData->setSourceKind(entry.value(QStringLiteral("sourceKind")).toString());
        metaData->setReadOnly(entry.value(QStringLiteral("readOnly")).toBool());
        metaData->setWriteOnly(entry.value(QStringLiteral("writeOnly")).toBool(false));
        _cachedMetaData.insert(name, metaData);
    }
}
