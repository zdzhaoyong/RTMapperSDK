cmake_minimum_required(VERSION 2.8)
project(RTMapperSample)

# force to build with type of Release
set(CMAKE_BUILD_TYPE Release)
message("-- Build type: " ${CMAKE_BUILD_TYPE})


LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/RTMapperSDK)
include(cmake/PICMake.cmake)


if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--retain-symbols-file=${CMAKE_CURRENT_LIST_DIR}/cmake/static_export.sym")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_LIST_DIR}/cmake/shared_export.map")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin/lnx)
else()
    set(CMAKE_CXX_FLAGS "-D_CRT_SECURE_NO_WARNINGS /EHsc")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin/vs)
endif()

include_directories(src/RTMapper ${PROJECT_BINARY_DIR})


# if you do not want to build Qt example then comment the following line
set(BUILD_EXAMPLE_QT TRUE)

if( BUILD_EXAMPLE_QT )
    pi_collect_packages(VERBOSE REQUIRED Qt)

    if( QT4_FOUND OR QT5_FOUND )
        if(QT4_FOUND)
            qt4_wrap_ui(UI_OPM example-qt/opmapcontrol/mapwidget/mapripform.ui)
        elseif(QT5_FOUND)
            qt5_wrap_ui(UI_OPM example-qt/opmapcontrol/mapwidget/mapripform.ui)
        endif()

        pi_add_target(example-qt BIN example-qt libs ${UI_OPM} REQUIRED Qt RTMapperSDK System)
    endif()
endif()

# console example
pi_add_target(example-console BIN example-console libs REQUIRED RTMapperSDK System)

# fitgps example
pi_add_target(example-fitgps BIN example-fitgps libs REQUIRED RTMapperSDK System)

pi_report_target()

