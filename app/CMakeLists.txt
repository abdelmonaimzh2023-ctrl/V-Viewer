cmake_minimum_required(VERSION 3.18.1)
project("V-Viewer")

# ========== إعدادات C++ ==========
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== تحسينات الأداء ==========
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto -ffast-math")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a -mtune=cortex-a76")

# ========== المكتبات ==========
find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)

# ========== ملفات المصدر ==========
add_library(vviewer SHARED
    src/main/cpp/main.cpp
    src/main/cpp/wayland_client.cpp
    src/main/cpp/renderer.cpp
    src/main/cpp/theme.cpp
    src/main/cpp/animations.cpp
    src/main/cpp/window_manager.cpp
)

# ========== ربط المكتبات ==========
target_link_libraries(vviewer
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv3-lib}
)

# ========== تضمين ملفات الرأس ==========
target_include_directories(vviewer PRIVATE
    src/main/cpp
    src/main/cpp/shaders
)
