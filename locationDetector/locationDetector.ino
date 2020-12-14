#include <Wire.h>
#include "esp_wifi.h"
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

// Mac of user that the device has to detect
String userMac = "B4:F1:DA:E9:A3:42";

bool detected = false;

// WiFi connection data
const char *ssid = "Gon:)";
const char *pass = "Password";

WiFiMulti wifimulti;

// Variables used to scan mac addresses
int listcount = 0;
const int maxMacs = 64;
String maclist[64][2];  // List of macs detected
int NUM_SCANS = 5;  // Number of scans done

const wifi_promiscuous_filter_t filt = { //Idk what this does
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

#define maxCh 13 //max Channel -> US = 11, EU = 13, Japan = 14
int curChannel = 1;

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

// Method to post data to an api
void httpPOSTRequest(const char *serverName, const char *body)
{
  HTTPClient http;
      
  // Domain name with URL path or IP address with path
  http.begin(serverName);
  
  // HTTP request with a content type: application/jsos
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(body); // Posting and obtaining response code

  Serial.println(serverName);
  Serial.println(body);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
    
  // Free resources
  http.end();
}

// Configuration to connect to WiFi
void connectToWiFi()
{
  WiFi.disconnect(true);  // Disconnect from any WiFi
  WiFi.mode(WIFI_STA);  // Set mode to WiFi Station
  wifimulti.addAP(ssid, pass);  // Add configuration data
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
  Serial.println(F("Mac scanner finished"));
  // Cancel promiscuous mode to stop receiving packets
  esp_wifi_set_promiscuous(false);
  // Stop scan
  esp_wifi_scan_stop();
}

// Method to interpretate Mac signal with room of the house
String getLocation(int distance)
{
  Serial.print("The distance is: ");
  Serial.println(distance);
  String location;
  
  if (distance > -60)
    location = "Living room";
  else if (distance > -70)
    location = "bedroom";
  else if (distance > -80)
    location = "bathroom";
  else if (distance > -90)
    location = "kitchen";
  else
    location = "balcony";
    
  return location;
}

// Method to reset mac addresses list
void resetMacs()
{
  for(int i = 0; i < maxMacs; i++) {
    maclist[i][0] = "";
  }
  listcount = 0;
}

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  wifiScan(); // WiFi scanner is launched

  detected = false;
  int i = 0;
  while (i < listcount && !detected)
  {
    if (maclist[i][0] == userMac) // Detect user mac address
      detected = true;
    i++;
  }

  if (detected)
  {
    Serial.print("User mac: ");
    Serial.println(maclist[i][0]);
    Serial.print("User distance: ");
    Serial.println(maclist[i][1]);  // Upload new user location
    connectToWiFi();
    String body = "{\"location\":\"" + getLocation(String(maclist[i][1]).toInt()) + "\"}";
    String visitUrl = "https://muii-g2-api-location.herokuapp.com/location";
  
    httpPOSTRequest(visitUrl.c_str(), body.c_str());
  }
  resetMacs();
  delay(300000);  // Wait 5 minutes until next detection
}
