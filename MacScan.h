#ifndef MACSCAN_H_
#define MACSCAN_H_
#include <iostream>
#include <string>

using namespace std;

class MacScan {

private:
    int macAddr;
    string name;

public:

    virtual bool scan(bool &device_found) = 0;

    virtual void checkMacAddr(string mac) = 0;



};

#endif /* MACSCAN_H_ */
