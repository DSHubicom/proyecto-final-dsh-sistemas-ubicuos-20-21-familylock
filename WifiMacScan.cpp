
#include "WifiMacScan.h"


bool WifiMacScan::scan(void *buf, wifi_promiscuous_pkt_type_t type) {
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
    for (int i = 8; i <= 8 + 6 + 1; i++) { // This reads the first couple of bytes of the packet. This is where you can read the whole packet replaceing the "8+6+1" with "p->rx_ctrl.sig_len"
        packet += String(p->payload[i], HEX);
    }
    for (int i = 4; i <=
                    15; i++) { // This removes the 'nibble' bits from the stat and end of the data we want. So we only get the mac address.
        mac += packet[i];
    }
    mac.toUpperCase();


    int added = 0;
    for (int i = 0; i <= 63; i++) { // checks if the MAC address has been added before
        if (mac == maclist[i][0]) {
            maclist[i][1] = defaultTTL;
            if (maclist[i][2] == "OFFLINE") {
                maclist[i][2] = "0";
            }
            added = 1;
        }
    }

    if (added == 0) { // If its new. add it to the array.
        maclist[listcount][0] = mac;
        maclist[listcount][1] = defaultTTL;
        Serial.println(mac);
        listcount++;
        if (listcount >= 64) {
            Serial.println("Too many addresses");
            listcount = 0;
        }
    }
}
