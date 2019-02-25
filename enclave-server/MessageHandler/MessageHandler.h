#ifndef MessageHandler_H
#define MessageHandler_H

#include <string>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <iostream>
#include <iomanip> 
#include <sys/wait.h> 
#include <sys/types.h> 
#include <unordered_map> 
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Enclave.h"
//#include "NetworkManagerServer.h"
#include "Messages.pb.h"
#include "UtilityFunctions.h"
#include "remote_attestation_result.h"
#include "LogBase.h"
#include "Network_def.h"
#include "../GeneralSettings.h"
#include "MysqlConnector.h"
#include "Websocket_http.h"
//#include "Server_http.h"

#define PHONE_SIZE              11
#define CIPHER_MAC_SIZE         16
#define USER_ID_SIZE            16
#define CIPHER_SIZE             1024 
#define PIN_CODE_SIZE           6 

#define SENDMESSAGE_PROGRAM     "sendSMS.jar"

using namespace std;
using namespace util;

typedef enum _handler_status_t {
    MSG_SUCCESS,
    MSG_TYPE_NOT_MATCH,
    MSG_PINCODE_SEND_FAILED,
} handler_status_t;

typedef struct SessionData {
    uint32_t msg_type;
    uint8_t cipher_phone[PHONE_SIZE];
    uint8_t cipher_phone_mac[CIPHER_MAC_SIZE];
    uint8_t pin_code[PIN_CODE_SIZE];
    SessionData():msg_type(0) {}
};

class MessageHandler {

public:
    MessageHandler(int port = Settings::rh_port);
    virtual ~MessageHandler();

    sgx_ra_msg3_t* getMSG3();
    int init();
    void start();
    //vector<string> incomingHandler(string v, int type);

    sgx_status_t initEnclave();
    uint32_t getExtendedEPID_GID(uint32_t *extended_epid_group_id);
    sgx_status_t getEnclaveStatus();

    vector<string> handleMessages(unsigned char *bytes, int len);
    //string createInitMsg(int type, string msg);
    uint32_t my_flag = 0;
    sgx_ec256_fix_data_t local_ec256_fix_data;

protected:
    Enclave *enclave = NULL;

private:
    string handleAttestationResult(sgx_ra_context_t session_id, Messages::AttestationMessage msg);
    string handleMSG0(sgx_ra_context_t session_id, Messages::MessageMSG0 msg);
    string handleMSG2(sgx_ra_context_t session_id, Messages::MessageMSG2 msg);
    string handleVerification(sgx_ra_context_t session_id);
    string handleRegisterMSG(sgx_ra_context_t session_id, Messages::RegisterMessage msg);
    string handlePinCodeBack(sgx_ra_context_t session_id, Messages::PinCodeBackMessage msg);
    string handleSMS(sgx_ra_context_t session_id, Messages::SMSMessage msg);

    string generateMSG0(sgx_ra_context_t session_id);
    string generateMSG1(sgx_ra_context_t session_id);
    void assembleMSG2(Messages::MessageMSG2 msg, sgx_ra_msg2_t **pp_msg2);
    void assembleAttestationMSG(Messages::AttestationMessage msg, ra_samp_response_header_t **pp_att_msg);

    bool getPhoneByUserID(sgx_ra_context_t session_id, uint8_t *userID, uint8_t *p_unsealed_phone);
    bool putSealedPhone(sgx_ra_context_t session_id, uint8_t *userID, uint8_t *p_sealed_phone, uint32_t sealed_phone_len);
    sgx_ra_context_t getAddSession(Messages::AllInOneMessage aio_msg, handler_status_t *p_handler_status, sgx_status_t *p_sgx_status);
    handler_status_t sendSMS(uint8_t *p_phone, int phone_size, string sms);

    const int busy_retry_time = 10;
    MysqlConnector *mysqlConnector = NULL;
    //NetworkManagerServer *nm = NULL;
    //Server_http *server_http = NULL;
    unordered_map<sgx_ra_context_t, SessionData*> g_session_mapping_um;
    shared_ptr<Listener> listener = NULL;
    int threads = 3;
    boost::asio::io_context ioc{threads};
    boost::asio::io_service io_service;

};

#endif
