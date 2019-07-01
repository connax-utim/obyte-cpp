// Byteduino lib - papabyte.com
// MIT License

#ifndef _KEY_ROTATION_H_
#define _KEY_ROTATION_H_

#include "byteduino.hpp"
#include "random_gen.hpp"
#include "signature.hpp"
#include "lib/SHA256.hpp"
#include <stdlib.h>
#include <fstream>


void setAndSendNewMessengerKey();
void getHashToSignForUpdatingMessengerKey(uint8_t* hash);
void getB64SignatureForUpdatingMessengerKey(char* sigb64, const uint8_t* hash);
void rotateMessengerKey();
void loadPreviousMessengerKeys();
void manageMessengerKey();

#endif