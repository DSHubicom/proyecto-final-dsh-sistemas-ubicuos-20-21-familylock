#include "MacScan.h"
#include "BLEDevice.h"

using namespace std;

/**
 * Clase que implementa la funcionalidad de escaneo mac a través del bluetooth
 */
class BluetoothMacScan : public MacScan {
public:
    BluetoothMacScan();

    bool scan(bool &device_found) override;

    void checkMacAddr(string mac) override;

};