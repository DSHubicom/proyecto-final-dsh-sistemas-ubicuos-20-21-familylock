/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-two-way-communication-esp32/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <esp_now.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include "esp_wifi.h"
#include <Wire.h>

// Pins were hardware is connected
#define button 4
#define redLed 25
#define greenLed 26
#define blueLed 27

// WiFi connection data
const char *ssid = "Gon:)";
const char *pass = "Password";

WiFiMulti wifimulti;

// Mac address of broadcast for communication between esp-32
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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
  int openDoor; // 0 if open must stay close and >0 if it must open
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
  Serial.print("Bytes received: ");
  Serial.println(len);
  
  if (incomingMessage.visitorRelation == "close") { // If visitor is close, door is open and led if tuned on in green
    digitalWrite(greenLed, HIGH);
    openDoor = 1;
  } else if (incomingMessage.visitorRelation == "medium")
    digitalWrite(blueLed, HIGH);
  else if (incomingMessage.visitorRelation == "unknown")
    digitalWrite(redLed, HIGH);

  
  if (incomingMessage.visitorRelation != "close") {
    int i = 0;
    //  30 seconds to let user press the button. If time is consumed, door is not opened.
    // Pressing of button must be of half a second aproximately.
    while ((openDoor = digitalRead(button)) == 0 && i < 60) {
      delay(500);
      i++;
    }
  }
  digitalWrite(blueLed, LOW);
  digitalWrite(redLed, LOW);  // Turning off led

  // Set values to send
  message.openDoor = openDoor;
  message.visitorRelation = "";

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &message, sizeof(message));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

  if (incomingMessage.visitorRelation == "close") {
    delay(5000);  // Turn off green led, it stays turned on for 5 seconds
    digitalWrite(greenLed, LOW);
  }
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
  // get the status of Transmitted packet
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



 
void setup()
{
  pinMode(button, INPUT); // Assigning pins to type of devices (input or output)
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  
  // Init Serial Monitor
  Serial.begin(115200);
 
  initEspNow();
}
 
void loop(){}
