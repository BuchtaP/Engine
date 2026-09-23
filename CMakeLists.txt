cmake_minimum_required(VERSION 3.20)
project(Engine)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(Engine src/main.cpp)
target_link_libraries(Engine PRIVATE glfw glad::glad glm::glm)

# Zkopiruj assets slozku vedle .exe po kazdem buildu
add_custom_command(
    TARGET Engine
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Engine>/assets
)