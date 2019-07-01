// Byteduino lib - papabyte.com
// MIT License

#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include "lib/json.hpp"
#include "byteduino.hpp"
#include "hub_challenge.hpp"
#include "wallet.hpp"
#include "pairing.hpp"

void respondToHub(uint8_t *payload);
void respondToRequestFromHub(nlohmann::json arr);
void respondToJustSayingFromHub(nlohmann::json justSayingObject);

void sendErrorResponse(const char* tag, const char* error);
void sendHeartbeat();
void treatResponseFromHub(nlohmann::json arr);
void treatReceivedPackage();
void requestCorrespondentPubkey(char senderPubkey [45]);
void getTag(char * tag, const char * extension);
//void secondWebSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void connectSecondaryWebsocket();
bool isValidArrayFromHub(const nlohmann::json& arr);
int sendTxtMessage(const char recipientPubkey [45],const char * deviceHub, const char * text);
void setCbTxtMessageReceived(cbMessageReceived cbToSet);
std::string getDomain(const char * hub);
std::string getPath(const char * hub);

#endif