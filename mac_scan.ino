#include "BLEDevice.h"
#include "BluetoothMacScan.h"

static BLEAddress *pServerAddress;
BLEScan* pBLEScan;
BLEClient*  pClient;
bool deviceFound = false;
int SIGNAL_INTENSITY = -60;
int SCAN_TIME = 5;
int counter = 0;

//Esto debemos obtenerlo de la api
String knownAddresses[] = { "DC:D9:16:33:AA:34", "E0:DC:FF:EB:F8:98", "4a:fb:c5:53:14:ab"};


void setup() {
  Serial.begin(115200);
  BLEDevice::init("");
  pClient  = BLEDevice::createClient();
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  Serial.println("Done");
}

bool Bluetooth() {
  if (counter <= 5) {
    Serial.println();
    Serial.println("BLE Scan restarted.....");
    deviceFound = false;
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
  return deviceFound;
}

bool Wifi() {
  Serial.println("Scanning wifi devices...");
  return false;
}
void loop() {
  bool result = false;
  while(counter <= SCAN_TIME) {
    result = Bluetooth();
    if (result) {
      Serial.println("Saliendo del bucle...");
      break;
    }
    counter ++;
  }
  counter = 0;
  if (result == false) {
    while(counter < 5) {
      bool result_w = Wifi();
      if (result_w) {
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
