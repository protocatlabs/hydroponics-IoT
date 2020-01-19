# hydroponics_IoT
Arduino code for the embedded software of the hydropinics kit

## Step 1: Blink the LED

First, the drivers need to be installed. Adafruit does a nice tutorial on it https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide

1. Install this https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers which enables you to connect to the drivers
2. Enter http://arduino.esp8266.com/stable/package_esp8266com_index.json into Additional Board Manager URLs field in the Arduino v1.6.4+ preferences.
- Next, use the Board manager to install the ESP8266 package.
3. Run `basic_testing_scripts/huzzah_blink` to see if you can blink the red LED


## Step 2: Connect to Wifi

1. Open `basic_testing_scripts/Huzzah_WifiConnect`
2. modify lines 8 and 9 to have your local wifi and password
3. Check the serial monitor. It should print success upon successful connection

## Step 3: Try run device and push data to thingspeak

1. Install `Ezo_I2c_lib`. This can be downloaded as a zip file from https://github.com/Atlas-Scientific/Ezo_I2c_lib and then in Arduino go Go Sketch>IncludeLibrary>UploadZip, and then select the above zip file. Check this guide for more information https://www.arduino.cc/en/guide/libraries
2. Create a Thing Speak Account and follow Step 6/7 in this tutorial to set it up https://www.instructables.com/id/Wifi-Hydroponics-Meter/. You don't need to do any widget dragging, it should automatically set up all the dashboards
3. Add ThingSpeak Library. https://www.arduinolibraries.info/libraries/thing-speak. Download the zip file, and build it the same way as you did the `Ezo_I2C:lib`.
4. Check that `ESP8266 Wifi library` is installed. If not go `Sketch -> Include library -> Manage libraries`, search for esp8266wifi, and install it.
5. Run `basic_testing_scripts/thingspeak_version/thingspeak_example.ino` and modify lines 10-13 to add wifi credentials and thingspeak credentials

## Step 4: Build a JSON with the data

1. Install ArduinoJSON https://arduinojson.org/v6/doc/installation/
2. Build a JSON file. see `basic_testing_scripts/json_test` for an example in building a JSON
3. Once this is working try modify the code in Step3 to include this JSON structure. Have a look at this gitdiff to see what PJ did https://github.com/protocatlabs/hydroponics-IoT/commit/b6723876c638d537e3c5298dbbe24212ed17c6fe

## Step 5: Get the time via Network Time Protocol (NTP)

1. Install `ntpclient` by Fabrice Weinberg using the arduino library manager. See this tutorial https://lastminuteengineers.com/esp8266-ntp-server-date-time-tutorial/

2. See the above tutorial and gitdiff to see how PJ modified the code to include time https://github.com/protocatlabs/hydroponics-IoT/commit/b6723876c638d537e3c5298dbbe24212ed17c6fe

In header:

```
#include <NTPClient.h> 
#include <WiFiUdp.h>

// Define NTP Client to get time
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 3600; // UTC offset for time in seconds
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
```


In void setup():
```
timeClient.begin()
```

In void loop():
```
timeClient.update();
Serial.println(timeClient.getEpochTime()); // to print the UNIX time
```

### Step 6: POST http request
https://arduinojson.org/v6/how-to/use-arduinojson-with-esp8266httpclient/

I used a API key from https://requestbin.com/ for debugging


Issues: This method currently works easily with HTTP requests, but has issues with HTTPS requests.

