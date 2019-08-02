// Byteduino lib - papabyte.com
// MIT License

#include "communication.hpp"


extern net::io_context webSocketForHub;
extern Base64Class Base64;

extern Byteduino byteduino_device;
extern bufferPackageReceived bufferForPackageReceived;
extern bufferPackageSent bufferForPackageSent;
cbMessageReceived _cbMessageReceived;


void setCbTxtMessageReceived(cbMessageReceived cbToSet){
	_cbMessageReceived = cbToSet;
}

bool isValidArrayFromHub(const nlohmann::json& arr){
	
	if (arr.size() != 2) {
#ifdef DEBUG_PRINT
		std::clog << "Array should have 2 elements" << std::endl;
#endif
		return false;
	}

	if (typeid(arr[0]) != typeid(char*))  {
#ifdef DEBUG_PRINT
		std::clog << "First array element should be a char" << std::endl;
#endif
		return false;
	}

	if (typeid(arr[1]) != typeid(nlohmann::json))  {
#ifdef DEBUG_PRINT
		std::clog << "Second array should be an object" << std::endl;
#endif
		return false;
	}
return true;
}


void respondToHub(uint8_t *payload) {
#ifdef DEBUG_PRINT
	std::clog << "Received: " << payload << std::endl;
#endif
    nlohmann::json arr;
	try {
        arr = nlohmann::json::parse(payload);
    } catch(...) {
#ifdef DEBUG_PRINT
		std::clog << "Deserialization failed" << std::endl;
#endif
		return;
	}

	if (!isValidArrayFromHub(arr)){
		return;
	}

	if (strcmp(arr[0].dump().c_str(), "justsaying") == 0) {
		respondToJustSayingFromHub(arr[1]);
	} else if (strcmp(arr[0].dump().c_str(), "request") == 0) {
		respondToRequestFromHub(arr);
	} else if (strcmp(arr[0].dump().c_str(), "response") == 0) {
		treatResponseFromHub(arr);
	}

}

std::string getDomain(const char * hub){
	std::string returnedString (hub);
	std::size_t slashIndex = returnedString.find('/');
	returnedString = returnedString.substr(0, slashIndex);
	return returnedString;
}

std::string getPath(const char * hub){
	std::string returnedString (hub);
	std::size_t slashIndex = returnedString.find('/');
	returnedString = returnedString.substr(slashIndex);
	return returnedString;
}

//#if !UNIQUE_WEBSOCKET
//
//void treatResponseFromSecondWebsocket(nlohmann::json arr){
//	if (typeid(arr[1]) == typeid(nlohmann::json)) {
//		const char* tag = arr[1]["tag"].dump().c_str();
//		if (tag != nullptr) {
//			if  (tag[9] == GET_RECIPIENT_KEY[1]){
//				checkAndUpdateRecipientKey(arr[1]);
//			}
//		}
//	} else {
//#ifdef DEBUG_PRINT
//		std::clog << "Second array second websocket should contain an object" << std::endl;
//#endif
//	}
//}
//
//
//void secondWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
//
//	switch (type) {
//		case WStype_DISCONNECTED:
//#ifdef DEBUG_PRINT
//			std::clog << "2nd Wss disconnected\n" << std::endl;
//#endif
//			break;
//		case WStype_CONNECTED: {
//#ifdef DEBUG_PRINT
//std::clog << "2nd Wss connected to: %s\n" << payload << std::endl;
//#endif
//			if (!bufferForPackageSent.isFree && !bufferForPackageSent.isRecipientKeyRequested)
//				requestRecipientMessengerTempKey();
//            break;
//		}
//		case WStype_TEXT: {
//			std::clog << "2nd Wss received: %s\n" << payload << std::endl;
//            nlohmann::json arr;
//			try {
//                arr = nlohmann::json::parse(payload);
//			} catch(...) {
//#ifdef DEBUG_PRINT
//				std::clog << "Deserialization failed" << std::endl;
//#endif
//				return;
//			}
//
//			if (!isValidArrayFromHub(arr)){
//				return;
//			}
//			if (strcmp(arr[0].dump().c_str(), "response") == 0) {
//				treatResponseFromSecondWebsocket(arr);
//			}
//			break;
//		}
//	}
//}
//
//#endif

int sendTxtMessage(const char recipientPubKey [45], const char * recipientHub, const char * text){

	if (bufferForPackageSent.isFree){
		if (strlen(recipientHub) < MAX_HUB_STRING_SIZE){
			if (strlen(recipientPubKey) == 44){
				if (strlen(text) < (SENT_PACKAGE_BUFFER_SIZE - 108)){
					nlohmann::json message;
					message["from"] = (const char*) byteduino_device.deviceAddress;
					message["device_hub"] = (const char*) byteduino_device.hub;
					message["subject"] = "text";

					message["body"]= text;

					loadBufferPackageSent(recipientPubKey, recipientHub);
					strcpy(bufferForPackageSent.message, message.dump().c_str());
#ifdef DEBUG_PRINT
					std::clog << std::setw(4) << message << std::endl;
#endif
					return SUCCESS;
				} else {
#ifdef DEBUG_PRINT
					std::clog << "text too long" << std::endl;
#endif
					return TEXT_TOO_LONG;
				}
			} else {
#ifdef DEBUG_PRINT
				std::clog << "wrong pub key size" << std::endl;
#endif
				return WRONG_PUBKEY_SIZE;
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "hub url too long" << std::endl;
#endif
			return HUB_URL_TOO_LONG;
		}

	} else {
#ifdef DEBUG_PRINT
			std::clog << "Buffer not free to send message" << std::endl;
#endif
		return BUFFER_NOT_FREE;
	}
}

void treatReceivedPackage(){
	
	if (!bufferForPackageReceived.isFree){
		
		nlohmann::json receivedPackage = nlohmann::json::parse(bufferForPackageReceived.message);
		if (!receivedPackage.empty()) {
			
			if (typeid(receivedPackage["encrypted_package"]) == typeid(nlohmann::json)){
				std::clog << "inner encrypted package" << std::endl;
				treatInnerPackage(receivedPackage["encrypted_package"]);
				return;
			}

			const char* subject = receivedPackage["subject"].dump().c_str();

			if (subject != nullptr) {

				if (strcmp(subject, "pairing") == 0){
					if (typeid(receivedPackage["body"]) == typeid(nlohmann::json)){
#ifdef DEBUG_PRINT
						std::clog << "handlePairingRequest" << std::endl;
#endif
						handlePairingRequest(receivedPackage);
					}

				} else if (strcmp(subject, "text") == 0){
#ifdef DEBUG_PRINT
					std::clog << "handle text" << std::endl;
#endif
					const char * messageDecoded = receivedPackage["body"].dump().c_str();
					const char * senderHub = receivedPackage["device_hub"].dump().c_str();
					if (messageDecoded != nullptr && senderHub != nullptr) {
						if(_cbMessageReceived){
							_cbMessageReceived(bufferForPackageReceived.senderPubkey, senderHub, messageDecoded);
						}
					} else {
#ifdef DEBUG_PRINT
						std::clog << "body and device_hub must be char" << std::endl;
#endif
					}

				}
#ifndef REMOVE_COSIGNING
				
				else if (strcmp(subject,"create_new_wallet") == 0){
					if (typeid(receivedPackage["body"]) == typeid(nlohmann::json)){
	#ifdef DEBUG_PRINT
						std::clog << "handle new wallet" << std::endl;
	#endif
						handleNewWalletRequest(bufferForPackageReceived.senderPubkey,receivedPackage);
					}

				} else if (strcmp(subject, "sign") == 0){
					if (typeid(receivedPackage["body"]) == typeid(nlohmann::json)){
	#ifdef DEBUG_PRINT
						std::clog << "handle signature request" << std::endl;
	#endif
						handleSignatureRequest(bufferForPackageReceived.senderPubkey,receivedPackage);
					}

				}
#endif
			} else {
#ifdef DEBUG_PRINT
				std::clog << "no subject for received message" << std::endl;
#endif
			}
	} else {
#ifdef DEBUG_PRINT
			std::clog << "Deserialization failed" << std::endl;
#endif
		}
	bufferForPackageReceived.isFree = true;
	}
}


void treatResponseFromHub(nlohmann::json arr){
	if (typeid(arr[1]) == typeid(nlohmann::json)) {
		const char* tag = arr[1]["tag"].dump().c_str();
		if (tag != nullptr) {
			if (tag[9] == HEARTBEAT[1]){
#ifdef DEBUG_PRINT
//				std::clog << ESP.getFreeHeap() << std::endl;
				std::clog << "Heartbeat acknowledged by hub" << std::endl;
#endif
			} else if (tag[9] == UPDATE_MESSENGER_KEY[1]){
#ifdef DEBUG_PRINT
				std::clog << "Ephemeral key updated" << std::endl;
#endif
			} else if (tag[9] == GET_RECIPIENT_KEY[1]){
#ifdef DEBUG_PRINT
				std::clog << "recipient pub key received" << std::endl;
#endif
				checkAndUpdateRecipientKey(arr[1]);
			} else if (tag[9] == GET_ADDRESS_DEFINITION[1]){
#ifdef DEBUG_PRINT
				std::clog << "definition received" << std::endl;
#endif
				handleDefinition(arr[1]);
			} else if (tag[9] == GET_INPUTS_FOR_AMOUNT[1]){
#ifdef DEBUG_PRINT
				std::clog << "inputs received" << std::endl;
#endif
				handleInputsForAmount(arr[1], tag);
			} else if (tag[9] == GET_PARENTS_BALL_WITNESSES[1]){
#ifdef DEBUG_PRINT
				std::clog << "units props received" << std::endl;
#endif
				handleUnitProps(arr[1]);
			} else if (tag[9] == POST_JOINT[1]){
#ifdef DEBUG_PRINT
				std::clog << "post joint result received" << std::endl;
#endif
				handlePostJointResponse(arr[1], tag);
			} else if (tag[9] == GET_BALANCE[1]){
#ifdef DEBUG_PRINT
				std::clog << "balance received" << std::endl;
#endif
				handleBalanceResponse(arr[1]);
			} else {
#ifdef DEBUG_PRINT
				std::clog << "wrong tag id for response" << std::endl;
#endif
			}
		} else {
#ifdef DEBUG_PRINT
		std::clog << "Second array of response should contain a object" << std::endl;
#endif
		}
	}
}


void respondToRequestFromHub(nlohmann::json arr) {

	const char* command = arr[1]["command"].dump().c_str();

	if (command != nullptr) {
		if (strcmp(command, "subscribe") == 0) {
			const char* tag = arr[1]["tag"].dump().c_str();
			if (tag != nullptr) {
				sendErrorResponse(tag, "I'm a microdevice, cannot subscribe you to updates");
			} else {
#ifdef DEBUG_PRINT
				std::clog << "Second array should contain a tag" << std::endl;
#endif
			}
		return;
		} else if (strcmp(command, "heartbeat") == 0) {
			const char* tag = arr[1]["tag"].dump().c_str();
			if (tag != nullptr) {
			//   sendHeartbeatResponse(tag);
			} else {
#ifdef DEBUG_PRINT
				std::clog << "Second array should contain a tag" << std::endl;
#endif
			}
		}

	} else {
#ifdef DEBUG_PRINT
		std::clog << "Second array should contain a command" << std::endl;
#endif
	}

}

void respondToJustSayingFromHub(nlohmann::json justSayingObject) {

	const char* subject = justSayingObject["subject"].dump().c_str();

	if (subject != nullptr) {

		if (strcmp(subject, "hub/challenge") == 0) {
			const char* body = justSayingObject["body"].dump().c_str();
			if (body != nullptr) {
				respondToHubChallenge(body);
			} else {
#ifdef DEBUG_PRINT
				std::clog << "justSayingObject should contain a body" << std::endl;
#endif
			}
		} else if (strcmp(subject, "hub/push_project_number") == 0) {
			byteduino_device.isAuthenticated = true;
			std::clog << "Authenticated by hub" << std::endl;
			return;
		} else if (strcmp(subject, "hub/message") == 0) {
			if (typeid(justSayingObject["body"]) == typeid(nlohmann::json)) {
				nlohmann::json messageBody = justSayingObject["body"];
				treatReceivedMessage(messageBody);
			}
		} else if (strcmp(subject, "hub/message_box_status") == 0) {
			bufferForPackageReceived.isRequestingNewMessage = false;
			const char* body = justSayingObject["body"].dump().c_str();
			if (body != nullptr) {
				if (strcmp(body, "empty") != 0){
					bufferForPackageReceived.hasUnredMessage = true;
#ifdef DEBUG_PRINT
					std::clog << "Message box not empty" << std::endl;
#endif
				} else{
					bufferForPackageReceived.hasUnredMessage = false;
#ifdef DEBUG_PRINT
					std::clog << "Empty message box" << std::endl;
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				std::clog << "justSayingObject should contain a body" << std::endl;
#endif
			}
		} 
	} else {
#ifdef DEBUG_PRINT
		std::clog << "Second array should contain a subject" << std::endl;
#endif
	}
}


void sendErrorResponse(const char* tag, const char* error) {

	char output[256];
	nlohmann::json mainArray;

	mainArray.push_back("response");
	nlohmann::json objResponse, objError;

	objError["error"] = error;
	objResponse["tag"] = tag;
	objResponse["response"] = objError;

	mainArray.push_back(objResponse);
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << std::setw(4) << mainArray << std::endl;
#endif
	sendTXT(output);

}

void getTag(char * tag, const char * extension){
	uint8_t random[5];
	getRandomNumbersForTag(random,5);
	Base64.encode(tag, (char *) random, 5);
	memcpy(tag+8,extension,2);
	tag[10] = 0x00;
}

void sendHeartbeat(void) {
	
	char output[60];
	nlohmann::json mainArray;

	mainArray.push_back("request");
	nlohmann::json objRequest;

	objRequest["command"] = "heartbeat";
	
	char tag[12];
	getTag(tag,HEARTBEAT);
	objRequest["tag"] = (const char*) tag;
	
	mainArray.push_back(objRequest);
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << std::setw(4) << mainArray << std::endl;
#endif
	sendTXT(output);
}
