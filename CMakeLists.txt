cmake_minimum_required(VERSION 3.3)
project(xframelesswidget
    DESCRIPTION "Qt frameless widget"
    LANGUAGES CXX
)

if(MSVC)
    set(X_WIN TRUE)
elseif(UNIX)
    if(NOT APPLE)
        set(X_LINUX TRUE)
    else()
		set(CMAKE_OSX_ARCHITECTURES "x86_64")
        set(X_MACOS TRUE)
    endif()
endif()

set(Uis 
    captionwidget.ui
)

set(Src
    captionwidget.cpp
	xframelesswidget.cpp
    $<$<BOOL:${X_WIN}>:winnativewindow.cpp>
	$<$<BOOL:${X_MACOS}>:xutil_macos.mm>
    $<$<BOOL:${X_LINUX}>:xutil_linux.cpp>
)

add_library(
    ${PROJECT_NAME}  SHARED
    ${Src}
    ${Uis}
)

target_compile_options(
    ${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8 -D_UNICODE -DUNICODE>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:DEBUG>:/WX>>
	$<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:DEBUG>>:/WX->>

    $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror>
)

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
    -DX_FRAMELESS_WIDGET_SHARED
)

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
if(X_LINUX)
    find_package(Qt5X11Extras REQUIRED)
    find_package(X11 REQUIRED)
endif()
target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets

	$<$<BOOL:${X_WIN}>:Dwmapi>
    $<$<BOOL:${X_LINUX}>:Qt5::X11Extras>
    "$<$<BOOL:${X_LINUX}>:${X11_LIBRARIES}>"
)
if(X_MACOS)
    target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        "-framework AppKit"
    )
endif()
target_include_directories(
    ${PROJECT_NAME} PUBLIC
    .
    "$<$<BOOL:${X_LINUX}>:${X11_INCLUDE_DIR}>"
)

# sampler executable;
set(Sampler ${PROJECT_NAME}_sample)

add_executable(
    ${Sampler} WIN32
    sample.cpp
)

target_compile_options(
    ${Sampler} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:DEBUG>:/WX>>
	$<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:DEBUG>>:/WX->>

    $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror>
)

set_target_properties(
    ${Sampler} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_link_libraries(
    ${Sampler}
    Qt5::Core
    Qt5::Widgets
    ${PROJECT_NAME}
)
