target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FlightAutoPilotPlugin.cc
        FlightAutoPilotPlugin.h
        FlightFirmwarePlugin.cc
        FlightFirmwarePlugin.h
        FlightFirmwarePluginFactory.cc
        FlightFirmwarePluginFactory.h
        FlightParameterMetaData.cc
        FlightParameterMetaData.h
        FlightPlaceholderComponent.cc
        FlightPlaceholderComponent.h
        FlightRadioComponent.cc
        FlightRadioComponent.h
        FlightRcMonitor.cc
        FlightRcMonitor.h
        FlightSensorsComponent.cc
        FlightSensorsComponent.h
        FlightVehicleComponent.cc
        FlightVehicleComponent.h
)

qt_add_resources(${CMAKE_PROJECT_NAME} flight_parameter_metadata
    PREFIX "/FirmwarePlugin/Flight"
    FILES
        FlightParameterFactMetaData.json
)

qt_add_resources(${CMAKE_PROJECT_NAME} flight_configure_pages
    PREFIX "/qml/QGroundControl/FirmwarePlugin/Flight"
    FILES
        FlightGimbal.qml
        FlightModesPage.qml
        FlightPlaceholder.qml
        FlightRadio.qml
        FlightSafety.qml
        FlightSensors.qml
        FlightStorage.qml
        FlightSummary.qml
)

qt_add_resources(${CMAKE_PROJECT_NAME} flight_section_icons
    PREFIX "/qmlimages"
    FILES
        FlightStorageIcon.png
)

