cmake_minimum_required(VERSION 3.21)

project(TradeSoft
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebSockets QUIET)
find_package(ZLIB QUIET)

file(GLOB_RECURSE TRADESOFT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

set(TRADESOFT_APP_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
list(REMOVE_ITEM TRADESOFT_SOURCES "${TRADESOFT_APP_MAIN}")

add_library(TradeSoftCore STATIC
    ${TRADESOFT_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/mainwindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/indicatordialog.ui"
)

target_include_directories(TradeSoftCore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(TradeSoftCore PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

if(TARGET Qt${QT_VERSION_MAJOR}::WebSockets AND ZLIB_FOUND)
    target_link_libraries(TradeSoftCore PUBLIC
        Qt${QT_VERSION_MAJOR}::WebSockets
        ZLIB::ZLIB
    )
    target_compile_definitions(TradeSoftCore PUBLIC TRADESOFT_HAS_WEBSOCKETS=1)
else()
    target_compile_definitions(TradeSoftCore PUBLIC TRADESOFT_HAS_WEBSOCKETS=0)
    if(NOT TARGET Qt${QT_VERSION_MAJOR}::WebSockets)
        message(STATUS "Qt WebSockets not found; TradeSoft will use polling realtime")
    endif()
    if(NOT ZLIB_FOUND)
        message(STATUS "zlib not found; TradeSoft will use polling realtime")
    endif()
endif()

add_executable(TradeSoft
    MACOSX_BUNDLE
    "${TRADESOFT_APP_MAIN}"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc"
)

target_include_directories(TradeSoft PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(TradeSoft PRIVATE
    TradeSoftCore
)

set_target_properties(TradeSoft PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.bortongo.tradesoft"
    MACOSX_BUNDLE_BUNDLE_NAME "TradeSoft"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(TradeSoft)
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(TradeSoftTests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_tradesoft.cpp"
    )

    target_link_libraries(TradeSoftTests PRIVATE
        TradeSoftCore
    )

    add_test(NAME TradeSoftTests COMMAND TradeSoftTests)
endif()
