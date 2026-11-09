#pragma once

#include "ParameterMetaData.h"

/// Reads the Flight parameter catalog generated from config/*.yaml.  The JSON
/// uses a flat "parameters" array with Chinese titles, groups and units so the
/// standard QGC parameter editor shows the project's own metadata.
class FlightParameterMetaData : public ParameterMetaData
{
    Q_OBJECT

public:
    explicit FlightParameterMetaData(QObject *parent = nullptr);
    ~FlightParameterMetaData() override;

protected:
    void parseParameterJson(const QJsonObject &json) override;
};
