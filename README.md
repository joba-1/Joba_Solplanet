# Joba_Solplanet

This linux program implements communication with AISWEI Dongles via ModbusTCP to investigate registers of the Apollo Wallbox. Results are published to Mqtt and InfluxDB.

The project was started for the ESP32 microcontroller to query and publish data from AISWEI/Solplanet inverters as described in api reference document 2.1.1. It utilized a Modbus library for RS485 communication and PubSub library for publishing results to an mqtt broker. Might get back to that later...

## Project Structure

- **platformio.ini**: Configuration file for ESP32, specifying target settings, defines and libraries.
- **CMakeLists.txt**: Configuration file for CMake on Linux, specifying defines and libraries.
- **src/**: Contains the source code files.
  - **main.cpp**: The main entry point of the application, initializing the ESP32 and handling Modbus communication.
  - **modbus_registers.h**: datatypes and prototypes for the Solplanet modbus interface.
  - **modbus_registers.cpp**: implements the Solplanet modbus interface functions.

## Linux Setup Instructions

* zypper install code gcc make cmake git mosquitto influxdb paho-cpp-devel nlohmann_json-devel
* git clone github repo
* edit *_config.h.in files
* build with cmake 
* start mosquitto broker and influxdb service
* influx -execute 'create database joba_solplanet'
* run joba_solplanet
* watch results coming in with
    * influx -database joba_solplanet -precision rfc3339 -execute "select * from joba_solplanet order by time desc limit 10"
    * influx -database joba_solplanet -precision rfc3339 -execute "select * from summary"
    * mosquitto_sub -v -t 'joba_solplanet/#'

## PlatformIO Setup Instructions (not relevant for now, use linux cmake)

1. **Install PlatformIO**: Ensure you have PlatformIO installed in your development environment (I use it as VS Code extension).
2. **Clone the Repository**: Clone this project to your local machine.
3. **Open in PlatformIO**: Open the project in the IDE.
4. **Install Dependencies**: Should work automatically with VS Code and the platformio extension
5. **Upload to ESP32**: Connect your ESP32 board via serial and upload the code as usual.

## Usage

After compiling code for Linux, the program will start receiving modbus responses over TCP.
The responses are then published on an mqtt broker and an Influx database.

## TODO

* Find out what the registers mean
* Use non generic names for registers where meaning is known
Finally: 
* implement intelligent logic to operate and monitor the pv inverter and wallbox automatically
* combine with heat pump control

## License

This project is licensed under the GPL V3 License.
