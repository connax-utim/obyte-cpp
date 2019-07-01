// Byteduino lib - papabyte.com
// MIT License

#include "hub_challenge.hpp"

extern net::io_context webSocketForHub;
extern Byteduino byteduino_device;


void respondToHubChallenge(const char* challenge) {

	if (strlen(challenge) != 40){
#ifdef DEBUG_PRINT
		std::clog << "challenge must be 40 chars long" << std::endl;
#endif
		return;
	}

	char output[320];
//	const int capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 20;
	nlohmann::json mainArray;

	mainArray.push_back("justsaying");
	nlohmann::json objJustSaying, objBody;

	objJustSaying["subject"] = "hub/login";
	objBody["challenge"] = challenge;
	objBody["max_message_count"] = MAX_MESSAGE_COUNT;
	objBody["max_message_length"] = MAX_MESSAGE_LENGTH;
	objBody["pubkey"] = (const char *) byteduino_device.keys.publicKeyM1b64;
	
	char sigb64[89];
	uint8_t hash[32];
	getSHA256ForJsonObject(hash, objBody);
	getB64SignatureForHash(sigb64, byteduino_device.keys.privateM1, hash, 32);
	
	objBody["signature"]  = (const char *) sigb64;
	objJustSaying["body"] = objBody;

	mainArray.push_back(objJustSaying);
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << std::setw(4) << mainArray.dump() << std::endl;
#endif
	sendTXT(output);

}