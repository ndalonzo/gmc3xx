# GQ/GMC 3xx Radiation Monitor Sensor Interface for Home Assistant

Forked and heavily borrowed from [gmc320](https://github.com/gi1mic/gmc320)

Tested only on GMC-500+ which uses 32-bit unsigned integers, and 115200 baud rate. Other models may vary.

The device is attached to the HA machine via a USB port, where it will appear as a USB serial device. The code then queries the monitor on a regular period and publishes the returned data to the specified MQTT broker. The baud rate on the monitor should be set to 115200.

## Building / Installation

This HA addon will automatically add the build tools, download the latest source, compile, install and run the gmc3xx application on the local HA machine.

To install, add "https://github.com/ndalonzo/gmc3xx" to the "addon store" repositories and then install from there. 

## System Dependencies

The following should be added to your HA instance before installing this addon:
- Mosquitto broker


## Addon Configuration

You need to specify the server, user and password for the MQTT broker. A user-name and password is mandatory. The server defaults to localhost:1883.

Note: There is no parameter checking, so if you get anything wrong the add-on will not start.

# Home Assistant Configuation
Once running, two new entities will appear based on the model and serial number of the unit. Navigate to HA > Settings > Entities, and search for "GMC".


## Running

The application will automatically run if a GQ radiation monitor is attached via serial USB serial port and a configured MQTT broker is available.
The GQ radiation monitor is polled every 60 seconds unless the "repeat" option is changed.
