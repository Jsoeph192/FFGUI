cmake_minimum_required(VERSION 3.16)
project(FFGUI++ VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Concurrent)

add_executable(FFGUI++
src/main.cpp
src/MainWindow.cpp
src/MainWindow.h
src/FFmpegProcessor.cpp
src/FFmpegProcessor.h
src/PresetManager.cpp
src/PresetManager.h
src/utils.cpp
src/utils.h
resources.qrc
app.rc
src/MediaPlayer.h
src/MediaPlayer.cpp
)

target_link_libraries(FFGUI++ PRIVATE
Qt6::Core
Qt6::Widgets
Qt6::Network
Qt6::Concurrent
)

if(WIN32)
target_link_libraries(FFGUI++ PRIVATE psapi)
endif()

if(WIN32)
if(EXISTS "${CMAKE_SOURCE_DIR}/app.rc")
target_sources(FFGUI++ PRIVATE app.rc)
else()
set(RESOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/app_resources.rc")
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/icons/logo.ico")
file(WRITE ${RESOURCE_FILE} "IDI_ICON1 ICON DISCARDABLE \"../resources/icons/logo.ico\"\n")
target_sources(FFGUI++ PRIVATE ${RESOURCE_FILE})
endif()
endif()
endif()

if(MINGW)
target_compile_options(FFGUI++ PRIVATE -O2)
target_link_options(FFGUI++ PRIVATE -Wl,--gc-sections -Wl,--strip-all)

if(WIN32)
set_target_properties(FFGUI++ PROPERTIES
WIN32_EXECUTABLE ON
)
endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bin")
    file(GLOB BIN_FILES "${CMAKE_SOURCE_DIR}/bin/*")
    foreach(BIN_FILE ${BIN_FILES})
        if(IS_DIRECTORY ${BIN_FILE})
            get_filename_component(DIR_NAME ${BIN_FILE} NAME)
            file(COPY ${BIN_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${DIR_NAME})
        else()
            configure_file(${BIN_FILE} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
        endif()
    endforeach()
endif()


if(WIN32)
get_target_property(QtCore_LOCATION Qt6::Core LOCATION)
get_filename_component(Qt_BIN_DIR "${QtCore_LOCATION}" DIRECTORY)
get_filename_component(Qt_ROOT_DIR "${Qt_BIN_DIR}" DIRECTORY)

message(STATUS "Qt Binary Directory: ${Qt_BIN_DIR}")
message(STATUS "Qt Root Directory: ${Qt_ROOT_DIR}")

set(REQUIRED_DLLS
"Qt6Core.dll"
"Qt6Gui.dll"
"Qt6Widgets.dll"
"Qt6Network.dll"
"Qt6Concurrent.dll"
"libgcc_s_seh-1.dll"
"libstdc++-6.dll"
"libwinpthread-1.dll"
"libmd4c.dll"
"libharfbuzz-0.dll"
"libfreetype-6.dll"
"libpng16-16.dll"
"zlib1.dll"
"libb2-1.dll"
"libdouble-conversion.dll"
"libicuin78.dll"
"libicuuc78.dll"
"libicudt78.dll"
"libpcre2-16-0.dll"
"libzstd.dll"
"libbrotlidec.dll"
"libglib-2.0-0.dll"
"libgraphite2.dll"
"libbz2-1.dll"
"libintl-8.dll"
"libpcre2-8-0.dll"
"libbrotlicommon.dll"
"libiconv-2.dll"
)

foreach(DLL_NAME ${REQUIRED_DLLS})
set(DLL_PATH "${Qt_BIN_DIR}/${DLL_NAME}")
if(EXISTS "${DLL_PATH}")
configure_file("${DLL_PATH}"
"${CMAKE_CURRENT_BINARY_DIR}/${DLL_NAME}"
COPYONLY)
else()
if(EXISTS "/mingw64/bin/${DLL_NAME}")
configure_file("/mingw64/bin/${DLL_NAME}"
"${CMAKE_CURRENT_BINARY_DIR}/${DLL_NAME}"
COPYONLY)
elseif(EXISTS "C:/msys64/mingw64/bin/${DLL_NAME}")
configure_file("C:/msys64/mingw64/bin/${DLL_NAME}"
"${CMAKE_CURRENT_BINARY_DIR}/${DLL_NAME}"
COPYONLY)
endif()
endif()
endforeach()

set(PLUGIN_DIRS "plugins" "share/qt6/plugins" "../share/qt6/plugins")

foreach(PLUGIN_SUBDIR ${PLUGIN_DIRS})
set(PLUGIN_DIR "${Qt_ROOT_DIR}/${PLUGIN_SUBDIR}")
if(EXISTS "${PLUGIN_DIR}")
if(EXISTS "${PLUGIN_DIR}/platforms")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins/platforms")

file(GLOB PLATFORM_DLLS "${PLUGIN_DIR}/platforms/*.dll")
foreach(PLUGIN_FILE ${PLATFORM_DLLS})
get_filename_component(PLUGIN_NAME ${PLUGIN_FILE} NAME)
configure_file("${PLUGIN_FILE}"
"${CMAKE_CURRENT_BINARY_DIR}/plugins/platforms/${PLUGIN_NAME}"
COPYONLY)
endforeach()
endif()

if(EXISTS "${PLUGIN_DIR}/imageformats")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins/imageformats")
file(GLOB IMAGEFORMATS_DLLS "${PLUGIN_DIR}/imageformats/*.dll")
foreach(IMAGEFORMAT_FILE ${IMAGEFORMATS_DLLS})
get_filename_component(IMAGEFORMAT_NAME ${IMAGEFORMAT_FILE} NAME)
configure_file("${IMAGEFORMAT_FILE}"
"${CMAKE_CURRENT_BINARY_DIR}/plugins/imageformats/${IMAGEFORMAT_NAME}"
COPYONLY)
endforeach()
endif()
break()
endif()
endforeach()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
"[Paths]\nPlugins = plugins\n")
endif()

install(TARGETS FFGUI++ RUNTIME DESTINATION bin)

if(EXISTS "${CMAKE_SOURCE_DIR}/bin")
    file(GLOB INSTALL_BIN_FILES "${CMAKE_SOURCE_DIR}/bin/*")
    set(INSTALL_FILES_LIST "")
    set(INSTALL_DIRS_LIST "")
    
    foreach(INSTALL_FILE ${INSTALL_BIN_FILES})
        if(IS_DIRECTORY ${INSTALL_FILE})
            list(APPEND INSTALL_DIRS_LIST ${INSTALL_FILE})
        else()
            list(APPEND INSTALL_FILES_LIST ${INSTALL_FILE})
        endif()
    endforeach()
    
    if(INSTALL_FILES_LIST)
        install(FILES ${INSTALL_FILES_LIST} DESTINATION bin)
    endif()
    
    foreach(INSTALL_DIR ${INSTALL_DIRS_LIST})
        get_filename_component(DIR_NAME ${INSTALL_DIR} NAME)
        install(DIRECTORY ${INSTALL_DIR} DESTINATION bin)
    endforeach()
endif()
