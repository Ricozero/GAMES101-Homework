cmake_minimum_required(VERSION 3.10)

# Application source
set(APPLICATION_SOURCE
    application.cpp
    main.cpp
    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_impl_glfw.cpp
)

# -------------------------------------------------------------------------------
# Set include directories
# -------------------------------------------------------------------------------
include_directories(
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${CGL_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${Stb_INCLUDE_DIR}
)

# -------------------------------------------------------------------------------
# Set link directories
# -------------------------------------------------------------------------------
link_directories(
    ${GLEW_LIBRARY_DIRS}
    ${GLFW_LIBRARY_DIRS}
    ${CGL_LIBRARY_DIRS}
    ${FREETYPE_LIBRARY_DIRS}
)

# -------------------------------------------------------------------------------
# Add executable
# -------------------------------------------------------------------------------
add_executable(ropesim ${APPLICATION_SOURCE})

target_link_libraries(ropesim
    CGL ${CGL_LIBRARIES}
    glew ${GLEW_LIBRARIES}
    glfw ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${CMAKE_THREADS_INIT}
    imgui::imgui
)

# -------------------------------------------------------------------------------
# Platform-specific configurations for target
# -------------------------------------------------------------------------------
if(APPLE)
    set_property(TARGET ropesim APPEND_STRING PROPERTY COMPILE_FLAGS
        "-Wno-deprecated-declarations -Wno-c++11-extensions")
endif(APPLE)

# Put executable in build directory root
set(EXECUTABLE_OUTPUT_PATH ..)

# Install to project root
install(TARGETS ropesim DESTINATION ${RopeSim_SOURCE_DIR})
