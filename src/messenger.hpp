// Byteduino lib - papabyte.com
// MIT License

#ifndef _MESSENGER_H_
#define _MESSENGER_H_

#include "byteduino.hpp"
#include "lib/json.hpp"
#include "hashing.hpp"
#include "signature.hpp"

#include "lib/GCM.hpp"
#include "lib/AES.hpp"
#include <thread>
#include <typeinfo>


void treatReceivedMessage(nlohmann::json messageBody);
bool checkMessageBodyStructure(nlohmann::json messageBody);
bool checkEncryptedPackageStructure(nlohmann::json encryptedPackage);
bool checkMessageStructure(nlohmann::json message);
void deleteMessageFromHub(const char* messageHash);
void decryptPackageAndPlaceItInBuffer(const nlohmann::json& encryptedPackage, const char* senderPubkey);
void getMessageHashB64(char * hashB64, nlohmann::json message);
void requestRecipientMessengerTempKey();
void checkAndUpdateRecipientKey(nlohmann::json objResponse);
void encryptPackage(const char * recipientTempMessengerkey, char * messageB64,char * ivb64, char * authTagB64);
void encryptAndSendPackage();
void loadBufferPackageSent(const char * recipientPubKey, const char *  recipientHub);
void treatInnerPackage(const nlohmann::json& encryptedPackage);
void managePackageSentTimeOut();
void refreshMessagesFromHub();
void treatSentPackage();

#endif