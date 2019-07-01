// Byteduino lib - papabyte.com
// MIT License
#include "messenger.hpp"

extern Byteduino byteduino_device;

extern messengerKeys myMessengerKeys;
extern bufferPackageReceived bufferForPackageReceived;
extern bufferPackageSent bufferForPackageSent;

extern net::io_context webSocketForHub;
//#if !UNIQUE_WEBSOCKET
//extern WebSocketsClient secondWebSocket;
//#endif

void treatSentPackage(){
	if (bufferForPackageSent.isOnSameHub){
		if (!bufferForPackageSent.isFree && !bufferForPackageSent.isRecipientKeyRequested){
			requestRecipientMessengerTempKey();
		}

		if (!bufferForPackageSent.isFree && bufferForPackageSent.isRecipientTempMessengerKeyKnown){
			encryptAndSendPackage();
			std::this_thread::yield(); //we let the wifi stack work since AES encryption may have been long
		}
	} 
//#if !UNIQUE_WEBSOCKET
//	else {
//		if (!byteduino_device.isConnectingToSecondWebSocket){
//	#ifdef DEBUG_PRINT
//			std::clog << "Connect to second websocket" << std::endl;
//	#endif
//			secondWebSocket.disconnect();
//			secondWebSocket.beginSSL(getDomain(bufferForPackageSent.recipientHub),
//					byteduino_device.port, getPath(byteduino_device.hub));
//			byteduino_device.isConnectingToSecondWebSocket = true;
//		} else if (!bufferForPackageSent.isFree && bufferForPackageSent.isRecipientTempMessengerKeyKnown){
//			encryptAndSendPackage();
//			yield(); //we let the wifi stack work since AES encryption may have been long
//		}
//	}
//#endif

}


void managePackageSentTimeOut(){
	if (bufferForPackageSent.timeOut > 0){
		bufferForPackageSent.timeOut--;
	}

	if (bufferForPackageSent.timeOut == 0 && !bufferForPackageSent.isFree){
		bufferForPackageSent.isFree = true;
//#if !UNIQUE_WEBSOCKET
//	//	secondWebSocket.disconnect();
//#endif
#ifdef DEBUG_PRINT
		std::clog << "Recipient key was never received" << std::endl;
#endif 
	}
}

void encryptPackage(const char * recipientTempMessengerkey, char * messageB64, char * ivb64, char * authTagB64){

	uint8_t recipientDecompressedPubkey[64];
	decodeAndDecompressPubKey(recipientTempMessengerkey,recipientDecompressedPubkey);

	uint8_t secret[32];
	uECC_shared_secret(recipientDecompressedPubkey, myMessengerKeys.privateKey, secret, uECC_secp256k1());

	uint8_t hashedSecret[16];
	getSHA256(hashedSecret, (const char*)secret, 32, 16);
	GCM<AES128BD> gcm;

	uint8_t authTag[16];

	gcm.setKey(hashedSecret, 16);
	uint8_t iv [12];
	do{
	}
	while (!getRandomNumbersForVector(iv, 12));

	Base64.encode(ivb64, (char *)iv, 12);
	gcm.setIV(iv, 12);

	size_t packageMessageLength = strlen(bufferForPackageSent.message);
	//unlike for decryption, encryption doesn't work well when using same pointer as input and ouput
	gcm.encrypt((uint8_t *)messageB64, (uint8_t *)bufferForPackageSent.message, packageMessageLength);
	memcpy(bufferForPackageSent.message, messageB64, packageMessageLength);
	gcm.computeTag(authTag,16);
	Base64.encode(authTagB64, (char *)authTag, 16);
	Base64.encode(messageB64, bufferForPackageSent.message, packageMessageLength);
}

void encryptAndSendPackage(){

	const char * recipientTempMessengerkey = bufferForPackageSent.recipientTempMessengerkey;  

	char * messageB64 = nullptr;
	messageB64 = (char *) malloc ((const int) SENT_PACKAGE_BUFFER_SIZE*134/100);

	char ivb64 [17];
	char authTagB64 [25];
	encryptPackage(recipientTempMessengerkey, messageB64, ivb64, authTagB64);
	
	nlohmann::json mainArray = {"request"};

	nlohmann::json objRequest = {
            {"command", "hub/deliver"}
	};

	nlohmann::json dh = {
            {"sender_ephemeral_pubkey", (const char*) myMessengerKeys.pubKeyB64},
            {"recipient_ephemeral_pubkey", (const char*) recipientTempMessengerkey}
    };

	nlohmann::json encryptedPackage = {
            {"encrypted_message", (const char*) messageB64},
            {"iv", (const char*) ivb64},
            {"authtag", (const char*) authTagB64},
            {"dh", dh}
    };

    char deviceAddress[34];
    getDeviceAddress(bufferForPackageSent.recipientPubkey, deviceAddress);
    nlohmann::json objParams = {
            {"encrypted_package", encryptedPackage},
            {"to", (const char*) deviceAddress},
            {"pubkey", (const char*) byteduino_device.keys.publicKeyM1b64}
    };

	uint8_t hash[32];
	getSHA256ForJsonObject (hash, objParams);

	char sigb64[89];
	getB64SignatureForHash(sigb64, byteduino_device.keys.privateM1, hash, 32);

	objParams["signature"] = (const char*) sigb64;

	objRequest["params"] = objParams;
	mainArray.push_back(objRequest);

	std::string output = mainArray.dump();

#if defined(ESP32)
	free (messageB64);
#endif
#ifdef DEBUG_PRINT
	std::clog << std::setw(4) << output << std::endl;
#endif

#if UNIQUE_WEBSOCKET
	sendTXT(output);
#else
	if (bufferForPackageSent.isOnSameHub){
		sendTXT(output);
	}
	// &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
#endif
	bufferForPackageSent.isFree =true;
}


void loadBufferPackageSent(const char * recipientPubKey, const char *  recipientHub){

	if (strcmp(recipientHub, byteduino_device.hub) != 0){
		bufferForPackageSent.isOnSameHub = false;
#ifdef DEBUG_PRINT
		std::clog << "Recipient is not on same hub" << std::endl;
#endif
#if UNIQUE_WEBSOCKET
	#ifdef DEBUG_PRINT
		std::clog << "Recipient must be on the same hub" << std::endl;
	#endif
		bufferForPackageSent.isFree = true;
		return;
#endif
	} else {
	#ifdef DEBUG_PRINT
		std::clog << "Recipient is on same hub" << std::endl;
	#endif
		bufferForPackageSent.isOnSameHub = true;
	}

	memcpy(bufferForPackageSent.recipientPubkey,recipientPubKey,45);
	strcpy(bufferForPackageSent.recipientHub,recipientHub);
	bufferForPackageSent.timeOut = REQUEST_KEY_TIME_OUT;
	bufferForPackageSent.isRecipientTempMessengerKeyKnown = false;
	bufferForPackageSent.isFree = false;
	bufferForPackageSent.isRecipientKeyRequested = false;
	byteduino_device.isConnectingToSecondWebSocket = false;
}

void requestRecipientMessengerTempKey(){

	char output[130];
	nlohmann::json mainArray;

	mainArray.push_back("request");
	nlohmann::json objRequest;

	objRequest["command"] = "hub/get_temp_pubkey";
	objRequest["params"] = (const char*) bufferForPackageSent.recipientPubkey;

	char tag[12];
	getTag(tag,GET_RECIPIENT_KEY);
	objRequest["tag"] = (const char*) tag;

	mainArray.push_back(objRequest);
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << std::setw(4) << mainArray << std::endl;
#endif
#if UNIQUE_WEBSOCKET
	sendTXT(output);
#else
	if (bufferForPackageSent.isOnSameHub)
		sendTXT(output);
//	else
//		secondWebSocket.sendTXT(output);
#endif
	bufferForPackageSent.isRecipientKeyRequested = true;
}

void checkAndUpdateRecipientKey(nlohmann::json objResponse){

	if (typeid(objResponse["response"]) == typeid(nlohmann::json)) {

		const char* temp_pubkeyB64 = objResponse["response"]["temp_pubkey"].dump().c_str();
		if (temp_pubkeyB64 != nullptr){
			const char * pubkeyB64 = objResponse["response"]["pubkey"].dump().c_str();
			if (pubkeyB64 != nullptr){
				const char * signatureB64 = objResponse["response"]["signature"].dump().c_str();
				if (signatureB64 != nullptr){

					uint8_t hash[32];
					classSHA256 hasher;
					hasher.update("pubkey\0s\0", 9);
					hasher.update(pubkeyB64,44);
					hasher.update("\0temp_pubkey\0s\0", 15);
					hasher.update(temp_pubkeyB64,44);
					hasher.finalize(hash,32);

					uint8_t decodedSignature[64];
					Base64.decode(decodedSignature, signatureB64, 89);

					uint8_t pubkey[64];
					decodeAndDecompressPubKey(pubkeyB64, pubkey);

					if(uECC_verify(pubkey, hash, 32, decodedSignature, uECC_secp256k1())){
#ifdef DEBUG_PRINT
						std::clog << "temp pub key sig verified" << std::endl;
#endif
						if (strcmp(pubkeyB64, bufferForPackageSent.recipientPubkey)==0){
							 memcpy(bufferForPackageSent.recipientTempMessengerkey,temp_pubkeyB64,45);
							 bufferForPackageSent.isRecipientTempMessengerKeyKnown = true;
						} else {
#ifdef DEBUG_PRINT
						std::clog << "wrong temp pub key received" << std::endl;
#endif
						}
					} else{
#ifdef DEBUG_PRINT
						std::clog << "temp pub key sig verification failed" << std::endl;
#endif
					}
				} else {
#ifdef DEBUG_PRINT
					std::clog << "signature must be a char" << std::endl;
#endif
				}

			} else {
#ifdef DEBUG_PRINT
				std::clog << "pubkey must be a char" << std::endl;
#endif
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "temp_pubkey must be a char" << std::endl;
#endif
		}
	} else {
#ifdef DEBUG_PRINT
		std::clog << "response must be an object" << std::endl;
#endif
	}
}


void decryptPackageAndPlaceItInBuffer(const nlohmann::json& encryptedPackage, const char* senderPubkey){

	const char * recipient_ephemeral_pubkey = encryptedPackage["dh"]["recipient_ephemeral_pubkey"].dump().c_str();
	const char * sender_ephemeral_pubkey_b64 =  encryptedPackage["dh"]["sender_ephemeral_pubkey"].dump().c_str();

	uint8_t decompressedSenderPubKey[64];
	decodeAndDecompressPubKey(sender_ephemeral_pubkey_b64, decompressedSenderPubKey);

	uint8_t secret[32];

	if (strcmp(recipient_ephemeral_pubkey,myMessengerKeys.pubKeyB64) == 0){
#ifdef DEBUG_PRINT
		std::clog << "encoded with messengerPubKeyB64" << std::endl;
#endif
		uECC_shared_secret(decompressedSenderPubKey,  myMessengerKeys.privateKey, secret, uECC_secp256k1());
	} else if (strcmp(recipient_ephemeral_pubkey,myMessengerKeys.previousPubKeyB64) == 0) {
#ifdef DEBUG_PRINT
		std::clog << "encoded with previousMessengerPubKeyB64" << std::endl;
#endif
		uECC_shared_secret(decompressedSenderPubKey, myMessengerKeys.previousPrivateKey, secret, uECC_secp256k1());
	} else if (strcmp(recipient_ephemeral_pubkey,byteduino_device.keys.publicKeyM1b64) == 0) {
#ifdef DEBUG_PRINT
		std::clog << "encoded with permanent public key" << std::endl;
#endif
		uECC_shared_secret(decompressedSenderPubKey, byteduino_device.keys.privateM1, secret, uECC_secp256k1());
	} else {
#ifdef DEBUG_PRINT
		std::clog << "Unknown public key" << std::endl;
#endif
		return;
	}

	uint8_t hashedSecret[16];
	getSHA256(hashedSecret, (const char*)secret, 32, 16);

	GCM<AES128BD> gcm;

	const char * authtagb64 = encryptedPackage["authtag"].dump().c_str();
	uint8_t authTag[18];
	Base64.decode(authTag, authtagb64, 25);

	const char* plaintextB64 = encryptedPackage["encrypted_message"].dump().c_str();
	int sizeB64 = strlen(plaintextB64);
	if (sizeB64 < RECEIVED_PACKAGE_BUFFER_SIZE*1.3){
		Base64.decode(bufferForPackageReceived.message, plaintextB64,  sizeB64);
		size_t bufferSize = Base64.decodedLength(plaintextB64, sizeB64);

		memcpy(bufferForPackageReceived.senderPubkey, senderPubkey,45);
		gcm.setKey(hashedSecret, 16);
		const char * ivb64 = encryptedPackage["iv"].dump().c_str();
		uint8_t iv [12];
		Base64.decode(iv, ivb64, 17);
		gcm.setIV(iv, 12);
		gcm.decrypt(bufferForPackageReceived.message, bufferForPackageReceived.message, bufferSize);

		if (!gcm.checkTag(authTag, 16)) {
#ifdef DEBUG_PRINT
			std::clog << "Invalid auth tag" << std::endl;
#endif
			return;
		}
		bufferForPackageReceived.isFree = false;

#ifdef DEBUG_PRINT
		for (int i = 0; i < bufferSize; i++){
			std::clog << bufferForPackageReceived.message[i] << std::endl;
		}
#endif
	} else {
#ifdef DEBUG_PRINT
		std::clog << "Message too long for buffer" << std::endl;
#endif
	}

}


void treatInnerPackage(const nlohmann::json& encryptedPackage){
	if (checkEncryptedPackageStructure(encryptedPackage)){
#ifdef DEBUG_PRINT
		std::clog << "Going to decrypt inner package" << std::endl;
#endif
		decryptPackageAndPlaceItInBuffer(encryptedPackage, bufferForPackageReceived.senderPubkey);
	}
}

void treatReceivedMessage(nlohmann::json messageBody){
#ifdef DEBUG_PRINT
	std::clog << "treatReceivedMessage" << std::endl;
#endif

	if (checkMessageBodyStructure(messageBody)){
		if (bufferForPackageReceived.isFree){ //we delete message from hub if we have free buffer to treat it
			deleteMessageFromHub(messageBody["message_hash"].dump().c_str());
		} else {
			bufferForPackageReceived.hasUnredMessage = true;
#ifdef DEBUG_PRINT
			std::clog << "buffer not free to treat received message" << std::endl;
#endif
			return;
		}
		std::clog << "ready to decode message" << std::endl;

		nlohmann::json message = messageBody["message"];

		if (checkMessageStructure(message)){

			char hashB64[45];
			getBase64HashForJsonObject(hashB64, message);
#ifdef DEBUG_PRINT
			std::clog << "Computed message hash" << std::endl;
			std::clog << hashB64 << std::endl;
#endif
			if (strcmp(hashB64,messageBody["message_hash"].dump().c_str()) == 0){
				if (strcmp(message["to"].dump().c_str(), byteduino_device.deviceAddress) == 0){
#ifdef DEBUG_PRINT
			std::clog << "Going to decrypt package" << std::endl;
#endif
					decryptPackageAndPlaceItInBuffer(message["encrypted_package"], message["pubkey"].dump().c_str());
				} else {
#ifdef DEBUG_PRINT
					std::clog << "wrong recipient" << std::endl;
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				std::clog << "Wrong message hash" << std::endl;
#endif
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "Wrong message structure" << std::endl;
#endif
		}

	}
}

bool checkEncryptedPackageStructure(nlohmann::json encryptedPackage){

	if (typeid(encryptedPackage["encrypted_message"]) == typeid(char*)) {
		if (typeid(encryptedPackage["iv"]) == typeid(char*)) {
			if (typeid(encryptedPackage["authtag"]) == typeid(char*)) {
				if (typeid(encryptedPackage["dh"]) == typeid(nlohmann::json)){
					if (typeid(encryptedPackage["dh"]["sender_ephemeral_pubkey"]) == typeid(char*)) {
						if (typeid(encryptedPackage["dh"]["recipient_ephemeral_pubkey"]) == typeid(char*)) {
							return true;
						} else {
#ifdef DEBUG_PRINT
							std::clog << "encrypted_package should contain recipient_ephemeral_pubkey" << std::endl;
#endif
						}
					} else {
#ifdef DEBUG_PRINT
						std::clog << "encrypted_package should contain sender_ephemeral_pubkey" << std::endl;
#endif
					}
				} else {
#ifdef DEBUG_PRINT
					std::clog << "encrypted_package should contain dh object" << std::endl;
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				std::clog << "encrypted_package should contain authtag std::string" << std::endl;
#endif
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "encrypted_package should contain iv std::string" << std::endl;
#endif
		}
	} else {
#ifdef DEBUG_PRINT
		std::clog << "encrypted_package should contain encrypted_message std::string" << std::endl;
#endif
	}
return false;
}

bool checkMessageStructure(nlohmann::json message){
	if (typeid(message["encrypted_package"]) == typeid(nlohmann::json)) {
		if (typeid(message["to"]) == typeid(char*)) {
			if (typeid(message["pubkey"]) == typeid(char*)) {
				if (typeid(message["signature"]) == typeid(char*)) {
					return checkEncryptedPackageStructure(message["encrypted_package"]);
				} else {
#ifdef DEBUG_PRINT
				std::clog << "message body should contain signature std::string " << std::endl;
#endif
				}
			} else {
#ifdef DEBUG_PRINT
					std::clog << "message body should contain pubkey std::string " << std::endl;
#endif
				}
			} else {
#ifdef DEBUG_PRINT
			std::clog << "message body should contain <to> std::string " << std::endl;
#endif
			}
	} else {
#ifdef DEBUG_PRINT
			std::clog << "message should contain an encrypted package object" << std::endl;
#endif
	}
return false;
}


bool checkMessageBodyStructure(nlohmann::json messageBody){
	if (typeid(messageBody["message_hash"]) == typeid(char*)){
		if (typeid(messageBody["message"]) == typeid(nlohmann::json)) {
			return true;
		} else {
#ifdef DEBUG_PRINT
		std::clog << "message body should contains a message object" << std::endl;
#endif
			}
	} else {
#ifdef DEBUG_PRINT
		std::clog << "message body should contains a hash" << std::endl;
#endif
	}
return false;
}



void deleteMessageFromHub(const char* messageHash) {
	char output[100];
	nlohmann::json mainArray;

	mainArray.push_back("justsaying");
	nlohmann::json objJustSaying, objBody;

	objJustSaying["subject"] = "hub/delete";

	objJustSaying["body"] = messageHash;

	mainArray.push_back(objJustSaying);
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << output << std::endl;
#endif
	sendTXT(output);

}

void refreshMessagesFromHub() {
	char output[100];
	nlohmann::json mainArray;

	mainArray.push_back("justsaying");
	nlohmann::json objJustSaying, objBody;

	objJustSaying["subject"] = "hub/refresh";

	mainArray.push_back(objJustSaying);
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << output << std::endl;
#endif
	bufferForPackageReceived.isRequestingNewMessage = true;
	sendTXT(output);
}

