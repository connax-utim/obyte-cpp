// Byteduino lib - papabyte.com
// MIT License

#include "cosigning.hpp"

walletCreation newWallet;
extern bufferPackageSent bufferForPackageSent;
extern Byteduino byteduino_device;
waitingConfirmationRoom waitingConfirmationSignature;

cbSignatureToConfirm _cbSignatureToConfirm;

void setCbSignatureToConfirm(cbSignatureToConfirm cbToSet){
	_cbSignatureToConfirm = cbToSet;
}

void removeKeyIfExisting(const char * key, nlohmann::json object){
	if (object.find(key) != object.end()){
		object.erase(key);
#ifdef DEBUG_PRINT
		std::clog << "remove key" << std::endl;
		std::clog << key << std::endl;
#endif
	}
	
}

std::string getOnGoingSignatureJsonString(){

	nlohmann::json mainObject;
	if (!waitingConfirmationSignature.isFree){
		mainObject["signedText"] = (const char *) waitingConfirmationSignature.signedText;
		mainObject["digest"] = (const char *) waitingConfirmationSignature.JsonDigest;
		mainObject["isConfirmed"] = (const bool) waitingConfirmationSignature.isConfirmed;
		mainObject["isRefused"] = (const bool) waitingConfirmationSignature.isRefused;
	}
	std::string returnedString = mainObject.dump();
	return returnedString;
}

bool acceptToSign(const char * signedTxt){
	for (int i=0;i<45;i++){
		if (signedTxt[i]!= waitingConfirmationSignature.signedText[i]){
			return false;
		}
	}
	waitingConfirmationSignature.isConfirmed = true;
	return true;
}

bool refuseTosign(const char * signedTxt){
	for (int i=0;i<45;i++){
		if (signedTxt[i]!= waitingConfirmationSignature.signedText[i]){
			return false;
		}
	}
	waitingConfirmationSignature.isRefused = true;
	return true;
}

void treatWaitingSignature(){
	if (!waitingConfirmationSignature.isFree &&
		(waitingConfirmationSignature.isConfirmed || waitingConfirmationSignature.isRefused)){
		if (bufferForPackageSent.isFree){
			waitingConfirmationSignature.isFree = true;
			nlohmann::json message;
			message["from"] = (const char*) byteduino_device.deviceAddress;
			message["device_hub"] = (const char*) byteduino_device.hub;
			message["subject"] = "signature";

			nlohmann::json objBody;
			objBody["signed_text"]= waitingConfirmationSignature.signedText;
			if (waitingConfirmationSignature.isRefused)
				objBody["signature"] = "[refused]";
			else
				objBody["signature"] = (const char*) waitingConfirmationSignature.sigb64;
			objBody["signing_path"] = (const char*) waitingConfirmationSignature.signing_path;
			objBody["address"] = (const char*) waitingConfirmationSignature.address;

			message["body"]= objBody;

			strcpy(bufferForPackageSent.message, message.dump().c_str());
			loadBufferPackageSent(waitingConfirmationSignature.recipientPubKey,
					waitingConfirmationSignature.recipientHub);
#ifdef DEBUG_PRINT
			std::clog << std::setw(4) << message << std::endl;
#endif
		} else {
#ifdef DEBUG_PRINT
			std::clog << "Buffer not free to send message" << std::endl;
#endif
		}
	}
}


void stripSignAndAddToConfirmationRoom(const char recipientPubKey[45], const char * recipientHub, nlohmann::json body){

	nlohmann::json unsignedUnit = body["unsigned_unit"];
	int authorsSize = unsignedUnit["authors"].size();

	for (int i=0; i < authorsSize; i++){
		nlohmann::json objAuthentifier = unsignedUnit["authors"][i];
		objAuthentifier.erase("authentifiers");
#ifdef DEBUG_PRINT
		std::clog << "remove authentifier" << std::endl;
#endif
	}
	
	//these unit properties aren't to be signed
	removeKeyIfExisting("unit", unsignedUnit);
	removeKeyIfExisting("headers_commission", unsignedUnit);
	removeKeyIfExisting("payload_commission", unsignedUnit);
	removeKeyIfExisting("main_chain_index", unsignedUnit);
	removeKeyIfExisting("timestamp", unsignedUnit);
	size_t messagesSize = unsignedUnit["messages"].size();
	
	//we create a JSON digest about what will signed
	nlohmann::json arrayDigest;
	for (size_t i=0; i < messagesSize; i++){
		const char * app = unsignedUnit["messages"][i]["app"].dump().c_str();
		nlohmann::json objApp;

		if (strcmp(app,"payment") == 0){
			objApp["type"] = "payment";
			if (typeid(unsignedUnit["messages"][i]["payload"]) == typeid(nlohmann::json)){
				nlohmann::json payload = unsignedUnit["messages"][i]["payload"];
				const char* payload_asset = payload["asset"].dump().c_str();
				if (payload_asset == nullptr)
					objApp["asset"] = (char*) "byte";
				else
					objApp["asset"] = (char *) payload_asset;
					
				if (typeid(payload["outputs"]) == typeid(nlohmann::json)){
					if (objApp.find("outputs") == objApp.end())
						objApp.push_back("outputs");
					size_t outputsSize = payload["outputs"].size();
					for (size_t j = 0; j<outputsSize; j++){
						const char* address = payload["outputs"][j]["address"].dump().c_str();
						if (address != nullptr && strlen(address) == 32) {
							if (typeid(payload["outputs"][j]["amount"]) == typeid(int)){
								int amountToAdd = payload["outputs"][j]["amount"];
								nlohmann::json objAppOutputs =	objApp["outputs"];
								if (objAppOutputs.find(address) != objAppOutputs.end()){
									int previousAmount = objAppOutputs[address];
									objAppOutputs[address] = previousAmount + amountToAdd;
								} else {
									objAppOutputs[(char *)address] = amountToAdd;
								}
							}
						}
					}
				}
				arrayDigest.push_back(objApp);
			}
		} else {
			if (typeid(unsignedUnit["messages"][i]["payload"]) == typeid(nlohmann::json)){
				arrayDigest.push_back(unsignedUnit["messages"][i]["payload"]);
			}
		}
		
		removeKeyIfExisting("payload", unsignedUnit["messages"][i]);
		removeKeyIfExisting("payload_uri", unsignedUnit["messages"][i]);
	}
	

	uint8_t hash[32];
	getSHA256ForJsonObject(hash, unsignedUnit);
	char sigb64 [89];
	getB64SignatureForHash(sigb64, byteduino_device.keys.privateM4400, hash, 32);

	const char * signing_path = body["signing_path"].dump().c_str();
	const char * address = body["address"].dump().c_str();
	if (signing_path != nullptr){
		if(strlen(signing_path) < MAX_SIGNING_PATH_SIZE){
			if (address != nullptr){
				if (strlen(address) == 32){
					strcpy(waitingConfirmationSignature.JsonDigest, arrayDigest.dump().c_str());
					std::clog << std::setw(4) << arrayDigest << std::endl;
					memcpy(waitingConfirmationSignature.recipientPubKey,recipientPubKey, 45);
					memcpy(waitingConfirmationSignature.hash, hash, 32);
					memcpy(waitingConfirmationSignature.sigb64, sigb64, 89);
					strcpy(waitingConfirmationSignature.signing_path, signing_path);
					memcpy(waitingConfirmationSignature.address, address, 33);
					strcpy(waitingConfirmationSignature.recipientHub, recipientHub);
					Base64.encode(waitingConfirmationSignature.signedText, (char *)hash, 32);
					waitingConfirmationSignature.isConfirmed = false;
					waitingConfirmationSignature.isRefused = false;
					waitingConfirmationSignature.isFree = false;
					if (_cbSignatureToConfirm){
						_cbSignatureToConfirm(waitingConfirmationSignature.signedText,
								waitingConfirmationSignature.JsonDigest);
					}
				} else {
#ifdef DEBUG_PRINT
					std::clog << "wrong address size" << std::endl;
#endif
				}
				
			} else {
#ifdef DEBUG_PRINT
				std::clog << "address must be a std::string" << std::endl;
#endif
				}
		}else{
#ifdef DEBUG_PRINT
			std::clog << "signing_path too long" << std::endl;
#endif
		}
	}else{
#ifdef DEBUG_PRINT
		std::clog << "signing_path must be a std::string" << std::endl;
#endif
	}
}


void handleSignatureRequest(const char senderPubkey[45], nlohmann::json& receivedPackage){
	if (typeid(receivedPackage["body"]["unsigned_unit"]) == typeid(nlohmann::json)){

		if (typeid(receivedPackage["body"]["unsigned_unit"]["messages"]) == typeid(nlohmann::json)){
			const char* device_hub = receivedPackage["device_hub"].dump().c_str();
			if (device_hub != nullptr){
				if(strlen(device_hub) < MAX_HUB_STRING_SIZE){
					int arraySize = receivedPackage["body"]["unsigned_unit"]["messages"].size();
					if (arraySize > 0){
						for (int i = 0;i<arraySize;i++){
							if(typeid(receivedPackage["body"]["unsigned_unit"]["messages"][i]) == typeid(nlohmann::json)){
								if(typeid(receivedPackage["body"]["unsigned_unit"]["messages"][i]["payload"]) == typeid(nlohmann::json)){
									const char * payloadHash = receivedPackage["body"]["unsigned_unit"]["messages"][i]["payload_hash"].dump().c_str();
									char hashB64[45];
									getBase64HashForJsonObject (hashB64, receivedPackage["body"]["unsigned_unit"]["messages"][i]["payload"]);
									if (strcmp(hashB64,payloadHash) != 0){
#ifdef DEBUG_PRINT
									std::clog << "payload hash does not match" << std::endl;
									std::clog << hashB64 << std::endl;
									std::clog << payloadHash << std::endl;
#endif
									return;
									}
					

								}else{
#ifdef DEBUG_PRINT
									std::clog << "payload must be an object" << std::endl;
#endif
									return;
								}
							}else{
#ifdef DEBUG_PRINT
								std::clog << "message must be an object" << std::endl;
#endif
								return;
							}

						}
						stripSignAndAddToConfirmationRoom(senderPubkey,device_hub, receivedPackage["body"]);

					} else {
#ifdef DEBUG_PRINT
						std::clog << "arraySize must be >0" << std::endl;
#endif
					}
				}
			}

		} else {
#ifdef DEBUG_PRINT
			std::clog << "messages must be an array" << std::endl;
#endif
		}
	} else {
#ifdef DEBUG_PRINT
	std::clog << "unsigned_unit must be object" << std::endl;
#endif
	}

}

