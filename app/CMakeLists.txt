cmake_minimum_required(VERSION 3.18.1)
project("V-Viewer")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -funroll-loops -fomit-frame-pointer -march=armv8-a")

# المكتبات المدمجة (Wayland, EGL, إلخ)
add_library(wayland-client SHARED IMPORTED)
set_target_properties(wayland-client PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libwayland-client.so.0)

add_library(wayland-egl SHARED IMPORTED)
set_target_properties(wayland-egl PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libwayland-egl.so.1)

find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)

# ملفات المصدر
add_library(vviewer SHARED
    src/main/cpp/main.cpp
    src/main/cpp/wayland_client.cpp
    src/main/cpp/renderer.cpp
    src/main/cpp/theme.cpp
    src/main/cpp/animations.cpp
    src/main/cpp/window_manager.cpp
)

# ربط المكتبات
target_link_libraries(vviewer
    wayland-client
    wayland-egl
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv3-lib}
)

target_include_directories(vviewer PRIVATE
    src/main/cpp
    src/main/cpp/shaders
)
