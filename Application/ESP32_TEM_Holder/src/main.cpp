
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <EEPROM.h>


#include "Eeprom_Lib.h"


/*************************************************************************************************
 * Network Credentials
 * 
 * ***********************************************************************************************/

const char* ssid = "MagruuFi";
const char* password = "kayabanana";

// const char* ssid = "ZMB-Y42F54-WIFI"; // ZMB Credentials
// const char* password = "jumbo8+Cloud";

/*************************************************************************************************
 * User Global Variables
 * 
 * ***********************************************************************************************/

#define EEPROM_SIZE 7              // define the number of bytes you want to access



int LED_PIN = 2;

uint16_t current_Holder_Position;   // saves the current holder position

int requested_Holder_Position = 0;  // saves the requested holder postion

int eeprom_pos = 0;

int Holder_State = 0;               // saves the state of the holder (Calibration, Live-View, Normal)

Eeprom_Holder_Pos Holder_Pos;



/*************************************************************************************************
 * Initialize JSON element
 * 
 * ***********************************************************************************************/

DynamicJsonDocument Tx_Doc(1024);   //Library Variable for Json
DynamicJsonDocument Rx_Doc(1024);   //Library Variable for Json
String Tx_Json;                     //Json-ized String
String Rx_Json;                     //Json-ized String


/*************************************************************************************************
 * Initialize Webserver & Websocket
 * 
 *    Asyncwebserver: multiple clients and async functionality
 *    Websocket:      communication library between client and server  
 * 
 * ***********************************************************************************************/

AsyncWebServer server(80); // Create AsyncWebServer object on port 80

AsyncWebSocket ws("/ws"); // Create WebSocketServer (usually on port 81)
 

/*************************************************************************************************
 * Websocket Functions
 * 
 *    handleWebSocketMessage():   handles incomming messages from the Websocket/Client  
 *    onEvent():                  Gets called when an event occurs on the WebSocket
 * 
 * ***********************************************************************************************/

// Handles the received message
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {

    data[len] = 0;
    Serial.print("RX: ");
    Serial.println((char*)data);

    Rx_Json = (char*) data;
    deserializeJson(Rx_Doc, Rx_Json);

    const char* expression = (const char*)Rx_Doc["message_type"];

    delay(1000);

    if(!strcmp(expression, "POSITION")){

      current_Holder_Position = Rx_Doc["data"];

      Holder_Pos.set_pos(current_Holder_Position, Holder_Pos.get_eeprom_pos());

      Serial.println("Got Position! Sending Ack");
      Tx_Doc["message_type"] = "ACK";
      Tx_Doc["data"] = "SET";
      serializeJson(Tx_Doc, Tx_Json);
      Serial.print("TX :");
      Serial.println(Tx_Json);
      ws.textAll(Tx_Json);

      Tx_Json.clear();
      
      Serial.println("Setting Position on all Clients");
      Tx_Doc["message_type"] = "POSITION";
      Tx_Doc["data"] = Rx_Doc["data"];
      serializeJson(Tx_Doc, Tx_Json);
      Serial.print("TX :");
      Serial.println(Tx_Json);
      ws.textAll(Tx_Json);

    }else if(!strcmp(expression, "ACK")){

    }else if(!strcmp(expression, "STATUS")){
      
    }

    Tx_Json.clear();
    
  }
}

// Gets called when an event occurs on the WebSocket
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
 void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:    // New client connected 
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT: // Client got disconnected
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:       // Data was received
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:       // Pong request
    case WS_EVT_ERROR:      // Error 
      break;
  }
}

/*************************************************************************************************
 * User Functions
 *    
 *    process_data():     gets called when a new stepper position is requested
 * 
 * ***********************************************************************************************/

// move stepper motor to the according position
void process_data(){
  if(digitalRead(LED_PIN) == LOW){
    digitalWrite(LED_PIN, HIGH);
  } else{
    digitalWrite(LED_PIN, LOW);
  }
    
}



/*************************************************************************************************
 * Arduino Setup Function
 * 
 *    Used as initialization function for the used libraries and functionalities
 * 
 * ***********************************************************************************************/

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Setup LED
  pinMode(LED_PIN, OUTPUT);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Display used space of SPIFFS
  unsigned int totalBytes = SPIFFS.totalBytes();
  unsigned int usedBytes = SPIFFS.usedBytes();

  Serial.println();
  Serial.println("===== File system info =====");

  Serial.print("Total space:      ");
  Serial.print(totalBytes);
  Serial.println("bytes");

  Serial.print("Total space used: ");
  Serial.print(usedBytes);
  Serial.println("bytes");

  Serial.print("Remaining free space: ");
  Serial.print(totalBytes - usedBytes);
  Serial.println("bytes");

  Serial.println();

  // Initialize EEPROM with predefined size
  Serial.println("===== EEPROM Init =====");
  Serial.print("Eeprom Size: ");
  Serial.print(EEPROM_SIZE);
  Serial.println(" Bytes");
  if(!EEPROM.begin(EEPROM_SIZE)){
    Serial.println("An Error has occurred while mounting EEPROM");
    return;
  }

  current_Holder_Position = Holder_Pos.get_pos();
  Serial.print("Recovered Holder Position: ");
  Serial.println(Holder_Pos.get_pos());
  Serial.print("Holder EEPROM Position: ");
  Serial.println(Holder_Pos.get_eeprom_pos());
  Serial.println();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println("Wifi Connected! Your IP:");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  // Initialize the WebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to load script.js file
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  // Start server
  server.begin();

}
 
/*************************************************************************************************
 * Arduino Loop Function
 * 
 *    Infinite loop for controlling the holder
 * 
 * ***********************************************************************************************/

void loop(){
  ws.cleanupClients();
}
