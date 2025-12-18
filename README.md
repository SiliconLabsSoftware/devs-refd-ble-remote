# Bluetooth remote control  
This reference design is for implementing a wireless Bluetooth Low Energy (BLE) remote control system. It demonstrates a complete solution with a bootloader for firmware updates, a transmitter application for command transmission, and a (test) receiver for processing the remote control commands.  

The demo contains the following projects:
 - [Transmitter bootloader](projects/transmitter_btl/readme.md)  
    The transmitter bootloader enables over-the-air (OTA) firmware updates, ensuring the remote control stays up-to-date.  
 - [Transmitter application](projects/transmitter_app/readme.md)  
    This is the main project, it's a modern remote with the following features:
    - BLE,
    - InfraRed transmitter,
    - accelerometer,
    - gyroscope (won't be available in the first releases),
    - PDM microphone(s) (for Voice recording and forwarding via BLE),
    - up to 48 button support,
    - LED backlight,
    - activity LED.
 - [Receiver](projects/receiver/readme.md)  
   Test application to verify the correctness of the transmitter application and bootloader.  
 - [Receiver on Host](projects/receiver_on_host/README.md)  
   Desktop tool (Python-based) that acts as a BLE receiver on Windows, macOS, or Linux. Features include:
   - BLE scanning and connection using Bleak
   - ADPCM audio reception, decoding, and WAV file writing
   - Key press and battery status display
   - Optional IR capture using Flirc tools
   - Optional AI analysis using Gemini API
   - Cross-platform Tkinter-based UI  

## Hardware
### Requirements
 - Transmitter:
   - For the first product increment (PI1, until the final board):  
      - BG22 Thunderboard (BRD4184A)
   - For the final solution (PI2):
      - Custom BLE remote transmitter board (BRD9402)
 - Receiver:
   - BRD4187C
   - A custom expansion board.
 - WSTK (BRD4001 or BRD4002)
   1 or 2 is needed depending on the product increment. 1 for the custom transmitter (not for the Thunderboard) and 1 for the receiver (BRD4187C).

### Setup
The Thunderboard interface is described [here](projects/transmitter_app/readme.md#pin-mapping).  
The custom transmitter board will have a Mini Simplicity connector to use with the WSTK.  
During debugging the switch position of BRD9402 shall be DBVDD position, during regular operation it shall be in the BVDD position.

As per the receiver the BRD4187C needs to be put on the WSTK and the expansion board will be on the left of the WSTK, connected directly to the expansion header.  
If the expansion board is not available, then the connection is detailed [here](projects/receiver/readme.md#hardware-details).

## Build
Build system dependency (these are installed if Simplicity Studio is installed):
 - CMAKE
 - Make
 - Ninja
 - ARM GCC compiler

### Build types
2 type of firmware "debug" and "release" can be built on every project.  
As usual debug is intended for development and release is for production.  
When debug is selected "DEBUG" is defined with the value 1, otherwise it is not defined.  

### Build with Make
To build every binary you shall run (in the root folder)
```shell
make
```

The following build targets are available:
 - `all`  
    Every project in this repository, this is the default.  
 - `transmitter_btl`  
    Will build bootloader/apploader for the transmitter application.  
 - `transmitter_app`  
    Will build BLE remote transmitter application.  
 - `receiver`
    Will build the test receiver of the BLE remote.  

The following arguments can be also added:
 - `TARGET`: What shall be the selected project target. Can be: `clean_build`, `configure_build`, `clean`, `configure` or `build`.  
             The `build` can be used for incremental building of a project but `configure` step must be at least once executed before this.
             The `clean_build` will execute clean, configure and build in this order.
             The `configure_build` will execute configure and build in this order this is the default selection.
- `TYPE`: What shall be the selected project build type. Can be: `debug`, or `release`.

Example:
```shell
make tag TARGET=build
```

### Build with Docker
Dockerfile is also present in the root folder, to use it the following shall be done:
 1. Install Docker: https://docs.docker.com/engine/install/
 2. Build a docker image. (Default platform is x86_64.)  
    `docker build . -t build-env:latest`  
    You can also use the Makefile to do this operation. This can also recognize Apple Silicon (aarch64).  
    `make docker_image`  
 3. Build the image  
    `docker run --rm -v .:/home -w /home build-env:latest /bin/sh -c "make all"`  
    With Makefile:  
    `make docker_build`  

## Flashing
Can be flashed on any host with Simplicity Commander (or with any compatible ARM debugger).
Simplicity Commander guide: https://www.silabs.com/documents/public/user-guides/ug162-simplicity-commander-reference-guide.pdf

## Debug
[Ozone](https://www.segger.com/products/development-tools/ozone-j-link-debugger/) is the suggested debugger.  
User guide can be found here: https://www.segger.com/downloads/jlink/UM08025_Ozone.pdf

## Testing and Development Tools
The remote has 1 special/dedicated button which is the "Voice/Mic" button.  
When this button is pressed the remote will start recording audio from the PDM microphone(s) and will send the audio data via BLE to a connected receiver (if any).

### Receiver on Host
The [Receiver on Host](projects/receiver_on_host/README.md) is a Python-based desktop application that provides an alternative way to test and interact with the BLE remote transmitter. It offers a user-friendly GUI and can run on Windows, macOS, or Linux without requiring embedded hardware.  
For detailed setup instructions, configuration options, and feature documentation, see the [Receiver on Host README](projects/receiver_on_host/README.md) or check directly the [Setup chapter](projects/receiver_on_host/README.md#Setup).

### BLE Voice Testing with Simplicity SDK
Alternatively, the BLE voice transmission can be verified with a PC application which is present in the Simplicity SDK.  
The project name is "BT Host Voice", default location (Simplicity SDK relative path): `app/bluetooth/example_host/bt_host_voice/`.  
A How-To can be found in the voice SoC application example: `app/bluetooth/example/bt_soc_voice/readme.md`.  

Note that the MAC address of the remote transmitter shall be provided to the PC application, otherwise it won't be able to connect to the remote transmitter.  
If encoding is enabled then `SYS_CNF_VOICE_ENCODE_SEND_STATE` shall be 0 because the PC application does not support the encoding state (only the remote receiver but for that this macro shall be 1). If encoding is disabled (add `-e` flag) then `SYS_CNF_VOICE_ENCODE_SEND_STATE` can be 0 or 1, it does not matter.

Example commands:
```shell
  ./exe/bt_host_voice -u /dev/tty.usbmodem0004402543721 -a 34:25:B4:10:AE:A9 -s 16000 -o brd9402_test_audio
  cd scripts/raspbian ; sh convert.sh ; cd ../..
```
The current path in the example is `<SDK_PATH>/app/bluetooth/example_host/bt_host_voice/`.  
The convert.sh script will search for the audio data on the Desktop by default so you need to copy the audio file there or modify the script to search in the current directory.

## Libraries
- Simplicity SDK 2024.12.2

## License
 - See the [LICENSE.md](./LICENSE.md) file for details.
 - If ST accelerometer is used (LIS2Dx12), [this BSD 3 license of ST](./projects/transmitter_app/drivers/accelerometer/lis2dx12/LICENSE.txt) must also apply.
