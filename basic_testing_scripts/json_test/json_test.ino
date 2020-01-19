#include <ArduinoJson.h>

void setup() {
  // Initialize Serial port
  Serial.begin(9600);
  while (!Serial) continue;

  // Allocate the JSON document
  //
  // Inside the brackets, 200 is the RAM allocated to this document.
  // Don't forget to change this value to match your requirement.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<200> doc;

  // StaticJsonObject allocates memory on the stack, it can be
  // replaced by DynamicJsonDocument which allocates in the heap.
  //
  // DynamicJsonDocument  doc(200);

  // Add values in the document
  //
  doc["type"] = "MFC";
  doc["unitid"] = "000235";

  // Add an array.
  //
  JsonArray parameters = doc.createNestedArray("parameters");
  
  JsonObject feed1 = parameters.createNestedObject();
  feed1["name"] = "nutrient_soil_temp";
  feed1["value"] = "25.36";

  JsonObject feed2 = parameters.createNestedObject();
  feed2["name"] = "electrical_conductivity";
  feed2["value"] = "-0.1";

  JsonObject feed3 = parameters.createNestedObject();
  feed3["name"] = "ph";
  feed3["value"] = "0.7";
  
  doc["timestamp"] = "1574129424";

  // Generate the minified JSON and send it to the Serial port.
  //
  serializeJson(doc, Serial);
  // The above line prints a line
  // Start a new line
  Serial.println();

  // Generate the prettified JSON and send it to the Serial port.
  //
  serializeJsonPretty(doc, Serial);
  // The above line prints pretty
}

void loop() {
  // not used in this example
}
