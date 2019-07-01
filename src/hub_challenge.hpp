// Byteduino lib - papabyte.com
// MIT License

#ifndef _HUB_CHALLENGE_H_
#define _HUB_CHALLENGE_H_

#include "byteduino.hpp"
#include "lib/SHA256.hpp"
#include "signature.hpp"

void respondToHubChallenge(const char* challenge);

#endif