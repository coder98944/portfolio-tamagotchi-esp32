#ifndef NETWORKMGR_H
#define NETWORKMGR_H

#include "BluetoothSerial.h"

class NetworkMgr {
public:
    void begin();
    char checkRemoteInput();
    bool checkCheatCommand(int &hunger, int &happiness, int &energy);

private:
    BluetoothSerial SerialBT;
};

#endif
