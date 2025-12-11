# Joba_Solplanet

This project was designed for the ESP32 microcontroller to utilizes a Modbus library for RS485 communication and PubSub library to publish results on an mqtt broker. 
It now implements communication with AISWEI Inverters (aka Solplanet) via ModbusTCP as described in api reference document 2.1.1.

## Project Structure

- **platformio.ini**: Configuration file for PlatformIO, specifying target settings, defines and libraries.
- **CMakeLists.txt**: Configuration file for CMake on Linux, specifying defines and libraries.
- **src/**: Contains the source code files.
  - **main.cpp**: The main entry point of the application, initializing the ESP32 and handling Modbus communication.
  - **modbus_registers.h**: datatypes and prototypes for the Solplanet inverter modbus interface.
  - **modbus_registers.cpp**: implements the Solplanet inverter modbus interface functions.

## PlatformIO Setup Instructions (not relevant, use linux cmake)

1. **Install PlatformIO**: Ensure you have PlatformIO installed in your development environment (I use the VS Code extension).
2. **Clone the Repository**: Clone this project to your local machine.
3. **Open in PlatformIO**: Open the project in the IDE.
4. **Install Dependencies**: Should work automatically with VS Code and the platformio extension
5. **Upload to ESP32**: Connect your ESP32 board via serial and upload the code as usual.

## Usage

After compiling code for Linux, the program will start receiving modbus responses over TCP.
The responses are then published on an mqtt broker.

## TODO

* Useful for testing: subscribe on a command topic receiving function and parameters to execute.
* Only on request is done. After processing the first result nothing happens (should send request in a loop)
* Numbers are always zero (correct number of bytes, all zero), Strings work
Finally: 
* implement intelligent logic to operate and monitor the inverter automatically
* For monitoring poll status and write to influxdb (visualize with grafana)

## License

This project is licensed under the GPL V3 License.