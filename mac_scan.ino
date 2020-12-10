#include "BLEDevice.h"
#include <WiFi.h>
#include <WiFiMulti.h>

static BLEAddress *pServerAddress;
BLEScan* pBLEScan;
BLEClient*  pClient;

bool deviceFound = false;

int counter = 0;
int curChannel = 1;

String* knownMacAddress[10];
int last_position;

const char* ssid     = "argynos";
const char* password = "qogdy3kf";

const char* server = "muii-g2-api-legitimate.herokuapp.com";

WiFiMulti WiFiMulti;

bool findMac(String mac) {
  bool found = false;
  for (int i = 0; i < last_position & !found; i++) {
    if (mac == knownMacAddress[i]->c_str()) {
      found = true;
    }
  }
  return found;
}


void setMacAddrs(String mac) {
  // si la mac no está en la lista, lo añadimos
  knownMacAddress[last_position] = &mac;
  last_position = sizeof(knownMacAddress) / sizeof(knownMacAddress[0]);

}

/**
   Realiza una conexión a un AP
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
  Serial.print("WiFi connected to ");
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
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

}

/**
   Realiza una petición GET al servidor "server" usando la mac
   pasada por parámetro
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
  Serial.println("closing connection");
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice Device) {
      pServerAddress = new BLEAddress(Device.getAddress());
      bool known = false;
      bool Master = false;

      Serial.printf("%s: ", pServerAddress->toString().c_str());
      Serial.println(Device.getRSSI());

      for (int i = 0; i < (sizeof(knownMacAddress) / sizeof(knownMacAddress[0])); i++) {
        if (strcmp(pServerAddress->toString().c_str(), knownMacAddress[i]->c_str()) == 0)
          known = true;
      }
      if (known) {
        Serial.print("Device found: ");
        Serial.println(Device.toString().c_str());
        // Si la intensidad de la señal es muy buena, quiere decir que el dispositivo está muy cerca de la puerta
        // POr lo que podremos aceptar que pretende llamar a la vivienda.
        if (Device.getRSSI() > -60) {
          //realizar aquí la llamada al http_request
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
    Serial.println("BLE Scan restarted.....");
    BLEScanResults scanResults = pBLEScan->start(10);
    Serial.printf("Se han encontrado %d dispositivos...\n", scanResults.getCount());
    if (deviceFound) {
      Serial.println("Dispositivo encontrado");
      Serial.println("Notificando al usuario ");
      delay(10000);
    }
    else {
      Serial.println("Dispositivo no encontrado");
      delay(1000);
    }
  }
}



void setup() {
  Serial.begin(115200);
  connectToWifi();
  Bluetooth();
  Serial.println("Done");
}


void loop() {
  bool result = false;
  while (counter <= 5) {
    bluetoothScan();
    if (result) {
      Serial.println("Saliendo del bucle...");
      break;
    }
    counter ++;
  }
  counter = 0;
  Serial.println("No se han encontrado dispositivos cerca de la puerta ni a través de wifi ni a través de bluetooth");
  delay(10000);
}
