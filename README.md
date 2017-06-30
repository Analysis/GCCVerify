# GCCVerify - A utility for verifying microcontroller-based Gamecube controller mods.
GCCVerify is a simple Java utility by http://github.com/kaysond that reads JSON-encoded parameters from a serial port, compares them to a manifest of permitted mods and thier values, then downloads the microcontroller program memory and compares against a local binary.

## Features
* Manifest file allows the user to easily adjust permitted firmwares, modifications, and mod values
* Automatic manifest updates from github repo
* Automatic firmware binary downloads from github repo
* Developers can easily add new firmwares by submitting github pull requests

## Supported platforms
* Arduino (specifically Arduino Nano or any atmega328p equivalent. Other platforms can be added easily)
* Windows
* Should theoretically work on Mac/Linux if run via the JRE directly, but it hasn't been tested.

## Instructions
1. Download the latest release zip ("Releases" link above)
2. Unzip the entire contents to a folder
3. Plug in the Arduino, wait for driver install if necessary
4. Run GCCVerify.exe

## Run Requirements
These are all included in release zips (and can be found in the build folder)
* Java 8.0 or newer, 64bit
* avrdude, libusb0.dll, and an avrdude.conf
* GCCVerify, jssc, and gson jar files
* Manifest file and firmware binaries (included in zips, also downloaded automatically except in offline mode)
* Appropriate drivers for whatever USB serial chip is used. On most systems, these will be installed automatically by Windows when you plug a device in for the first time (internet connection may be required)

## Build Requirements
* [jssc 2.8.0](https://github.com/scream3r/java-simple-serial-connector)
* [gson 2.8.1](https://github.com/google/gson)
* [launch4j](http://launch4j.sourceforge.net/)
* JDK 1.8.0 64bit

## Build Instructions
1. Compile the source (e.g. `javac -cp "build\jars\*" src\GCCVerify\*.java`)
2. Create GCCVerify.jar with the included manifest (e.g. `jar cvfm build\jars\GCCVerify.jar src\META-INF\MANIFEST.MF src\GCCVerify\*.class`)
3. Run launch4j on config.xml (e.g. `cd build & launch4jc config.xml`)
Note: The latest GCCVerify.jar is included in the repo so steps 1 and 2 can be skipped if desired

## Command Line Options
* -d : Enables debug mode
* -o : Enables offline mode (does not attempt to update manifest or firmware binaries from github)

## Pull Requests
Pull requests welcome. To add your firmware to the manifest, please submit a pull request modifying only lib/manifest.json and adding only your firmware binary. Hash and filesize can be easily found by adding the firmware to your local manifest and enabling debug mode. Ensure the manifest timestamp is correctly updated (unix timestamp in seconds). You must include a link to your source code.

## Microcontroller parameter validation specification
On boot, the microcontroller serial port must be configured to 9600baud, 8 data bits, 1 stop bit, and no parity. The utility will attempt to reboot the microcontroller, wait 1 second (for bootloaders), then send "GCCVerify" every 250ms for 1s. The microcontroller should accept the magic string "GCCVerify". It is advisable that the microcontroller do not send any serial data until at least 1s after boot, though the utility will attempt to ignore extraneous communication before the JSON object Upon receiving the string, the microcontroller should respond with a JSON-encoded object terminated by "\r\n". The object should contain the following:

* name: String - the firmware name
* major_version: int - the firmware major version number
* minor_version: int - the firmware minor version number
* modSpecs: ModSpec[] - array of objects - any number of ModSpec objects

ModSpec:
* name: String - name of the modification
* enabled: bool - if the mod is active or not
* valueSpecs: ValueSpec[] - array of objects - any number of ValueSpec objects

ValueSpec:
* name: String - name of the value
* value: int - the actual value

See below for allowed mod names and their values.

Check the examples folder for a compliant version of the v2 Hax/WatchingTime firmware

## Mods
To be completed. See [build/lib/manifest.json](build/lib/manifest.json)