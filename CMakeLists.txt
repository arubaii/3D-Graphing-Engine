cmake_minimum_required(VERSION 3.31)
project(3D_Graphing_Engine)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

set(CMAKE_CXX_STANDARD 23)

if(NOT EMSCRIPTEN)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLEW REQUIRED glew)
    find_package(glm CONFIG REQUIRED)
endif()

include(FetchContent)

FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.13.0
)

if(EMSCRIPTEN)
    FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 1.0.1
    )
endif()

FetchContent_MakeAvailable(entt)

if(EMSCRIPTEN)
    FetchContent_MakeAvailable(glm)
endif()

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

file(GLOB_RECURSE SRC_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(3D_Graphing_Engine
        ${SRC_FILES}
        vendor/stb/stb_image.cpp
)

target_compile_definitions(3D_Graphing_Engine PRIVATE
        PROJECT_DIR="${CMAKE_SOURCE_DIR}"
)

add_subdirectory(vendor/yaml-cpp)

target_link_libraries(3D_Graphing_Engine PRIVATE yaml-cpp EnTT::EnTT imgui)

if(EMSCRIPTEN)
    target_link_libraries(3D_Graphing_Engine PRIVATE glm::glm)
endif()

target_include_directories(3D_Graphing_Engine PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/vendor
        ${CMAKE_SOURCE_DIR}/vendor/imgui
        ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
)

if(NOT EMSCRIPTEN)
    target_include_directories(3D_Graphing_Engine PRIVATE
            ${GLEW_INCLUDE_DIRS}
    )
endif()

if(EMSCRIPTEN)
    target_compile_definitions(3D_Graphing_Engine PRIVATE __EMSCRIPTEN__)
    target_compile_options(3D_Graphing_Engine PRIVATE
            "-fexceptions"
    )
    target_link_options(3D_Graphing_Engine PRIVATE
            "-fexceptions"
            "-sUSE_GLFW=3"
            "-sFULL_ES3=1"
            "-sUSE_WEBGL2=1"
            "-sWASM=1"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sASSERTIONS=2"
            "-sSTACK_OVERFLOW_CHECK=2"
            "-gsource-map"
            "-sNO_DISABLE_EXCEPTION_CATCHING"
            "--preload-file" "${CMAKE_SOURCE_DIR}/shaders@/shaders"
            "--shell-file" "${CMAKE_SOURCE_DIR}/build-web/shell.html"
    )
else()
    target_link_libraries(imgui PUBLIC glfw "-framework OpenGL")
    target_link_libraries(3D_Graphing_Engine PRIVATE OpenGL::GL ${GLEW_LIBRARIES})
    if(APPLE)
        if (EXISTS "/opt/homebrew/lib/libGLEW.dylib")
            target_link_libraries(3D_Graphing_Engine PRIVATE
                    glfw
                    "-framework OpenGL"
                    /opt/homebrew/lib/libGLEW.dylib
                    m
            )
        else()
            target_link_libraries(3D_Graphing_Engine PRIVATE
                    glfw
                    ${GLEW_LIBRARIES}
                    "-framework OpenGL"
                    m
            )
        endif()
    else()
        target_link_libraries(3D_Graphing_Engine PRIVATE
                glfw
                ${GLEW_LIBRARIES}
                OpenGL::GL
        )
    endif()
endif()