# Joba_Solplanet

This project was designed for the ESP32 microcontroller and utilizes a ModbusRTU library for RS485 communication and PubSub library to publish results on an mqtt broker. Now it has branches for ModbusTCP and Linux. State is alpha, meaning on linux I get results on requests for id=1 and fc=3 but they are probably mostly wrong. 
It implements communication with AISWEI Inverters (aka Solplanet) as described in api reference document 2.1.1.

## Project Structure

- **platformio.ini**: Configuration file for PlatformIO, specifying environment settings and libraries.
- **src/**: Contains the source code files.
  - **main.cpp**: The main entry point of the application, initializing the ESP32 and handling Modbus communication.
  - **modbus_registers.h**: datatypes and prototypes for the Solplanet inverter modbus interface.
  - **modbus_registers.cpp**: implements the Solplanet inverter modbus interface functions.

## Setup Instructions

1. **Install PlatformIO**: Ensure you have PlatformIO installed in your development environment (I use the VS Code extension).
2. **Clone the Repository**: Clone this project to your local machine.
3. **Open in PlatformIO**: Open the project in the IDE.
4. **Install Dependencies**: Should work automatically with VS Code and the platformio extension
5. **Upload to ESP32**: Connect your ESP32 board via serial and upload the code as usual.

## Usage

After uploading the code to the ESP32, the device will start receiving modbus responses over RS485.
The responses are then published on an mqtt broker.

## TODO

* The ESP32 should issue modbus commands via functions from modbus_registers.h
* As a first step implement a web interface to select the function and enter parameters 
* Also useful: subscribe on a command topic receiving function and parameters to execute.
Finally: 
* implement intelligent logic to operate and monitor the inverter automatically
* For monitoring poll status and write to influxdb (visualize with grafana)


## License

This project is licensed under the GPL V3 License.
