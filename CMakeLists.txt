cmake_minimum_required(VERSION 3.31)
project(Engine_Core)
set(CMAKE_CXX_STANDARD 23)

find_package(glfw3 CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLEW REQUIRED glew)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)


# ======================================================
# =============== EnTT (ECS) SETUP =====================
# ======================================================

include(FetchContent)

FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.13.0
)

FetchContent_MakeAvailable(entt)


# ======================================================
# =============== IMGUI SETUP ==========================
# ======================================================

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        ${CMAKE_SOURCE_DIR}/include/GLcommon.h
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw "-framework OpenGL")

# ======================================================
# =============== Project Sources ======================
# ======================================================

file(GLOB_RECURSE SRC_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp

)

add_executable(Engine_Core
        ${SRC_FILES}
        vendor/stb/stb_image.cpp

)

target_compile_definitions(Engine_Core PRIVATE
        PROJECT_DIR="${CMAKE_SOURCE_DIR}"
)

#add_compile_definitions(ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")


# ======================================================
# =============== Include Directories ==================
# ======================================================
add_subdirectory(vendor/yaml-cpp)

target_link_libraries(Engine_Core PRIVATE yaml-cpp)

target_include_directories(Engine_Core PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/vendor
        ${CMAKE_SOURCE_DIR}/vendor/imgui
        ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
        ${GLEW_INCLUDE_DIRS}
)

# ======================================================
# =============== Link Libraries =======================
# ======================================================

if(APPLE)
    if (EXISTS "/opt/homebrew/lib/libGLEW.dylib")
        target_link_libraries(Engine_Core PRIVATE
                glfw
                "-framework OpenGL"
                /opt/homebrew/lib/libGLEW.dylib
                imgui
                EnTT::EnTT
                yaml-cpp
        )
    else()
        target_link_libraries(Engine_Core PRIVATE
                glfw
                ${GLEW_LIBRARIES}
                "-framework OpenGL"
                imgui
                EnTT::EnTT
                yaml-cpp
        )
    endif()
else()
    target_link_libraries(Engine_Core PRIVATE
            glfw
            ${GLEW_LIBRARIES}
            OpenGL::GL
            imgui
            EnTT::EnTT
            yaml-cpp
    )
endif()

if(APPLE)
    target_link_libraries(Engine_Core PRIVATE m)
endif()