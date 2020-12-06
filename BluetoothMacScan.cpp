
#include "BluetoothMacScan.h"

bool BluetoothMacScan::scan(bool &device_found) {
    if (counter <= 5) {
        Serial.println();
        Serial.println("BLE Scan restarted.....");
        BLEScanResults scanResults = pBLEScan->start(SCAN_TIME);
        Serial.printf("Se han encontrado %d dispositivos...\n", scanResults.getCount());
        if (device_found) {
            Serial.println("Dispositivo encontrado");
            Serial.println("Notificando al usuario ");
            delay(10000);
        }
        else{
            Serial.println("Dispositivo no encontrado");
            delay(1000);
        }
    }
    return device_found;
}

void BluetoothMacScan::checkMacAddr(string mac) {

}

BluetoothMacScan::BluetoothMacScan() {
    BLEDevice::init("");
    pClient  = BLEDevice::createClient();
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice Device){
        pServerAddress = new BLEAddress(Device.getAddress());
        bool known = false;
        bool Master = false;

        Serial.printf("%s: ", pServerAddress->toString().c_str());
        Serial.println(Device.getRSSI());

        for (int i = 0; i < (sizeof(knownAddresses)/sizeof(knownAddresses[0])); i++) {
            if (strcmp(pServerAddress->toString().c_str(), knownAddresses[i].c_str()) == 0)
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