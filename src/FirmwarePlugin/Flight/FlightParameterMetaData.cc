#include "FlightParameterMetaData.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

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
    for (const QJsonValue &value : parameters) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject entry = value.toObject();
        const QString name = entry.value(QStringLiteral("name")).toString();
        if (name.isEmpty()) {
            continue;
        }

        auto *metaData = new FactMetaData(FactMetaData::valueTypeFloat, this);
        metaData->setName(name);
        metaData->setGroup(entry.value(QStringLiteral("group")).toString());
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
        metaData->setReadOnly(false);
        _cachedMetaData[name] = metaData;
    }
}
