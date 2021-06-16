
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <TMCStepper.h>

/*************************************************************************************************
 * Network Credentials
 * 
 * ***********************************************************************************************/

// const char* ssid = "NETGEAR";
// const char* password = "mhsi12jaia";

const char* ssid = "MagruuFi";
const char* password = "kayabanana";

// const char* ssid = "ZMB-Y42F54-WIFI"; // ZMB Credentials
// const char* password = "jumbo8+Cloud";

/*************************************************************************************************
 * User Global Variables
 * 
 * ***********************************************************************************************/

#define EEPROM_SIZE      7  // define the number of bytes you want to access

#define EN_PIN           25 // Enable
#define DIR_PIN          4  // Direction
#define STEP_PIN         14 // Step
#define HW_RX            15 // TMC2208/TMC2224 HardwareSerial receive pin
#define HW_TX            27 // TMC2208/TMC2224 HardwareSerial transmit pin

#define FLAG_SET     1
#define FLAG_UNSET   0

bool positionFlag = FLAG_UNSET;
bool calibrationFlag = FLAG_UNSET;

// Timer for the periodic interrupt
hw_timer_t * timer1 = NULL;

uint32_t currentPosition = 0;  // saves the current holder position
uint32_t desiredPosition = 0; 
uint32_t rawDesiredPosition = 0;

uint32_t minPosition = 0;
uint32_t maxPosition;

enum Direction {left = 1, right = 0};

enum State {normal, calibration, setting, homing};

State state = calibration;

/*************************************************************************************************
 * Initialize JSON element
 * 
 * ***********************************************************************************************/

DynamicJsonDocument Tx_Doc(1024);   //Library Variable for Json
DynamicJsonDocument Rx_Doc(1024);   //Library Variable for Json
String Tx_Json;                     //Json-ized String
String Rx_Json;                     //Json-ized String

/*************************************************************************************************
 * Motor Parameter
 * 
 * ***********************************************************************************************/

//For Small Motor there is a signifcant difference between turning directions

// OK for 5v
// #define STALL_VALUE_LEFT      35 
// #define STALL_VALUE_RIGHT     40

// OK for 12V
#define STALL_VALUE_LEFT      37
#define STALL_VALUE_RIGHT     37

#define STALL_VALUE_STARTUP   0

#define SERIAL_PORT Serial2 // TMC2208/TMC2224 HardwareSerial port
#define DRIVER_ADDRESS 0b00 // TMC2209 Driver address according to MS1 and MS2

#define R_SENSE 0.11f // Value for SilentStepStick

// Motor Paramters
#define FULL_STEPS_ROTATION 200 // Number of Full Steps for one rotation
#define MICROSTEPS_RESOLUTION 16 // Makes 1 Full Step with the defined number of microsteps

/*************************************************************************************************
 * Holder Parameters
 * 
 * ***********************************************************************************************/

#define MAX_HOLDER_VALUE        400.0
#define HOME_HOLDER_VALUE       50.0

/*************************************************************************************************
 * Initialize Stepper Motor Driver
 * 
 * ***********************************************************************************************/

TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDRESS);

using namespace TMC2209_n;

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
 * User Functions
 *    
 *    controlPosition():  this function is called in the periodic timer ISR, it gets called to see 
 *                        if position is set or not
 *    calibratePosition(): calibrates the holder automaticly and aborts it if something goes wrong.
 *                         Is done on startup, but can be called by user
 *    setPosition():      converts the desiredPosition from values of 0 to MAX_HOLDER_VALUE (on the GUI) to values of the internal map
 *    
 * ***********************************************************************************************/

void controlPosition(){
  digitalWrite(STEP_PIN, !digitalRead(STEP_PIN));

  if(currentPosition == desiredPosition && state != calibration){
    timerAlarmDisable(timer1);
    digitalWrite(EN_PIN, HIGH);  // Disable the Stepper Driver
    positionFlag = FLAG_SET;
    Serial.print("My current position = ");
    Serial.println(currentPosition);
  }

  switch(state){
    case setting:
      if (currentPosition < desiredPosition){
        ++currentPosition;
      } else if(currentPosition > desiredPosition){
        --currentPosition;
      }
      break;
    case homing:
      if (currentPosition < desiredPosition){
        ++currentPosition;
      } else if(currentPosition > desiredPosition){
        --currentPosition;
      }
      break;
    case calibration:
      ++currentPosition;
      break;
    case normal:
      break;
  }
}

void setPosition(uint32_t position){

  desiredPosition = position/MAX_HOLDER_VALUE * maxPosition;
  if (currentPosition < desiredPosition){
    driver.SGTHRS(STALL_VALUE_RIGHT);
    driver.shaft(right);
  } else if (currentPosition > desiredPosition){
    driver.SGTHRS(STALL_VALUE_LEFT);
    driver.shaft(left);
  } 
  digitalWrite(EN_PIN, LOW);  // Enable the Stepper Driver
  timerAlarmEnable(timer1);
}

void calibratePosition(){

  timerAlarmDisable(timer1);
  digitalWrite(EN_PIN, HIGH);  // Disable the Stepper Driver
  Serial.println();
  Serial.println("Calibration started...");
  driver.SGTHRS(STALL_VALUE_STARTUP);
  driver.shaft(left);
  delay(1000);
  digitalWrite(EN_PIN, LOW);  // Enable the Stepper Driver
  timerAlarmEnable(timer1);
  delay(2);
  driver.SGTHRS(STALL_VALUE_LEFT);
  
   uint32_t last_time=millis();
  while(true){

    uint32_t ms = millis();
    // if no Diag Signal after 5s something went wrong
    if((ms-last_time) > 15000) { //run every 10s
      last_time = ms;

      Tx_Doc["message_type"] = "STATUS";
      Tx_Doc["data"] = "calibration error";
      serializeJson(Tx_Doc, Tx_Json);
      Serial.print("TX :");
      Serial.println(Tx_Json);
      ws.textAll(Tx_Json);
      Tx_Json.clear();
      Serial.println("Something went wrong with the calibration, holder reboots!");
      delay(5000);
      ESP.restart();
    }

    if(driver.diag()){
      timerAlarmDisable(timer1);
      digitalWrite(EN_PIN, HIGH);  // Disable the Stepper Driver
      break;
    }
  }

  currentPosition = minPosition; // sets the counted currentPosition to 0

  driver.SGTHRS(STALL_VALUE_STARTUP);
  driver.shaft(right);
  delay(1000);
  digitalWrite(EN_PIN, LOW);  // Enable the Stepper Driver
  timerAlarmEnable(timer1);
  delay(2);
  driver.SGTHRS(STALL_VALUE_RIGHT);
  
  last_time = millis();
  while(true){

    uint32_t ms = millis();
    // if no Diag Signal after 5s something went wrong
    if((ms-last_time) > 15000) { //run every 15s
      last_time = ms;

      Tx_Doc["message_type"] = "STATUS";
      Tx_Doc["data"] = "calibration error";
      serializeJson(Tx_Doc, Tx_Json);
      Serial.print("TX :");
      Serial.println(Tx_Json);
      ws.textAll(Tx_Json);
      Tx_Json.clear();
      Serial.println("Something went wrong with the calibration, holder reboots!");
      delay(5000);
      ESP.restart();
    }

    if(driver.diag()){
      timerAlarmDisable(timer1);
      digitalWrite(EN_PIN, HIGH);  // Disable the Stepper Driver
      break;
    }
  }

  maxPosition = currentPosition+1; // upon wall detection, set the position to the maxPosition

  state = normal; // change state to normal operation state

  calibrationFlag = FLAG_SET;; // notifies that calibration has ended

  Serial.println("Calibration ended successfully!");
  Serial.print("Min Position = ");
  Serial.println(minPosition);

  Serial.print("Max Position = ");
  Serial.println(maxPosition);

}

void homePosition(){
  Serial.println("Homing Initiated!");
  uint32_t homePosition = minPosition + 20.0;
  setPosition(homePosition);
}

// Callback function when timer elapsed
void IRAM_ATTR onTimer() {

  controlPosition();
} 


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

    if(!strcmp(expression, "POSITION")){ 

      rawDesiredPosition = (uint32_t)Rx_Doc["data"];
      state = setting;
      
      // Serial.println("Setting Position on all Clients");
      // Tx_Doc["message_type"] = "POSITION";
      // Tx_Doc["data"] = Rx_Doc["data"];
      // serializeJson(Tx_Doc, Tx_Json);
      // Serial.print("TX :");
      // Serial.println(Tx_Json);
      // ws.textAll(Tx_Json);

      // Tx_Json.clear();

    }else if(!strcmp(expression, "ACK")){

    }else if(!strcmp(expression, "STATUS")){

      if(!strcmp(Rx_Doc["data"], "calibration")){
        state = calibration;
        calibrationFlag = FLAG_UNSET;
      }else if(!strcmp(Rx_Doc["data"], "homing")){
        state = homing;
        positionFlag =  FLAG_UNSET;
      }
      
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
      Tx_Doc["message_type"] = "POSITION";
      Tx_Doc["data"] = currentPosition;
      serializeJson(Tx_Doc, Tx_Json);
      ws.text(client->id(), Tx_Json);
      Tx_Json.clear();
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
 * Arduino Setup Function
 * 
 *    Used as initialization function for the used libraries and functionalities
 * 
 * ***********************************************************************************************/

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Serial port for Stepper Driver Communication
  SERIAL_PORT.begin(115200, SERIAL_8N1, HW_RX, HW_TX);

  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

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

  // Enable the Stepper Driver
  digitalWrite(EN_PIN, LOW);

  // Set Stepper Driver Values
  driver.begin();
  driver.pdn_disable(true); 
  driver.I_scale_analog(false);
  driver.toff(4);
  //driver.VACTUAL(SPEED);
  driver.blank_time(24);
  driver.rms_current(140, 0.1); // mA
  driver.en_spreadCycle(false);
  driver.mstep_reg_select(true);
  driver.microsteps(MICROSTEPS_RESOLUTION);
  driver.TCOOLTHRS(0xFFFFF); // 20bit max
  driver.semin(3);
  driver.semax(0);
  driver.sedn(0b01);
  driver.SGTHRS(STALL_VALUE_RIGHT);
  driver.shaft(left);

  // Set stepper interrupt
  cli();//stop interrupts
  timer1 = timerBegin(3, 80, true); // Initialize timer 4. Prescaler 80,  ESP(0,1,2,3)
  timerAttachInterrupt(timer1, &onTimer, true); //link interrupt with function onTimer
  timerAlarmWrite(timer1, 80, true); // Time to pass for timer interrupt to occur
  //timerAlarmEnable(timer1);    //Enable timer        
  sei();//allow interrupts

  calibratePosition();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.print("Wifi Connected with IP: ");
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

  // if(driver.diag()){
  //   state = calibration;
  //   calibratePosition();
  // }

  if (state == calibration){
        calibratePosition();

        static uint32_t last_time=millis();
        while(calibrationFlag != FLAG_SET){
          uint32_t ms = millis();
          // if position is not set after 5s something went wrong
          if((ms-last_time) > 15000) { //run every 10s
            last_time = ms;
            
            Tx_Doc["message_type"] = "STATUS";
            Tx_Doc["data"] = "calibration error";
            serializeJson(Tx_Doc, Tx_Json);
            Serial.print("TX :");
            Serial.println(Tx_Json);
            ws.textAll(Tx_Json);
            Tx_Json.clear();
            Serial.println("Something went wrong while calibrating, holder reboots!");
            delay(5000);
            ESP.restart();
          }
        } // wait for flag to be raised
        Tx_Json.clear();
        Tx_Doc["message_type"] = "STATUS";
        Tx_Doc["data"] = "calibrated";
        Serial.print("TX :");
        serializeJson(Tx_Doc, Tx_Json);
        Serial.println(Tx_Json);
        ws.textAll(Tx_Json);

        Tx_Json.clear();
  }else if(state == setting){
      setPosition(rawDesiredPosition);

      uint32_t last_time=millis();
      while(positionFlag != FLAG_SET){ // Wait for holder to reach desiredPosition
        
        uint32_t ms = millis();
        // if position is not set after 15s something went wrong
        if((ms-last_time) > 15000) { //run every 15s
          last_time = ms;

          Tx_Doc["message_type"] = "STATUS";
          Tx_Doc["data"] = "positioning error";
          serializeJson(Tx_Doc, Tx_Json);
          Serial.print("TX :");
          Serial.println(Tx_Json);
          ws.textAll(Tx_Json);
          Tx_Json.clear();
          Serial.println("Something went wrong while setting the position, holder reboots!");
          delay(5000);
          ESP.restart();
        }
      }  

      positionFlag = FLAG_UNSET;   // Unset the positonFlag variable
      state = normal;

      Serial.println("Got Position! Sending Ack");
      Tx_Doc["message_type"] = "ACK";
      Tx_Doc["data"] = "SET";
      serializeJson(Tx_Doc, Tx_Json);
      Serial.print("TX :");
      Serial.println(Tx_Json);
      ws.textAll(Tx_Json);
      Tx_Json.clear();

  }else if (state == homing){
      setPosition(HOME_HOLDER_VALUE);

      uint32_t last_time_2=millis();
      while(positionFlag != FLAG_SET){ // Wait for holder to reach desiredPosition
        
        uint32_t ms_2 = millis();
        // if position is not set after 15s something went wrong
        if((ms_2-last_time_2) > 15000) { //run every 15s
          last_time_2 = ms_2;

          Tx_Doc["message_type"] = "STATUS";
          Tx_Doc["data"] = "positioning error";
          serializeJson(Tx_Doc, Tx_Json);
          Serial.print("TX :");
          Serial.println(Tx_Json);
          ws.textAll(Tx_Json);
          Tx_Json.clear();
          Serial.println("Something went wrong while setting the position, holder reboots!");
          delay(5000);
          ESP.restart();
        }
      }

      positionFlag = FLAG_UNSET;   // Unset the positonFlag variable
      state = normal;

      Serial.println("Homing done! Sending Ack");
      Tx_Doc["message_type"] = "STATUS";
      Tx_Doc["data"] = "homing done";
      serializeJson(Tx_Doc, Tx_Json);
      Serial.print("TX :");
      Serial.println(Tx_Json);
      ws.textAll(Tx_Json);
      Tx_Json.clear();  
  }

}
