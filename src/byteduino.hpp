// Byteduino lib - papabyte.com
// MIT License

#ifndef _BYTEDUINO_H_
#define _BYTEDUINO_H_

//#include "lib/WebSocketsClient.hpp"
#include "lib/websocketclient.hpp"
#include "definitions.hpp"
#include "communication.hpp"
#include "structures.hpp"
#include "messenger.hpp"
#include "key_rotation.hpp"
#include "cosigning.hpp"
#include "wallet.hpp"
#include "payment.hpp"
#include "lib/micro-ecc/uECC.h"
#include "lib/Base64.hpp"
#include "random_gen.hpp"
//#include "lib/components/spi_flash/include/esp_partition.h"
//#include "lib/components/esp32/include/esp_attr.h"

#define DEBUG_PRINT
//#define REMOVE_COSIGNING

void byteduino_init ();
void webSocketEvent();

void byteduino_loop();
void timerCallback(void * pArg);
std::string getDeviceInfosJsonString();
void setHub(const char * hub);
void setDeviceName(const char * deviceName);
void setExtPubKey(const char * extPubKey);
void setPrivateKeyM1(const char * privKeyB64);
void setPrivateKeyM4400(const char * privKeyB64);
void printDeviceInfos();
void setTestNet();
bool isDeviceConnectedToHub();

#endif