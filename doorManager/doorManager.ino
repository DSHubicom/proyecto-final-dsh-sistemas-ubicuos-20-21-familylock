#include <Wire.h>
#include "esp_wifi.h"
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "ESP32_Servo.h"
#include <esp_now.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Pins were hardware is connected
#define servoPin 19
#define proximitySensor 4

Servo servo;

// Mac address of broadcast for communication between esp-32
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// WiFi connection data
const char *ssid = "Gon:)";
const char *pass = "Password";

WiFiMulti wifimulti;

// Variables to get actual time and date
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time obtained
String formattedDate;
String dayStamp;
String timeStamp;

// Published values for SG90 servos; adjust if needed
int minUs = 500;
int maxUs = 2400;

// These are all GPIO pins on the ESP32
// Recommended pins include 2,4,12-19,21-23,25-27,32-33

int pos = 0; // Position in degrees of servo

String api_response; // To store api response

// Variables used to scan mac addresses
int listcount = 0;
const int maxMacs = 64;
String maclist[64][2];  // List of macs detected
int SIGNAL_INTENSITY = -60; // Signal intensity to consider macs detected
int NUM_SCANS = 5;  // Number of scans done

const wifi_promiscuous_filter_t filt = { //Idk what this does
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

#define maxCh 13 //max Channel -> US = 11, EU = 13, Japan = 14
int curChannel = 1;

int openDoor; // 0 if door must close, >0 if it must open

// Variable to store if sending data was successful
String success;

//Structure to send data
//Must match the receiver structure
typedef struct struct_message {
    int openDoor;
    String visitorRelation;
} struct_message;

// struct_message called to hold data
struct_message message;

// struct_message to hold incoming data
struct_message incomingMessage;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
  Serial.print("Bytes received: ");
  Serial.println(len);
  openDoor = incomingMessage.openDoor;  // Only important data is new door state
}

// Method to process mac address and transform it to the correct format
String procesarMac(String mac)
{
  int _index = 0;
  int limit = 2; // Exclude position 2
  char dot = ':';
  int size = mac.length() / 2;
  String newMac = "";
  for (int i = 0; i < size; i++)
  {
    // Every two numbers, insert : in the chain
    String twoChars = mac.substring(_index, limit); // Obtain two character's value
    // Insert : on secondChar position in the chain
    newMac += twoChars;
    if (i != size - 1)
    {
      newMac += dot;
    }

    _index = limit;
    limit += 2;
  }
  return newMac;
}

// Method to implement the mac scanner
void sniffer(void *buf, wifi_promiscuous_pkt_type_t type)
{

  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t *)buf; // Dont know what these 3 lines do
  String packet;
  String mac;
  // If signal intensity of mac address is over -60 db, it means that device is close enough to be considered by the system
  if (p->rx_ctrl.rssi > SIGNAL_INTENSITY)
  {
    for (int i = 8; i <= 8 + 6 + 1; i++)
    {
      // With this operation, the first pair of bytes of the packet is read
      // Those bytes are the mac address
      // This is where you can read the whole packet replacing the "8+6+1" with "p->rx_ctrl.sig_len"
      packet += String(p->payload[i], HEX);
    }
    int last_pos = 15;
    for (int i = 4; i <= last_pos; i++)
    {
      // Mac address starts at position 4 and finish on position 15 of data packet
      // Its length is 12 characters
      // Only mac address is taken
      mac += packet[i];
    }

    mac.toUpperCase();
    // Mac is formatted
    String newMac = procesarMac(mac);

    int added = 0;
    // Mac added to mac addresses list
    for (int i = 0; (i < maxMacs) & (added == 0); i++)
    {
      if (newMac == maclist[i][0])
        added = 1;
    }
    if (added == 0)
    {
      maclist[listcount][0] = newMac;
      maclist[listcount][1] = p->rx_ctrl.rssi;
      listcount++;

      Serial.println(newMac + ": " + p->rx_ctrl.rssi + "dB");
      if (listcount >= maxMacs)
      {
        Serial.println("Too many addresses");
        listcount = 0;
      }
    }
  }
}

// Method to post data to an api
void httpPOSTRequest(const char *serverName, const char *body)
{
  HTTPClient http;
      
  // Domain name with URL path or IP address with path
  http.begin(serverName);
  
  // HTTP request with a content type: application/json
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(body); // Posting and obtaining response code

  Serial.println(serverName);
  Serial.println(body);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
    
  // Free resources
  http.end();
}

// Method to get data from an api
String httpGETRequest(const char *serverName)
{
  HTTPClient http;

  // IP address with path or Domain name with URL path
  http.begin(serverName);

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

// Method to init ESP NOW communication
void initEspNow() {
  WiFi.mode(WIFI_AP); // Set WIFI_AP mode for WiFi
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_peer_info_t peerInfo; // Peer is the other side of communication
  memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
  memcpy(peerInfo.peer_addr, broadcastAddress, sizeof(uint8_t[6]));
  peerInfo.ifidx = ESP_IF_WIFI_AP;
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

// Configuration to connect to WiFi
void connectToWiFi()
{
  WiFi.disconnect(true);  // Disconnect from any WiFi
  WiFi.mode(WIFI_STA);  // Set mode to WiFi Station
  wifimulti.addAP(ssid, pass);  // Add configuration data  // Add configuration data
  Serial.print("Connecting to ");
  Serial.println(ssid);

  while (wifimulti.run() != WL_CONNECTED) // Connecting
  {
    delay(500);
    Serial.print('.');
  }
  Serial.flush();
  Serial.println();
  Serial.print("Connected. My IP address is: ");
  Serial.println(WiFi.localIP());
  Serial.flush();

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);
}

// Method to get actual time and date from the Internet
void getActualTime()
{
  timeClient.forceUpdate();
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // Extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");  //Split String with 'T'
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
}

// Configuration of WiFi scanner
void wifiScan()
{
  /* setup wifi */
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);

  Serial.println(F("Starting Wifi Scan..."));
  int index = 0;
  while (index < NUM_SCANS) // Doing 5 scans, each one using all 13 channels
  {
    Serial.print(F("SCAN Nº "));
    Serial.println(index + 1);
    while (curChannel <= maxCh)
    {
      esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
      Serial.println("Using channel:" + String(curChannel));
      Serial.println();
      delay(500);
      curChannel++;
    }
    curChannel = 1;
    index++;
    Serial.println(F("----------------------------------------------"));
  }
  Serial.println(F("Program finished"));
  // Cancel promiscuous mode to stop receiving packets
  esp_wifi_set_promiscuous(false);
  // Stop scan
  esp_wifi_scan_stop();
}

// Method to reset mac addresses list
void resetMacs()
{
  for(int i = 0; i < maxMacs; i++) {
    maclist[i][0] = "";
  }
  listcount = 0;
}

// Method to open door moving the Servo
void openingDoor() {
  delay(2000);
        
  for (pos = 0; pos <= 90; pos += 1)
  { // sweep from 0 degrees to 90 degrees
    // in steps of 1 degree
    servo.write(pos);
    delay(100); // waits 100ms for the servo to reach the position
  }

  // Wait time before closing door
  // In a real scenario, this delay should of at least 30 or 60 seconds
  delay(5000);
  
  for (pos = 90; pos >= 0; pos -= 1)
  { // sweep from 180 degrees to 0 degrees
    servo.write(pos);
    delay(100);
  }
}

void setup()
{
  servo.attach(servoPin, minUs, maxUs); // Assigning pins to type of devices (input or output)
  pinMode(proximitySensor, INPUT);

  Serial.begin(115200);
}

void loop()
{
  if (digitalRead(proximitySensor)) // Detect pressence with sensor
  {
    openDoor = -1;
    wifiScan(); // Scan macs
    connectToWiFi();  // Connect to WiFi
    JSONVar myObject; 
    String visitorMac = "";

    // Extract the closest mac address
    for (int i = 0; i < listcount; i++)
    {
      if (i > 0 && String(maclist[i][1]).toInt() > String(maclist[i-1][1]).toInt())
        visitorMac = maclist[i][0];
      else if (i == 0)
        visitorMac = maclist[0][0];
    }
    // Check that one mac has been received
    if (visitorMac != "") {
      getActualTime();  // Get time to insert visit to database
      String body = "{\"person_MAC\":[\"" + visitorMac + "\"],\"date\":\"" + dayStamp + "\",\"time\":\"" + timeStamp + "\"}";
      String visitUrl = "https://muii-g2-api-historic.herokuapp.com/historic";
  
      httpPOSTRequest(visitUrl.c_str(), body.c_str());  // Insert visit to database
      
      String legitimateUrl = "https://muii-g2-api-legitimate.herokuapp.com/legitimate_person/" + visitorMac;
      Serial.println(legitimateUrl);
      // Get person information
      api_response = httpGETRequest(legitimateUrl.c_str());
      Serial.println(api_response);
  
      myObject = JSON.parse(api_response);  // Parsing information to JSON
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined")
      {
        Serial.println("Parsing input failed!");
        return;
      }
  
      Serial.print("JSON object = ");
      Serial.println(myObject);
      
      String visitorRelation; // Getting visitor relation to user
      if(myObject["legitimate"].length() >= 1) {
        visitorRelation = myObject.stringify(myObject["legitimate"][0]["relation"]);
        visitorRelation.replace("\"","");
      } else  // If mac doesn't exist on database, visitor is unknown
        visitorRelation = "unknown";

      // Request user to open the door
      // If person is close, it just advise the user
      if (visitorRelation == "unknown" || visitorRelation == "medium" || visitorRelation == "close"){
        initEspNow();
        // Set values to send
        message.openDoor = 0;
        message.visitorRelation = visitorRelation;
      
        // Send message via ESP-NOW
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &message, sizeof(message));
         
        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
          Serial.println("Error sending the data");
        }

        int i = 0;
        // Wait for a response from the user
        while(openDoor < 0 && i < 40) { // If after 40 seconds, there is no respones, the door is not opened
          delay(1000);
          i++;
        }
        
        if (openDoor > 0) { // Open the door if user has allowed it
          Serial.println("Opening door");
          openingDoor();
        } else
        Serial.println("Not opening door");
      }
      resetMacs();
    }
  }
}
