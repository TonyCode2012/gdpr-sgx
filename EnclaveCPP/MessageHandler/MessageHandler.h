#ifndef MessageHandler_H
#define MessageHandler_H

#include <string>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <iostream>
#include <iomanip> 
#include "Enclave.h"
//#include "NetworkManagerServer.h"
#include "Messages.pb.h"
#include "UtilityFunctions.h"
#include "remote_attestation_result.h"
#include "LogBase.h"
#include "Network_def.h"
#include "GeneralSettings.h"
#include "MysqlConnector.h"

#define PHONE_SIZE              11
#define CIPHER_MAC_SIZE         16
#define USER_ID_SIZE            16
#define CIPHER_SIZE             1024 

using namespace std;
using namespace util;

class MessageHandler {

public:
    MessageHandler(int port = Settings::rh_port);
    virtual ~MessageHandler();

    sgx_ra_msg3_t* getMSG3();
    //int init();
    //void start();
    //vector<string> incomingHandler(string v, int type);
    static void printName() {
        printf("========== MessageHandler ==========\n");
    }

    sgx_status_t initEnclave();
    uint32_t getExtendedEPID_GID(uint32_t *extended_epid_group_id);
    sgx_status_t getEnclaveStatus();

    void assembleAttestationMSG(Messages::AttestationMessage msg, ra_samp_response_header_t **pp_att_msg);
    string handleAttestationResult(Messages::AttestationMessage msg);
    string handleRegisterMSG(Messages::RegisterMessage msg);
    string handleSMS(Messages::SMSMessage msg, unsigned char *p_data, int *p_size);
    string handleMessages(unsigned char *bytes, int len, unsigned char *p_data, int *p_size);
    void assembleMSG2(Messages::MessageMSG2 msg, sgx_ra_msg2_t **pp_msg2);
    string handleMSG2(Messages::MessageMSG2 msg);
    string handleMSG0(Messages::MessageMSG0 msg);
    string generateMSG1();
    string handleVerification();
    string generateMSG0();
    bool getPhoneByUserID(uint8_t *userID, uint8_t *p_unsealed_phone);
    bool putSealedPhone(uint8_t *userID, uint8_t *p_sealed_phone, uint32_t sealed_phone_len);
    //string createInitMsg(int type, string msg);
    uint32_t my_flag = 0;
    sgx_enclave_id_t local_enclave_id;
    sgx_ra_context_t local_enclave_context;
    sgx_status_t local_enclave_status;
    sgx_ec256_fix_data_t local_ec256_fix_data;

protected:
    Enclave *enclave = NULL;

private:
    const int busy_retry_time = 4;
    MysqlConnector *mysqlConnector = NULL;
    //NetworkManagerServer *nm = NULL;

};

#endif
