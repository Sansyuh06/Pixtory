cmake_minimum_required(VERSION 3.10)
project(RosslerSimulation)

set(CMAKE_CXX_STANDARD 17)

# Find dependencies installed via vcpkg
find_package(glew CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(RosslerSim Source1.cpp)

# Link the libraries
target_link_libraries(RosslerSim PRIVATE 
    GLEW::GLEW 
    glfw 
    glm::glm 
    imgui::imgui
    opengl32
)
