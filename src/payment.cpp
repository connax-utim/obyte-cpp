// Byteduino lib - papabyte.com
// MIT License

#include "payment.hpp"

extern net::io_context webSocketForHub;
extern Byteduino byteduino_device;

bufferPaymentStructure bufferPayment;

cbPaymentResult _cbPaymentResult;
cbBalancesReceived _cbBalancesReceived;

void setCbPaymentResult(cbPaymentResult cbToSet){
	_cbPaymentResult = cbToSet;
}

void setCbBalancesReceived(cbBalancesReceived cbToSet){
	_cbBalancesReceived = cbToSet;
}

int loadBufferPayment(const int amount, const bool hasDataFeed, nlohmann::json dataFeed, const char * recipientAddress, const int id){
	if(!bufferPayment.isFree){
#ifdef DEBUG_PRINT
		std::clog << "Buffer not free to send payment" << std::endl;
#endif
		return BUFFER_NOT_FREE;
	} 
	if (!isValidChash160(recipientAddress)){
#ifdef DEBUG_PRINT
		std::clog << "Recipient address is not valid" << std::endl;
#endif
		return ADDRESS_NOT_VALID;
	}
	if (MAX_DATA_FEED_JSON_SIZE < dataFeed.dump().size()){
#ifdef DEBUG_PRINT
		std::clog << "data feed size exceeds maximum allowed" << std::endl;
#endif
		return DATA_FEED_TOO_LONG;
	}

	bufferPayment.isFree = false;
	bufferPayment.id = id;
	bufferPayment.amount = amount;
	memcpy(bufferPayment.recipientAddress, recipientAddress,33);
	bufferPayment.arePropsReceived = false;
	bufferPayment.isDefinitionReceived = false;
	bufferPayment.requireDefinition = false;
	bufferPayment.areInputsRequested = false;
	strcpy(bufferPayment.unit,"");
	bufferPayment.isPosted = false;
	strcpy(bufferPayment.dataFeedJson, dataFeed.dump().c_str());
	bufferPayment.hasDataFeed = hasDataFeed;
	bufferPayment.timeOut = SEND_PAYMENT_TIME_OUT;
	requestDefinition(byteduino_device.fundingAddress);
	getParentsAndLastBallAndWitnesses();
	return SUCCESS;
}

int sendPayment(const int amount, const char * recipientAddress, const int payment_id){
	nlohmann::json objDataFeed; // dummy object
	return loadBufferPayment(amount, false, objDataFeed, recipientAddress, payment_id);
}

int postDataFeed(const char * key, const char * value, const int id){
	nlohmann::json objDataFeed;
	objDataFeed[key] = value;
	return loadBufferPayment(0, true, objDataFeed, "", id);
}


void treatPaymentComposition(){
	if (!bufferPayment.isFree &&
		bufferPayment.isDefinitionReceived &&
		bufferPayment.arePropsReceived &&
			!bufferPayment.areInputsRequested){
		requestInputsForAmount(bufferPayment.amount, byteduino_device.fundingAddress);
		bufferPayment.areInputsRequested = true;
	}
}

void managePaymentCompositionTimeOut(){
	if (bufferPayment.timeOut > 0){
		bufferPayment.timeOut--;
	}

	if (bufferPayment.timeOut == 0 && !bufferPayment.isFree){
		if (bufferPayment.isPosted){
			if(_cbPaymentResult){
				_cbPaymentResult(bufferPayment.id, TIMEOUT_PAYMENT_NOT_ACKNOWLEDGED, bufferPayment.unit);
			}
		} else {
			if(_cbPaymentResult){
				_cbPaymentResult(bufferPayment.id, TIMEOUT_PAYMENT_NOT_SENT,"");
			}
		}
		bufferPayment.isFree = true;
#ifdef DEBUG_PRINT
		std::clog << "Payment was not sent" << std::endl;
#endif 
	}
}

void requestDefinition(const char* address){

	char output[130];
	char tag[12];
	getTag(tag, GET_ADDRESS_DEFINITION);
	nlohmann::json mainArray = {
		"request", {
			{"command", "light/get_definition"},
			{"params", address},
			{"tag", (const char*) tag}
		}
	};
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << std::setw(4) << mainArray << std::endl;
#endif
	sendTXT(output);
}

void handlePostJointResponse(nlohmann::json receivedObject, const char * tag){
	std::string stringTag = tag;
	std::string strId = stringTag.substr(10);
	int id = stoi(strId);
	const char * response = receivedObject["response"].dump().c_str();
	if (response != nullptr){
		if (strcmp(response, "accepted") == 0){
			if(_cbPaymentResult){
				_cbPaymentResult(id, PAYMENT_ACKNOWLEDGED, bufferPayment.unit);
			}
			bufferPayment.isFree = true;
		} else {
			if (typeid(receivedObject["error"]) == typeid(nlohmann::json)){
				if(_cbPaymentResult){
					_cbPaymentResult(id, PAYMENT_REFUSED, bufferPayment.unit);
				}
				bufferPayment.isFree = true;
			}
		}
	} 
}


void handleDefinition(const nlohmann::json &receivedObject) {

	if (typeid(receivedObject["response"]) == typeid(nlohmann::json)){
		bufferPayment.requireDefinition = false;
	} else {
		bufferPayment.requireDefinition = true;
	}

	bufferPayment.isDefinitionReceived = true;
}


void requestInputsForAmount(int amount, const char * address){
#ifdef DEBUG_PRINT
	std::clog << "requestInputsForAmount" << std::endl;
#endif
	char tag[12];
	getTag(tag, GET_INPUTS_FOR_AMOUNT);
	std::string idString = std::to_string(bufferPayment.id);
	std::string tagWithId = tag + idString;
	std::clog << tagWithId << std::endl;

	nlohmann::json mainArray = {
		"request", {
			{"command", "light/pick_divisible_coins_for_amount"},
			{"params", {
				{"last_ball_mci", 1000000000},
				{"amount", amount + 1000 + strlen(bufferPayment.dataFeedJson)},
				{"addresses", {address}}
			}},
			{"tag", tagWithId}
		}
	};

	char output[mainArray.dump().size() + 1];
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << output << std::endl;
#endif
	sendTXT(output);

}

void handleInputsForAmount(nlohmann::json receivedObject, const char * tag) {
	std::string stringTag = tag;
//	String strId = std::stringTag.substring(10);
	int id = std::stoi(stringTag.substr(0,10));
	if (typeid(receivedObject["response"]) == typeid(nlohmann::json)){
		if (typeid(receivedObject["response"]["inputs_with_proofs"]) == typeid(nlohmann::json)){
			if (typeid(receivedObject["response"]["total_amount"]) == typeid(int)){
				if (receivedObject["response"]["total_amount"] > 0){
					if (!bufferPayment.isFree && id == bufferPayment.id)
						composeAndSendUnit(receivedObject["response"]["inputs_with_proofs"], receivedObject["response"]["total_amount"]);
				}else{
					if(_cbPaymentResult){
						_cbPaymentResult(id, NOT_ENOUGH_FUNDS, "");
					}
					bufferPayment.isFree = true;
				}

			} else {
#ifdef DEBUG_PRINT
				std::clog << "total_amount must be an int" << std::endl;
#endif
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "inputs_with_proofs should be an array" << std::endl;
#endif
		}
	} else {
	#ifdef DEBUG_PRINT
		std::clog << "response should be an object" << std::endl;
	#endif
	}

}

void composeAndSendUnit(nlohmann::json arrInputs, int total_amount){
	
	int headers_commission = 0;
	int payload_commission = 0;
	nlohmann::json mainArray, objParams, unit;

	if (byteduino_device.isTestNet){
		unit["version"] = TEST_NET_VERSION;
		unit["alt"] = TEST_NET_ALT;
		headers_commission += 5;
	} else {
		unit["version"] = MAIN_NET_VERSION;
		unit["alt"] = MAIN_NET_ALT;
		headers_commission += 4;
	}

	unit["witness_list_unit"] = (const char *) bufferPayment.witness_list_unit;
	unit["last_ball_unit"] = (const char *) bufferPayment.last_ball_unit;
	unit["last_ball"] = (const char *) bufferPayment.last_ball;
	headers_commission += 132;

	nlohmann::json parent_units;
	parent_units.push_back((const char * ) bufferPayment.parent_units[0]);
	headers_commission+=88; // PARENT_UNITS_SIZE
	if (strcmp(bufferPayment.parent_units[1],"") != 0){
		parent_units.push_back((const char * ) bufferPayment.parent_units[1]);
	}

	headers_commission += 32 + 88;// for authors
	if (bufferPayment.requireDefinition)
		headers_commission += 44 + 3;// for definition

	nlohmann::json datafeedPayload = nlohmann::json::parse(bufferPayment.dataFeedJson);
	char datafeedPayloadHash[45];

	if (bufferPayment.hasDataFeed){
		payload_commission += 9 + 6 + 44;//data_feed + inline + payload hash
		for (nlohmann::json p : datafeedPayload) {
			payload_commission += p.dump().size();
		}

		getBase64HashForJsonObject (datafeedPayloadHash, datafeedPayload);
	}

	nlohmann::json paymentPayload;
	payload_commission += 7 + 6 + 44;//payment + inline + payload hash

	nlohmann::json inputs;

	for (nlohmann::json elem : arrInputs) {
		nlohmann::json input = elem["input"];
		inputs.push_back(input);
		payload_commission+= 44 + 8 + 8; //unit + message index + output index
	}
	paymentPayload["inputs"] = inputs;

	nlohmann::json outputs;
	nlohmann::json firstOutput;

	if (bufferPayment.amount > 0){
		firstOutput["address"] = (const char *) bufferPayment.recipientAddress;
		firstOutput["amount"] = (const int) bufferPayment.amount;
		payload_commission+= 32 + 8; // address + amount
	}
	nlohmann::json changeOutput;
	changeOutput["address"] = (const char *) byteduino_device.fundingAddress;
	payload_commission+= 32 + 8; // address + amount
	changeOutput["amount"] = total_amount - bufferPayment.amount - headers_commission - payload_commission;
	outputs.push_back(changeOutput);

	if (bufferPayment.amount>0)
		outputs.push_back(firstOutput);

	char paymentPayloadHash[45];
	getBase64HashForJsonObject (paymentPayloadHash, paymentPayload);

	unit["parent_units"] = parent_units;
	nlohmann::json messages, payment;

	payment["app"] = "payment";
	payment["payload_location"] = "inline";
	payment["payload_hash"] = (const char *) paymentPayloadHash;
	messages.push_back(payment);

	if (bufferPayment.hasDataFeed){
		nlohmann::json datafeed;
		datafeed["app"] = "data_feed";
		datafeed["payload_location"] = "inline";
		datafeed["payload_hash"] = (const char *) datafeedPayloadHash;
		messages.push_back(datafeed);
	}

	unit["messages"] = messages;
	nlohmann::json firstAuthor;
    nlohmann::json authors; // = unit.createNestedArray("authors");
	firstAuthor["address"] = (const char *) byteduino_device.fundingAddress;

	nlohmann::json definition;
	if (bufferPayment.requireDefinition){
		nlohmann::json objSig;
		objSig["pubkey"] = (const char *) byteduino_device.keys.publicKeyM4400b64;
		definition.push_back("sig");
		definition.push_back(objSig);
		firstAuthor["definition"] = definition;
	}

	authors.push_back(firstAuthor);
    unit["authors"] = authors;

	uint8_t hash[32];
	getSHA256ForJsonObject(hash, unit);
	char sigb64 [89];
	getB64SignatureForHash(sigb64, byteduino_device.keys.privateM4400, hash, 32);

	nlohmann::json authentifier;
	authentifier["r"] = (const char *) sigb64;
	firstAuthor["authentifiers"] = authentifier;

	char content_hash[45];
	getBase64HashForJsonObject (content_hash, unit);

	unit["content_hash"] = (const char *) content_hash;

	firstAuthor.erase("authentifiers");
	firstAuthor.erase("definition");
	unit.erase("messages");
	char unit_hash[45];
	getBase64HashForJsonObject (unit_hash, unit);

	unit["unit"] = (const char *) unit_hash;
	strcpy(bufferPayment.unit, unit_hash);
	unit.erase("content_hash");
	unit["messages"] = messages;

	firstAuthor["authentifiers"] = authentifier;
	if (bufferPayment.requireDefinition){
		firstAuthor["definition"] = definition;
	}
	unit["messages"][0]["payload"] = paymentPayload;

	unit["messages"][1]["payload"] = datafeedPayload;
	mainArray.push_back("request");
	nlohmann::json objRequest;

	unit["payload_commission"] = payload_commission;
	unit["headers_commission"] = headers_commission;

	objRequest["command"] = "post_joint";
	objRequest["params"] = objParams;
	char tag[12];
	getTag(tag, POST_JOINT);
	std::string idString = std::to_string(bufferPayment.id);
	std::string tagWithId = tag + idString;;
	objRequest["tag"] = tagWithId;
	
	mainArray.push_back(objRequest);
	char output[mainArray.dump().size() + 1];
	strcpy(output, mainArray.dump().c_str());
	std::clog << output;
	bufferPayment.isPosted = true;
	sendTXT(output);

}


void getParentsAndLastBallAndWitnesses(){

	char output[200];
	char tag[12];
	getTag(tag, GET_PARENTS_BALL_WITNESSES);
	nlohmann::json mainArray = {
		"request", {
			{"command", "light/get_parents_and_last_ball_and_witness_list_unit"},
			{"params", {}},
			{"tag", (const char*) tag}
		}
	};

	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << output << std::endl;
#endif
	sendTXT(output);

}


void handleUnitProps(nlohmann::json receivedObject){
	if (typeid(receivedObject["response"]) == typeid(nlohmann::json)){
		nlohmann::json response = receivedObject["response"];

		if (typeid(response["parent_units"]) == typeid(nlohmann::json)){
			const char* parent_units_0 = response["parent_units"][0].dump().c_str();
			if (parent_units_0 != nullptr) {
				if (strlen(parent_units_0) == 44){
					memcpy(bufferPayment.parent_units[0], parent_units_0,45);
					std::clog << bufferPayment.parent_units[0];
				} else {
				
#ifdef DEBUG_PRINT
					std::clog << "parent unit should be 44 chars long " << std::endl;
#endif
					return;
				}
			}
			const char* parent_units_1 = response["parent_units"][1].dump().c_str();
			if (parent_units_1 != nullptr) {
				if (strlen(parent_units_1) == 44){
					memcpy(bufferPayment.parent_units[1], parent_units_1,45);
				} else {
#ifdef DEBUG_PRINT
					std::clog << "parent unit should be 44 chars long " << std::endl;
#endif
					return;
				}
			} else{
					strcpy(bufferPayment.parent_units[1], "");
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "parent_units should be an array" << std::endl;
#endif
			return;
		}

		const char* last_stable_mc_ball = response["last_stable_mc_ball"].dump().c_str();
		if (last_stable_mc_ball != nullptr) {
			if (strlen(last_stable_mc_ball) == 44){
				memcpy(bufferPayment.last_ball, last_stable_mc_ball,45);
			} else {
#ifdef DEBUG_PRINT
				std::clog << "last_stable_mc_ball  should be 44 chars long " << std::endl;
#endif
				return;
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "last_stable_mc_ball must be a char" << std::endl;
#endif
			return;
		}

		const char* last_stable_mc_ball_unit = response["last_stable_mc_ball_unit"].dump().c_str();
		if (last_stable_mc_ball_unit != nullptr) {
			if (strlen(last_stable_mc_ball_unit) == 44){
				memcpy(bufferPayment.last_ball_unit, last_stable_mc_ball_unit,45);
			} else {
#ifdef DEBUG_PRINT
				std::clog << "last_stable_mc_ball_unit  should be 44 chars long " << std::endl;
#endif
				return;
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "last_stable_mc_ball_unit must be a char" << std::endl;
#endif
			return;
		}
		const char* witness_list_unit = response["witness_list_unit"].dump().c_str();
		if (witness_list_unit != nullptr) {
			if (strlen(witness_list_unit) == 44){
				memcpy(bufferPayment.witness_list_unit, witness_list_unit,45);
			} else {
				
#ifdef DEBUG_PRINT
				std::clog << "witness_list_unit  should be 44 chars long " << std::endl;
#endif
				return;
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "witness_list_unit must be a char" << std::endl;
#endif
			return;
		}
	}else{
#ifdef DEBUG_PRINT
		std::clog << "response must be an object" << std::endl;
#endif
		return;
	}
	bufferPayment.arePropsReceived = true;
}

void getPaymentAddressFromPubKey(const char * pubKey, char * paymentAddress) {
	nlohmann::json mainArray = {
			"sig",
			{"pubkey", pubKey}
	};
	getChash160ForArray (mainArray, paymentAddress);
}

void getAvailableBalances(){
	char tag[12];
	getTag(tag, GET_BALANCE);
	nlohmann::json mainArray = {
		"request",
		{
			{"command", "light/get_balances"},
			{"params", {(const char *) byteduino_device.fundingAddress}},
			{"tag", (const char*) tag}
		}
	};

	char output[mainArray.dump().size()];
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << output << std::endl;
#endif
	sendTXT(output);
}

void handleBalanceResponse(nlohmann::json receivedObject){
	if (typeid(receivedObject["response"]) == typeid(nlohmann::json)){
		if (typeid(receivedObject["response"][byteduino_device.fundingAddress]) == typeid(nlohmann::json)){
			if(_cbBalancesReceived){
				_cbBalancesReceived(receivedObject["response"][byteduino_device.fundingAddress]);
			} else {
#ifdef DEBUG_PRINT
				std::clog << "no get balance callback set" << std::endl;
#endif
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "no balance for my payment address" << std::endl;
#endif
		}

	} else {
#ifdef DEBUG_PRINT
		std::clog << "response must be a object" << std::endl;
#endif
	}

}
