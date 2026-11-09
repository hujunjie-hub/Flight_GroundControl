# MockLink is required by VehicleTest and other integration fixtures in all
# configurations when QGC_BUILD_TESTING is enabled. Keep it in Debug builds
# even when tests are disabled, preserving the developer Debug workflow.
if(NOT _QGC_DEBUG_BUILD AND NOT QGC_BUILD_TESTING)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MockConfiguration.cc
        MockConfiguration.h
        MockLink.cc
        MockLink.h
        MockLinkCamera.cc
        MockLinkCamera.h
        MockLinkFTP.cc
        MockLinkFTP.h
        MockLinkGimbal.cc
        MockLinkGimbal.h
        MockLinkWorker.cc
        MockLinkWorker.h
        MockLinkMissionItemHandler.cc
        MockLinkMissionItemHandler.h
        MockLinkPX4Calibration.cc
        MockLinkPX4Calibration.h
)

set_source_files_properties(MockLink.General.MetaData.json PROPERTIES QT_RESOURCE_ALIAS General.MetaData.json)
set_source_files_properties(MockLink.General.MetaData.json.xz PROPERTIES QT_RESOURCE_ALIAS General.MetaData.json.xz)
set_source_files_properties(MockLink.Parameter.MetaData.json.xz PROPERTIES QT_RESOURCE_ALIAS Parameter.MetaData.json.xz)
set_source_files_properties(MockLink.Parameter.MetaData.json PROPERTIES QT_RESOURCE_ALIAS Parameter.MetaData.json)
qt_add_resources(${CMAKE_PROJECT_NAME} mocklink_resources
    PREFIX "/MockLink"
    FILES
        GenericMockLink.params
        PX4MockLink.params
        MockLink.General.MetaData.json
        MockLink.General.MetaData.json.xz
        MockLink.Parameter.MetaData.json.xz
        MockLink.Parameter.MetaData.json
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE "MockLink.h")
