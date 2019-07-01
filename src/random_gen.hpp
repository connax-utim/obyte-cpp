// Byteduino lib - papabyte.com
// MIT License

#ifndef _RANDOM_GEN_H_
#define _RANDOM_GEN_H_

#include <lib/components/soc/esp32/include/soc/soc.h>
#include <definitions.hpp>
#include <inttypes.h>

int getRandomNumbersForUecc(uint8_t *dest, unsigned size);
bool getRandomNumbersForVector(uint8_t *dest, unsigned size);
bool getRandomNumbersForPrivateKey(uint8_t *dest, unsigned size);
bool getRandomNumbersForTag(uint8_t *dest, unsigned size);
void updateRandomPool();
bool isRandomGeneratorReady();

#endif