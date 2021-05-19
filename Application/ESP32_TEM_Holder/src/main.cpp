
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "MagruuFi";
const char* password = "kayabanana";

// Led Pin
int LED_PIN = 2;

// Initialize JSON element
DynamicJsonDocument Tx_Doc(1024);
DynamicJsonDocument Rx_Doc(1024);
String Tx_Json;
String Rx_Json;

const char* Position = "POSITION";
const char* Ack = "ACK";


// Input Parameters to connect Frontend and Backend
String sliderValue = "0";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create WebSocketServer (usually on port 81)
AsyncWebSocket ws("/ws");
 
// move stepper motor to the according position
void process_data(){
  if(digitalRead(LED_PIN) == LOW){
    digitalWrite(LED_PIN, HIGH);
  } else{
    digitalWrite(LED_PIN, LOW);
  }
    
}

// Websocket functions

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

    if(!strcmp(expression, Position)){

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

    }else if(!strcmp(expression, Ack)){

    }

    Tx_Json.clear();
    
  }
}

// Gets called when an event occurs on the WebSocket
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
 void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

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
 
void loop(){
  ws.cleanupClients();
}
