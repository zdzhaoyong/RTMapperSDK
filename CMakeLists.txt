cmake_minimum_required(VERSION 2.8)
project(RTMapperSample)

if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Release)
endif()

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/RTMapperSDK)
include(cmake/PICMake.cmake)

pi_collect_packages(VERBOSE REQUIRED Qt)

if(QT4_FOUND)
    qt4_wrap_ui(UI_OPM example/opmapcontrol/mapwidget/mapripform.ui)
elseif(QT5_FOUND)
    qt5_wrap_ui(UI_OPM example/opmapcontrol/mapwidget/mapripform.ui)
endif()

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    set(CMAKE_CXX_FLAGS "-D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin/vs)
endif()

include_directories(src/RTMapper ${PROJECT_BINARY_DIR})
pi_add_target(sample BIN example ${UI_OPM} REQUIRED Qt RTMapperSDK System)
pi_report_target()
