
#include "MacScan.h"
#include <WiFi.h>
#include <Wire.h>
using namespace std;

/**
 * Clase que implementa la funcionalidad de escaneo mac a través de la wifi
 */
class WifiMacScan : public MacScan {

public:
    bool scan(void *buf, wifi_promiscuous_pkt_type_t type) override;
};