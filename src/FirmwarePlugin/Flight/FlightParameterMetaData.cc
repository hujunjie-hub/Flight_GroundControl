#include "FlightParameterMetaData.h"

#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QHash>
#include <QtCore/QStringList>

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
    const QJsonArray parameters = json.value(QStringLiteral("parameters")).toArray();

    // Reject every occurrence of an ambiguous name, not merely the second one.
    QHash<QString, int> nameOccurrences;
    for (const QJsonValue &value : parameters) {
        if (value.isObject()) {
            const QString name = value.toObject().value(QStringLiteral("name")).toString().trimmed();
            if (!name.isEmpty()) {
                ++nameOccurrences[name];
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

        if (!validNumericField(entry, QStringLiteral("min")) ||
            !validNumericField(entry, QStringLiteral("max")) ||
            !validNumericField(entry, QStringLiteral("default")) ||
            (entry.contains(QStringLiteral("rebootRequired")) &&
             !entry.value(QStringLiteral("rebootRequired")).isBool()) ||
            (entry.contains(QStringLiteral("decimalPlaces")) &&
             (!entry.value(QStringLiteral("decimalPlaces")).isDouble() ||
              entry.value(QStringLiteral("decimalPlaces")).toInt(-1) < 0 ||
              entry.value(QStringLiteral("decimalPlaces")).toInt(-1) > 9))) {
            qCWarning(FlightParameterMetaDataLog)
                << "Ignoring parameter metadata with invalid numeric fields:" << name;
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
        metaData->setVehicleRebootRequired(entry.value(QStringLiteral("rebootRequired")).toBool(false));
        metaData->setReadOnly(false);
        _cachedMetaData.insert(name, metaData);
    }
}
