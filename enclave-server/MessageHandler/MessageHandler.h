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

#define HANDLER_MK_ERROR(x)             (0x00000000|(x))

typedef enum _handler_status_t {
    MSG_SUCCESS                     = HANDLER_MK_ERROR(0x00c8),  // 200
    MSG_SGX_FAILED                  = HANDLER_MK_ERROR(0x0190),  // 400
    MSG_TYPE_NOT_MATCH              = HANDLER_MK_ERROR(0x0002),
    MSG_PINCODE_SEND_FAILED         = HANDLER_MK_ERROR(0x0003),
} handler_status_t;

typedef struct SessionData {
    uint32_t msg_type;
    uint8_t cipher_phone[PHONE_SIZE];
    uint8_t cipher_phone_mac[CIPHER_MAC_SIZE];
    uint8_t pin_code[PIN_CODE_SIZE];
    pthread_rwlock_t rwlock;
    SessionData():msg_type(0) {
        pthread_rwlock_init(&rwlock,NULL);
    }
};

class MessageHandler {

public:
    MessageHandler(int port = Settings::rh_port);
    virtual ~MessageHandler();

    int start();
    vector<string> handleMessages(unsigned char *bytes, int len);
    //vector<string> incomingHandler(string v, int type);

protected:
    Enclave *enclave = NULL;

private:
    sgx_status_t initEnclave();
    sgx_status_t getEnclaveStatus();
    uint32_t getExtendedEPID_GID(uint32_t *extended_epid_group_id);

    string handleVerification(sgx_ra_context_t session_id);
    string handleMSG0(sgx_ra_context_t session_id, Messages::MessageMSG0 msg);
    string handleMSG2(sgx_ra_context_t session_id, Messages::MessageMSG2 msg);
    string handleAttestationResult(sgx_ra_context_t session_id, Messages::AttestationMessage msg);
    string handleRegisterMSG(sgx_ra_context_t session_id, Messages::RegisterMessage msg);
    string handlePinCodeBack(sgx_ra_context_t session_id, Messages::PinCodeBackMessage msg);
    string handleSMS(sgx_ra_context_t session_id, Messages::SMSMessage msg);

    sgx_ra_msg3_t* getMSG3();
    string generateMSG0(sgx_ra_context_t session_id);
    string generateMSG1(sgx_ra_context_t session_id);
    void assembleMSG2(Messages::MessageMSG2 msg, sgx_ra_msg2_t **pp_msg2);
    void assembleAttestationMSG(Messages::AttestationMessage msg, ra_samp_response_header_t **pp_att_msg);

    bool getPhoneByUserID(sgx_ra_context_t session_id, uint8_t *userID, uint8_t *p_unsealed_phone);
    bool putSealedPhone(sgx_ra_context_t session_id, uint8_t *userID, uint8_t *p_sealed_phone, uint32_t sealed_phone_len);
    sgx_ra_context_t getAddSession(Messages::AllInOneMessage aio_msg, handler_status_t *p_handler_status, sgx_status_t *p_sgx_status);
    handler_status_t sendSMS(uint8_t *p_phone, int phone_size, string sms);

    void atomic_wr_session(sgx_ra_context_t session_id, Messages::Type type);

    //string createInitMsg(int type, string msg);
    sgx_ec256_fix_data_t local_ec256_fix_data;

    const int busy_retry_time = 10;
    MysqlConnector *mysqlConnector = NULL;

    // web server related
    //NetworkManagerServer *nm = NULL;
    //Server_http *server_http = NULL;
    unordered_map<sgx_ra_context_t, SessionData*> g_session_mapping_um;
    shared_ptr<Server_http> server = NULL;
    int threads = 3;
    boost::asio::io_context ioc{threads};
    //boost::asio::io_service io_service;

};

#endif
