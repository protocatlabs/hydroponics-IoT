#include <ArduinoJson.h>                                         //Library for JSON Creation https://arduinojson.org/v6/doc/installation/
#include <Ezo_i2c.h>                                             //EZO I2C library from https://github.com/Atlas-Scientific/Ezo_I2c_lib
#include <Wire.h>                                                //Arduinos i2c library
#include <ESP8266WiFi.h>                                         //esp8266 wifi library https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide
#include <ESP8266HTTPClient.h>                                   //HTTP client, should come automatically when installing the wifi library
#include <WiFiUdp.h>                                             //UDP is required for the NTP Client to get the UNIX time - see NTP link below
#include <NTPClient.h>                                           //NTP Client in order to get Unix Time - https://lastminuteengineers.com/esp8266-ntp-server-date-time-tutorial/
#include "ThingSpeak.h"                                          //include thingspeak library https://www.arduinolibraries.info/libraries/thing-speak


WiFiClient  client;                                              //declare that this device connects to a Wi-Fi network,create a connection to a specified internet IP address



//----------------Fill in Wifi / HTTP or ThingSpeak Credentials -------
const String ssid = "PJ's iPhone";                                 //The name of the Wi-Fi network you are connecting to
const String pass = "password";                             //Your WiFi network password
const long myChannelNumber = 964626;                            //DD- Your Thingspeak channel number
const char * myWriteAPIKey = "Y8JFTDGASMCRF5AS";                 //DD- Your ThingSpeak Write API Key
const String httpAddress = "http://sensing.sg/greenilai/gen2/post_generic.php"; // API key for sending http requests

const String unitID = "000235";                                 // Device ID of the unit
//------------------------------------------------------------------

// Define NTP Client to get time
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 0;                             // UTC offset for time, change if you want a timezone offset
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


Ezo_board PH = Ezo_board(99, "PH");                              //create a pH circuit object, who's I2C address is 99 and name is "PH"
Ezo_board EC = Ezo_board(100, "EC");                             //create a EC circuit object, who's I2C address is 100 and name is "EC"
Ezo_board RTD = Ezo_board(102, "RTD");                           //create an RTD circuit object who's I2C address is 102 and name is "RTD"


enum reading_step {REQUEST_TEMP, READ_TEMP_AND_REQUEST_PH_EC, READ_PH_EC };       //the readings are taken in 3 steps
                                                                                  //step 1 tell the temp sensor to take a reading
                                                                                  //step 2 consume the temp reading and tell the ph and EC to take a reading based on the temp reading we just received 
                                                                                  //step 3 consume the ph and EC readings

enum reading_step current_step = REQUEST_TEMP;                                    //the current step keeps track of where we are. lets set it to REQUEST_TEMP (step 1) on startup     

int return_code=0;                                                                //holds the return code sent back from thingSpeak after we upload the data 
unsigned long next_step_time = 0;                                                 //holds the time in milliseconds. this is used so we know when to move between the 3 steps    

const unsigned long reading_delay = 815;                                          //how long we wait to receive a response, in milliseconds 
const unsigned long loop_delay = 15000;



void setup() {                                                                    //set up the hardware
  Wire.begin();                                                                   //enable I2C port
  Serial.begin(9600);                                                             //enable serial port, set baud rate to 9600
  Serial.println("starting sketch");
  WiFi.mode(WIFI_STA);                                                            //set ESP8266 mode as a station to be connected to wifi network
  ThingSpeak.begin(client);                                                       //DD- enable ThingSpeak connection

  timeClient.begin(); // begin time client
  
  
}


void loop() {
  StaticJsonDocument<300> doc;                                                    // Set up JSON with size 300. If JSON size is increased in the future, increase this number
  
  JsonArray dataArray = doc.createNestedArray("data");
  JsonObject data = dataArray.createNestedObject();
  
  data["type"] = "MFC";
  data["unitid"] = unitID;                                                       // Set unit ID of device for JSON output (defined above)
  data["timestamp"] = timeClient.getEpochTime();                                   // Get UNIX time at UTC and set to timestamp
  JsonArray parameters = data.createNestedArray("parameters");                     // Set a nested array called parameters where data is stored

  // Declare HTTP Client
  HTTPClient http;


  
                                                                         //connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {                                            //if we are not connected (when WL_CONNECTED =1 we have a successful connection)
      WiFi.begin(ssid, pass);
      //initialize wifi connection

    while (WiFi.status() != WL_CONNECTED) {                                       //while we are not connected 
      Serial.print(".");                                                          //print "........connected"
      delay(50);                                                                  //a single dont is printed every 50 ms
    }
    Serial.println("connected");                                                  //once connected print "connected"
  }

  switch(current_step) {                                                          //selects what to do based on what reading_step we are in
//------------------------------------------------------------------
 
    case REQUEST_TEMP:                                                            //when we are in the first step
      if (millis() >= next_step_time) {                                           //check to see if enough time has past, if it has 
        RTD.send_read_cmd();                                                      //request a temp reading
        next_step_time = millis() + reading_delay;                                //advance the next step time by adding the delay we need for the sensor to take the reading
        current_step = READ_TEMP_AND_REQUEST_PH_EC;                               //switch to step 2
      }
      break;                                                                      //break out of this we are done
//------------------------------------------------------------------

    case READ_TEMP_AND_REQUEST_PH_EC:                                             //when we are in the second step
      if (millis() >= next_step_time) {                                           //check to see if enough time has past, if it has   

      timeClient.update();
      Serial.println(timeClient.getEpochTime());
            
        RTD.receive_read_cmd();                                                   //get the temp reading  
        Serial.print(RTD.get_name()); Serial.print(": ");                         //print the name of the circuit we just got a reading from
        
        if((reading_succeeded(RTD) == true) && (RTD.get_last_received_reading() > -1000.0)){  //if the temperature reading has been received and it is valid
          PH.send_read_with_temp_comp(RTD.get_last_received_reading());                       //send readings from temp sensor to pH sensor
          EC.send_read_with_temp_comp(RTD.get_last_received_reading());                       //send readings from temp sensor to EC sensor
        } 
        else                                                                                  //if the temperature reading is invalid
        {
          PH.send_read_with_temp_comp(25.0);                                                  //send default temp = 25 deg C to pH sensor
          EC.send_read_with_temp_comp(25.0);                                                  //send default temp = 25 deg C to EC sensor
        }
        
        if(RTD.get_error() == Ezo_board::SUCCESS){                                            //if the RTD reading was successful
          Serial.print(RTD.get_last_received_reading(), 1);                                   //print the reading (with 1 decimal places)
        }
        
        Serial.print(" ");                                                                    //print a blank space so the output string on the serial monitor is easy to read
        
        next_step_time = millis() + reading_delay;                                            //advance the next step time by adding the delay we need for the sensor to take the reading
        current_step = READ_PH_EC;                                                            //switch to step 3 
      }
      break;                                                                                  //break out of this we are done
//------------------------------------------------------------------

    case READ_PH_EC:                                                                          //when we are in the third step
      if (millis() >= next_step_time) {                                                       //check to see if enough time has past, if it has
        
        PH.receive_read_cmd();                                                                //get the PH reading 
        Serial.print(PH.get_name()); Serial.print(": ");                                      //print the name of the circuit we just got a reading from
        
        if(reading_succeeded(PH) == true){                                                    //if the pH reading has been received and it is valid
          Serial.print(PH.get_last_received_reading(), 2);                                    //print the reading (with 2 decimal places)
          ThingSpeak.setField(1, String(PH.get_last_received_reading(), 2));                  // DD- assign pH readings to first column of thingspeak channel    


          JsonObject feed1 = parameters.createNestedObject();                                 // Declare the first nested object within parameters as feed1
          feed1["name"] = "ph";                                                               // give nested object, feed1, "name" and "value
          feed1["value"] = String(PH.get_last_received_reading(),2);
        }

        Serial.print(" ");                                                                    //print a blank space so the output string on the serial monitor is easy to read
        
        EC.receive_read_cmd();                                                                //get the EC reading 
        Serial.print(EC.get_name()); Serial.print(": ");                                      //print the name of the circuit we just got a reading from
        
        if(reading_succeeded(EC) == true){                                                    //if the EC reading has been received and it is valid
          Serial.print(EC.get_last_received_reading(), 0);                                    //print the reading (with 0 decimal places)
          ThingSpeak.setField(2, String(EC.get_last_received_reading(),0));                   //assign EC readings to the second column of thingspeak channel   

          JsonObject feed2 = parameters.createNestedObject();                                 // Declare second nested object in parameters as feed2
          feed2["name"] = "electrical_conductivity";                                          // Same as in feed1, we give nested JSON object a name and value
          feed2["value"] = String(EC.get_last_received_reading(),0);
        }
        Serial.println();
        Serial.println(EC.get_last_received_reading());
        

        Serial.println();                                                                     //print a new line so the output string on the serial monitor is easy to read
        

        if(RTD.get_error() == Ezo_board::SUCCESS){                                            //if the RTD reading was successful (back in step 1)
          ThingSpeak.setField(3, String(RTD.get_last_received_reading(),1));                            //assign temperature readings to the third column of thingspeak channel

          JsonObject feed3 = parameters.createNestedObject();                                 // Same as above for feed1 and feed2
          feed3["name"] = "nutrient_soil_temp";
          feed3["value"] = String(RTD.get_last_received_reading(),1);

        }

        serializeJsonPretty(doc, Serial);                                                     // Print the JSON to see if it is what we want

          
          String json;                                                                        // Serialize JSON document
          serializeJson(doc, json);
        
          if (http.begin(httpAddress)) {                                                      // Send request
            http.POST(json);
            Serial.print(http.getString());                                                   // Read response
            http.end();                                                                       // Disconnect
            
          } else {
            Serial.printf("[HTTP] GET... failed. error");
            }
        
        Serial.println(); 
        return_code = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);                 //DD- upload the data to thingspeak, read the return code 

        if (return_code == 200) {                                                             //DD- check thingspeak return code if it is 200 then the upload was a success
          Serial.println("Thingspeak upload success");                                                          //DD- print "success"
        }
        else {                                                                                //DD- if the thingspeak return code was not 200  
          Serial.println("upload error, code: " + String(return_code));                       //DD- print "upload error, code:" and whatever number is in the return_code var  
        }
        Serial.println();                                                                     //print a new line so the output string on the serial monitor is easy to read
        next_step_time =  millis() + loop_delay;                                              //update the time for the next reading loop 
        current_step = REQUEST_TEMP;                                                          //switch back to step 1 
        
      }
      break;                                                                                  //break out of this we are done
  }
//------------------------------------------------------------------
}


bool reading_succeeded(Ezo_board &Sensor) {                                                 //this function makes sure that when we get a reading we know if it was valid or if we got an error 

  switch (Sensor.get_error()) {                                                             //switch case based on what the response code was
    case Ezo_board::SUCCESS:                                                                //if the reading was a success
      return true;                                                                          //return true, the reading succeeded 
      
    case Ezo_board::FAIL:                                                                   //if the reading faild
      Serial.print("Failed ");                                                              //print "failed"
      return false;                                                                         //return false, the reading was not successful

    case Ezo_board::NOT_READY:                                                              //if the reading was taken to early, the command has not yet finished calculating
      Serial.print("Pending ");                                                             //print "Pending"
      return false;                                                                         //return false, the reading was not successful

    case Ezo_board::NO_DATA:                                                                //the sensor has no data to send
      Serial.print("No Data ");                                                             //print "no data"
      return false;                                                                         //return false, the reading was not successful
      
    default:                                                                                //if none of the above happened
     return false;                                                                          //return false, the reading was not successful
  }
} 
