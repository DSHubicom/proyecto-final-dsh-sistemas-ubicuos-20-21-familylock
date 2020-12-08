#include "BLEDevice.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Wire.h>
#include "esp_wifi.h"

static BLEAddress *pServerAddress;
BLEScan* pBLEScan;
BLEClient*  pClient;
bool deviceFound = false;
int SIGNAL_INTENSITY = -60;
int SCAN_TIME = 5;
int counter = 0;
int curChannel = 1;
String knownMacAddress[64];
int last_position;
const char* ssid     = "argynos";
const char* password = "qogdy3kf";

const char* server = "muii-g2-api-legitimate.herokuapp.com";

WiFiMulti WiFiMulti;

String defaultTTL = "60"; // Maximum time (Apx seconds) elapsed before device is consirded offline

const wifi_promiscuous_filter_t filt = { //Idk what this does
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA
};

typedef struct { // or this
    uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct { // still dont know much about this
    int16_t fctl;
    int16_t duration;
    MacAddr da;
    MacAddr sa;
    MacAddr bssid;
    int16_t seqctl;
    unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;


#define maxCh 13 //max Channel -> US = 11, EU = 13, Japan = 14


bool findMac(String mac) {
  bool found = false;
  for (int i = 0; i < last_position & !found; i++) {
    if (mac == known_mac_address[i]) {
      found = true;
    }
  }
  return found;
}


class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice Device) {
        pServerAddress = new BLEAddress(Device.getAddress());
        bool known = false;
        bool Master = false;

        Serial.printf("%s: ", pServerAddress->toString().c_str());
        Serial.println(Device.getRSSI());

        // sustituir por el find y hacer una petición
        String mac = pServerAddress->toString().c_str();
        // Si la señal del dispositivo es aceptable
        
        for (int i = 0; i < (sizeof(knownMacAddress)/sizeof(knownMacAddress[0])); i++) {
            if (strcmp(pServerAddress->toString().c_str(), knownMacAddress[i].c_str()) == 0)
                known = true;
        }
        if (known) {
            Serial.print("Device found: ");
            Serial.println(Device.toString().c_str());
            // Si la intensidad de la señal es muy buena, quiere decir que el dispositivo está muy cerca de la puerta
            // POr lo que podremos aceptar que pretende llamar a la vivienda.
            if (Device.getRSSI() > SIGNAL_INTENSITY) {
                deviceFound = true;
            }
            else {
                deviceFound = false;
            }
            Device.getScan()->stop();
            delay(100);
        }
    }
};


void setMacAddrs(String mac) {
  // si la mac no está en la lista, lo añadimos
  known_mac_address[last_position] = mac;
  last_position = sizeof(knownMacAddress)/sizeof(known_mac_address[0]);
  
}

void sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t *) buf; // Dont know what these 3 lines do
    int len = p->rx_ctrl.sig_len;
    WifiMgmtHdr *wh = (WifiMgmtHdr *) p->payload;
    len -= sizeof(WifiMgmtHdr);
    if (len < 0) {
        Serial.println("Receuved 0");
        return;
    }
    String packet;
    String mac;
    int fctl = ntohs(wh->fctl);
    // This reads the first couple of bytes of the packet. This is where you can read the whole packet replaceing the "8+6+1" with "p->rx_ctrl.sig_len"
    for (int i = 8; i <= 8 + 6 +1; i++) { 
        packet += String(p->payload[i], HEX);
    }
    for (int i = 4; i <= 15; i++) {
        mac += packet[i];
    }
    mac.toUpperCase();
    // seteamos la mac a nuestra lista
    if (!findMac(mac)) {
        setMacAddrs(mac);
        Serial.print("Se ha añadido la mac ");
        Serial.print(mac);
        Serial.println(" a la lista de direcciones");
    } else {
        Serial.println("La mac ya está en la lista");
    }
}

/**
 * Realiza una conexión a un AP
 */
void connectToWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  while ((WiFiMulti.run() != WL_CONNECTED)) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void getRequest(WiFiClient client, String url, String mac) {
  // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 20000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }
      // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }

}

/**
 * Realiza una petición GET al servidor "server" usando la mac
 * pasada por parámetro
 */
void httpRequest(String mac) {
  Serial.print("connecting to ");
    Serial.println(server);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(server, httpPort)) {
        Serial.println("connection failed");
        return;
    }

    // We now create a URI for the request
    String url = "/legitimate_person/";
    url += mac;

    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    getRequest(client, url, mac);

  
    Serial.println();
    Serial.println("closing connection");
}

void setup() {
  Serial.begin(115200);
  connectToWifi();
  Bluetooth();
  Wifi();
  Serial.println("Done");
}

void Bluetooth() {
    BLEDevice::init("");
    pClient = BLEDevice::createClient();
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
}

void bluetoothScan() {
  Serial.println("Scanning bluetooth devices...");
  if (counter <= 5) {
        Serial.println();
        Serial.println("BLE Scan restarted.....");
        BLEScanResults scanResults = pBLEScan->start(SCAN_TIME);
        Serial.printf("Se han encontrado %d dispositivos...\n", scanResults.getCount());
        if (deviceFound) {
            Serial.println("Dispositivo encontrado");
            Serial.println("Notificando al usuario ");
            delay(10000);
        }
        else{
            Serial.println("Dispositivo no encontrado");
            delay(1000);
        }
    }
}

void Wifi() {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
}

void wifiScan() {
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  if (curChannel > maxCh) {
      curChannel = 1;
  }
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
  Serial.println("Scanning wifi devices...");
  curChannel +=1;
}

void loop() {
  bool result = false;
  while(counter <= SCAN_TIME) {
    bluetoothScan();
    if (result) {
      Serial.println("Saliendo del bucle...");
      break;
    }
    counter ++;
  }
  counter = 0;
  if (result == false) {
    while(counter < 5) {
      wifiScan();
      if (result) {
        Serial.println("Saliendo del bucle...");
        break;
      }
      counter ++;
    }
  }
  counter = 0;
  Serial.println("No se han encontrado dispositivos cerca de la puerta ni a través de wifi ni a través de bluetooth");
  delay(10000);
}
