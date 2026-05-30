cmake_minimum_required(VERSION 3.18.1)
project("V-Viewer")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -funroll-loops -fomit-frame-pointer -march=armv8-a")

find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)

# مكتبة native_app_glue من NDK
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue)

# ملفات المصدر الأساسية فقط (بدون wayland_client لتجنب أخطاء الروابط الآن)
add_library(vviewer SHARED
    src/main/cpp/main.cpp
    src/main/cpp/renderer.cpp
    src/main/cpp/theme.cpp
    src/main/cpp/animations.cpp
    src/main/cpp/window_manager.cpp
)

target_link_libraries(vviewer
    native_app_glue
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv3-lib}
)

target_include_directories(vviewer PRIVATE
    src/main/cpp
    src/main/cpp/shaders
)
