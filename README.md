This is a work in progress project to connect several ESP32 using SPI connection.
Each board acts as a spi-master of the next board and as a spi-slave of the previous board, forming a sort of ring.

# Install repo
Version required: ESP-IDF v5.1.2
Clone this repo and cd into it to run:
```bash
get_idf
idf.py build
```
It will take a while while it builds all the libraries

# GPIO - Connections for 2 ESP32 - Physical ports ring connections

```
MASTER_GPIO_MOSI 23 <-> SLAVE_GPIO_MOSI 13
MASTER_GPIO_SCLK 18 <-> SLAVE_GPIO_SCLK 14
MASTER_GPIO_CS 5 <-> SLAVE_GPIO_CS 15
```
And viceversa.

# IDF configuration

```
CONFIG_LWIP_L2_TO_L3_COPY=y (Enable copy between Layer2 and Layer3 packets)
CONFIG_LWIP_IP_FORWARD=y (Enable IP forwarding)
CONFIG_LWIP_NETIF_LOOPBACK is not set (Support per-interface loopback - Replace default route, that's why it must be disabled)
LWIP_IP_DEBUG=y
```

# Own Architecture diagram
https://drive.google.com/file/d/1VNsu5GcJccJ-3FP7wIq0gd38yYbneho3/view?usp=sharing


# ESP32 Component Testing Guide

This guide explains how to add unit tests to your ESP32 component and how to run them using the ESP-IDF unit testing framework.

## Adding Tests to Your Component

1. Create a `test` directory in your component folder:

   ```
   your_component/
   ├── include/
   ├── your_component.c
   ├── CMakeLists.txt
   └── test/
       ├── test_your_component.c
       └── CMakeLists.txt
   ```

2. Write your test cases in `test_your_component.c`:

   ```c
   #include "unity.h"
   #include "your_component.h"

   TEST_CASE("Test case description", "[your_component]")
   {
       // Your test code here
       TEST_ASSERT_EQUAL(expected, your_function());
   }
   ```

3. Create a `CMakeLists.txt` file in the `test` directory:

   ```cmake
   idf_component_register(SRC_DIRS "."
                          INCLUDE_DIRS "."
                          REQUIRES your_component unity)
   ```

## Modifying the Unit Test App

1. Navigate to the unit test app directory:

   ```
   cd $IDF_PATH/tools/unit-test-app
   ```

2. Modify the `CMakeLists.txt` file to include your component's directory:

   ```cmake
   list(APPEND EXTRA_COMPONENT_DIRS "/path/to/your/components/directory")
   ```

   Replace `/path/to/your/components/directory` with the actual path to your components.


## Running the Tests


1. Navigate to the unit test app directory:

   ```
   cd $IDF_PATH/tools/unit-test-app
   ```

2. Configure the project:

   ```
   idf.py menuconfig
   ```

   Ensure that "Component config" -> "Unity unit testing library" -> "Enable unit tests" is enabled.

3. Build the unit test app:

   ```
   idf.py -T build all build
   ```
   or 

   ```
   idf.py -T build your_component build
   ```

4. Flash the app to your ESP32:

   ```
   idf.py -p PORT flash
   ```

   Replace `PORT` with your ESP32's port (e.g., `/dev/ttyUSB0`).

5. Run the tests:

   ```
   idf.py -p PORT monitor
   ```

   In the monitor, press 't' to see the test menu and select your tests to run.

## Further Reading


For more detailed information on ESP-IDF's build system and component structure, refer to the official ESP-IDF Programming Guide:

- [ESP-IDF Build System](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html#cmake-file-globbing)
- [Unit Testing in ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/unit-tests.html)
