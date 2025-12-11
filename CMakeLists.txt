cmake_minimum_required(VERSION 3.10)
project(Joba_Solplanet)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PahoMqttCpp REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/modbus_registers.cpp
)

# Create executable
add_executable(joba_solplanet ${SOURCES})

# Link libraries
target_link_libraries(joba_solplanet PRIVATE 
    paho-mqttpp3
    paho-mqtt3as
)

# Include directories
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
target_include_directories(joba_solplanet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
configure_file(mqtt_config.h.in mqtt_config.h)
configure_file(modbus_config.h.in modbus_config.h)