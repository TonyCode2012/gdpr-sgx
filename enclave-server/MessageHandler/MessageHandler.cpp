#include "MessageHandler.h"
#include "sgx_tseal.h"
#include <inttypes.h>

using namespace util;

MessageHandler::MessageHandler(int port) {
    //this->nm = NetworkManagerServer::getInstance(port);
    this->local_ec256_fix_data.g_key_flag = 1;
    mysqlConnector = new MysqlConnector();
    mysqlConnector->initDB("localhost", "root", "fishbowl", "SMS");

    //this->server_http = new Server_http(io_service);
    auto const address = boost::asio::ip::make_address("127.0.0.1");
    this->server = std::make_shared<Server_http>(ioc, tcp::endpoint({address,port}));
}

MessageHandler::~MessageHandler() {
    delete this->enclave;
    delete this->mysqlConnector;
}


int MessageHandler::start() {
    // initial enclave
    sgx_status_t ret = this->initEnclave();
    if (SGX_SUCCESS != ret) {
        Log("Error, call enclave_init_ra fail", log::error);
    } else {
        Log("Call enclave_init_ra success");
    }

    // set webserver related
    this->server->connectCallbackHandler([this](unsigned char *bytes, int len) {
        return this->handleMessages(bytes, len);
    });
    // Create and launch a listening port
    this->server->run();

    // Run the I/O service on the requested number of threads
    boost::asio::io_context *pioc = &this->ioc;
    vector<thread> v;
    v.reserve(threads-1);
    for(int i=threads-1;i>0;--i) {
        v.emplace_back(
                [pioc]{
                    pioc->run();
                });
    }
    pioc->run();
}


sgx_status_t MessageHandler::initEnclave() {
    this->enclave = Enclave::getInstance();
    sgx_status_t ret = this->enclave->createEnclave();
    return ret;
}


sgx_status_t MessageHandler::getEnclaveStatus() {
    return this->enclave->getStatus();
}


uint32_t MessageHandler::getExtendedEPID_GID(uint32_t *extended_epid_group_id) {
    int ret = sgx_get_extended_epid_group_id(extended_epid_group_id);
    ret = 0;

    if (SGX_SUCCESS != ret) {
        Log("Error, call sgx_get_extended_epid_group_id fail: %lx", ret);
        print_error_message((sgx_status_t)ret);
        return ret;
    } else
        Log("Call sgx_get_extended_epid_group_id success");

    return ret;
}


string MessageHandler::generateMSG0(sgx_ra_context_t session_id) {
    Log("Call MSG0 generate");

    uint32_t extended_epid_group_id;
    int ret = this->getExtendedEPID_GID(&extended_epid_group_id);

    Messages::MessageMSG0 *msg = new Messages::MessageMSG0();
    msg->set_type(Messages::Type::RA_MSG0);
    msg->set_status(MSG_SUCCESS);

    if (ret == SGX_SUCCESS) {
        msg->set_epid(extended_epid_group_id);
    } else {
        msg->set_status(TYPE_TERMINATE);
        msg->set_epid(0);
    }

    string s;
    Messages::AllInOneMessage aio_ret_msg;
    aio_ret_msg.set_type(Messages::Type::RA_MSG0);
    aio_ret_msg.set_allocated_msg0(msg);
    aio_ret_msg.set_sessionid(session_id);
    if(aio_ret_msg.SerializeToString(&s)) {
        Log("Serialization successful");
        atomic_wr_session(session_id,Messages::Type::RA_MSG0);
    }
    else {
        Log("Serialization failed", log::error);
        s = "";
    }

    return s;
}


string MessageHandler::generateMSG1(sgx_ra_context_t session_id) {
    int retGIDStatus = 0;
    int count = 0;
    sgx_ra_msg1_t sgxMsg1Obj;
    Log("========== SEALED ENCLAVE PUB KEY ==========");
    local_ec256_fix_data.g_key_flag = 0;

    while (1) {
        retGIDStatus = sgx_ra_get_msg1(session_id,
                                       this->enclave->getID(),
                                       sgx_ra_get_ga,
                                       &sgxMsg1Obj,
                                       &local_ec256_fix_data);
        
        Log("\t gax:%s",ByteArrayToString(sgxMsg1Obj.g_a.gx, 32));
        Log("\t gay:%s",ByteArrayToString(sgxMsg1Obj.g_a.gy, 32));

        if (retGIDStatus == SGX_SUCCESS) {
            break;
        } else if (retGIDStatus == SGX_ERROR_BUSY) {
            if (count == 5) { //retried 5 times, so fail out
                Log("Error, sgx_ra_get_msg1 is busy - 5 retries failed", log::error);
                break;;
            } else {
                sleep(3);
                count++;
            }
        } else {    //error other than busy
            Log("Error, failed to generate MSG1,error code:%lx", retGIDStatus, log::error);
            break;
        }
    }


    Messages::MessageMSG1 *msg = new Messages::MessageMSG1();
    msg->set_type(Messages::Type::RA_MSG1);
    msg->set_status(MSG_SUCCESS);
    if (SGX_SUCCESS == retGIDStatus) {
        for (auto x : sgxMsg1Obj.g_a.gx)
            msg->add_gax(x);

        for (auto x : sgxMsg1Obj.g_a.gy)
            msg->add_gay(x);

        for (auto x : sgxMsg1Obj.gid) {
            msg->add_gid(x);
        }
    } else {
        msg->set_status(MSG_SGX_FAILED);
    }

    string s;
    Messages::AllInOneMessage aio_ret_msg;
    aio_ret_msg.set_type(Messages::Type::RA_MSG1);
    aio_ret_msg.set_allocated_msg1(msg);
    aio_ret_msg.set_sessionid(session_id);
    if(aio_ret_msg.SerializeToString(&s)) {
        Log("Serialization successful");
        atomic_wr_session(session_id,Messages::Type::RA_MSG2);
    }
    else {
        Log("Serialization failed", log::error);
        s = "";
    }

    return s;
}


void MessageHandler::assembleMSG2(Messages::MessageMSG2 msg, sgx_ra_msg2_t **pp_msg2) {
    Log("=============== ASSEMBLE MSG2 ===============");
    uint32_t size = msg.size();

    sgx_ra_msg2_t *p_msg2 = NULL;
    p_msg2 = (sgx_ra_msg2_t*) malloc(size + sizeof(sgx_ra_msg2_t));

    uint8_t pub_key_gx[32];
    uint8_t pub_key_gy[32];

    sgx_ec256_signature_t sign_gb_ga;
    sgx_spid_t spid;

    for (int i; i<32; i++) {
        pub_key_gx[i] = msg.publickeygx(i);
        pub_key_gy[i] = msg.publickeygy(i);
    }
    /*
    Log("\tsp public key:");
    for(int i=0;i<32;i++) {
        printf("%u,",pub_key_gx[i]);
    }
    printf("\n");
    for(int i=0;i<32;i++) {
        printf("%u,",pub_key_gy[i]);
    }
    printf("\n");
    */
    Log("\tpub key gx:%s",ByteArrayToString(pub_key_gx,32));
    Log("\tpub key gy:%s",ByteArrayToString(pub_key_gy,32));

    for (int i=0; i<16; i++) {
        spid.id[i] = msg.spid(i);
    }
    //Log("\tspid:%s",ByteArrayToString(spid.id,16));

    for (int i=0; i<8; i++) {
        sign_gb_ga.x[i] = msg.signaturex(i);
        sign_gb_ga.y[i] = msg.signaturey(i);
    }
    for(int i=0;i<8;i++){
        printf("%" PRIu32 ",",sign_gb_ga.x[i]);
    }
    printf("\n");
    for(int i=0;i<8;i++){
        printf("%" PRIu32 ",",sign_gb_ga.y[i]);
    }
    printf("\n");

    memcpy(&p_msg2->g_b.gx, &pub_key_gx, sizeof(pub_key_gx));
    memcpy(&p_msg2->g_b.gy, &pub_key_gy, sizeof(pub_key_gy));
    memcpy(&p_msg2->sign_gb_ga, &sign_gb_ga, sizeof(sign_gb_ga));
    memcpy(&p_msg2->spid, &spid, sizeof(spid));

    p_msg2->quote_type = (uint16_t)msg.quotetype();
    p_msg2->kdf_id = msg.cmackdfid();
    //Log("\tquote type:%d",p_msg2->quote_type);
    //Log("\tkdf id    :%d",p_msg2->kdf_id);

    uint8_t smac[16];
    for (int i=0; i<16; i++)
        smac[i] = msg.smac(i);
    //Log("\tsmac:%s",ByteArrayToString(smac,16));

    memcpy(&p_msg2->mac, &smac, sizeof(smac));

    p_msg2->sig_rl_size = msg.sizesigrl();
    //uint8_t *sigrl = (uint8_t*) malloc(sizeof(uint8_t) * msg.sizesigrl());
    uint8_t *sigrl = new uint8_t[msg.sizesigrl()];
    //Log("\tsig rl size:%d",p_msg2->sig_rl_size);

    for (int i=0; i<msg.sizesigrl(); i++)
        sigrl[i] = msg.sigrl(i);
    //Log("\tsigrl:%s",ByteArrayToString(sigrl, p_msg2->sig_rl_size));

    memcpy(&p_msg2->sig_rl, &sigrl, msg.sizesigrl());

    *pp_msg2 = p_msg2;
}


string MessageHandler::handleMSG2(sgx_ra_context_t session_id, Messages::MessageMSG2 msg) {
    Log("Received MSG2");

    uint32_t size = msg.size();

    sgx_ra_msg2_t *p_msg2;
    this->assembleMSG2(msg, &p_msg2);

    sgx_ra_msg3_t *p_msg3 = NULL;
    uint32_t msg3_size;
    int ret = 0;

    Log("========== sgx ra proc msg2 para ==========");

    int timeout = 0;
    do {
        ret = sgx_ra_proc_msg2(session_id,
                               this->enclave->getID(),
                               sgx_ra_proc_msg2_trusted,
                               sgx_ra_get_msg3_trusted,
                               p_msg2,
                               size,
                               &p_msg3,
                               &msg3_size);
    } while (SGX_SUCCESS != ret && ++timeout < busy_retry_time);
    //} while (SGX_ERROR_BUSY == ret && ++timeout < busy_retry_time);
    //SafeFree(p_msg2);

    Messages::MessageMSG3 *msg3 = new Messages::MessageMSG3();
    msg3->set_type(Messages::Type::RA_MSG3);
    msg3->set_size(msg3_size);
    msg3->set_status(MSG_SUCCESS);

    if (SGX_SUCCESS != (sgx_status_t)ret) {
        Log("Error, call sgx_ra_proc_msg2 fail, error code: %lx", ret);
        msg3->set_status(MSG_SGX_FAILED);
    } else {
        Log("Call sgx_ra_proc_msg2 success");
        for (int i=0; i<SGX_MAC_SIZE; i++)
            msg3->add_sgxmac(p_msg3->mac[i]);

        for (int i=0; i<SGX_ECP256_KEY_SIZE; i++) {
            msg3->add_gaxmsg3(p_msg3->g_a.gx[i]);
            msg3->add_gaymsg3(p_msg3->g_a.gy[i]);
        }

        for (int i=0; i<256; i++) {
            msg3->add_secproperty(p_msg3->ps_sec_prop.sgx_ps_sec_prop_desc[i]);
        }

        for (int i=0; i<1116; i++) {
            msg3->add_quote(p_msg3->quote[i]);
        }

        //SafeFree(p_msg3);

    }

    string s;
    Messages::AllInOneMessage aio_ret_msg;
    aio_ret_msg.set_type(Messages::Type::RA_MSG3);
    aio_ret_msg.set_allocated_msg3(msg3);
    aio_ret_msg.set_sessionid(session_id);
    if(aio_ret_msg.SerializeToString(&s)) {
        Log("Serialization successful");
        atomic_wr_session(session_id,Messages::Type::RA_ATT_RESULT);
    }
    else {
        Log("Serialization failed", log::error);
        s = "";
    }

    //SafeFree(p_msg3);

    return s;
}


void MessageHandler::assembleAttestationMSG(Messages::AttestationMessage msg, ra_samp_response_header_t **pp_att_msg) {
    //Log("\t========= assemble attestation msg 1");
    sample_ra_att_result_msg_t *p_att_result_msg = NULL;
    ra_samp_response_header_t* p_att_result_msg_full = NULL;
    int msg_size = msg.size();    
    //Log("Att msg size %d", msg_size);

    int msg_resultsize = msg.resultsize();
    //Log("Att msg result size %d", msg_resultsize);

    int total_size = msg.size() + sizeof(ra_samp_response_header_t) + msg.resultsize();
    //Log("Att result total size %d", total_size);

    p_att_result_msg_full = (ra_samp_response_header_t*) malloc(total_size);

    memset(p_att_result_msg_full, 0, total_size);

    p_att_result_msg_full->type = Messages::Type::RA_ATT_RESULT;
    p_att_result_msg_full->size = msg.size();
    //Log("Att result type: %d", p_att_result_msg_full->type);
    //Log("Att platform_info_blob_t size: %d", sizeof(ias_platform_info_blob_t));

    p_att_result_msg = (sample_ra_att_result_msg_t *) p_att_result_msg_full->body;

    p_att_result_msg->platform_info_blob.sample_epid_group_status = msg.epidgroupstatus();

    p_att_result_msg->platform_info_blob.sample_tcb_evaluation_status = msg.tcbevaluationstatus();
    p_att_result_msg->platform_info_blob.pse_evaluation_status = msg.pseevaluationstatus();

    for (int i=0; i<PSVN_SIZE; i++)
        p_att_result_msg->platform_info_blob.latest_equivalent_tcb_psvn[i] = msg.latestequivalenttcbpsvn(i);

    for (int i=0; i<ISVSVN_SIZE; i++)
        p_att_result_msg->platform_info_blob.latest_pse_isvsvn[i] = msg.latestpseisvsvn(i);

    for (int i=0; i<PSDA_SVN_SIZE; i++)
        p_att_result_msg->platform_info_blob.latest_psda_svn[i] = msg.latestpsdasvn(i);

    for (int i=0; i<GID_SIZE; i++)
        p_att_result_msg->platform_info_blob.performance_rekey_gid[i] = msg.performancerekeygid(i);

    for (int i=0; i<SAMPLE_NISTP256_KEY_SIZE; i++) {
        p_att_result_msg->platform_info_blob.signature.x[i] = msg.ecsign256x(i);
        p_att_result_msg->platform_info_blob.signature.y[i] = msg.ecsign256y(i);
    }

    for (int i=0; i<SAMPLE_MAC_SIZE; i++)
        p_att_result_msg->mac[i] = msg.macsmk(i);

    p_att_result_msg->secret.payload_size = msg.resultsize();
    //Log("Att result payload_size: %d", p_att_result_msg->secret.payload_size);

    for (int i=0; i<12; i++)
        p_att_result_msg->secret.reserved[i] = msg.reserved(i);
    //Log("Att result reserved");

    for (int i=0; i<SAMPLE_SP_TAG_SIZE; i++) {
        p_att_result_msg->secret.payload_tag[i] = msg.payloadtag(i);
    }

    for (int i=0; i<msg.resultsize(); i++) {
        p_att_result_msg->secret.payload[i] = (uint8_t)msg.payload(i);
    }

    *pp_att_msg = p_att_result_msg_full;
}


string MessageHandler::handleAttestationResult(sgx_ra_context_t session_id, Messages::AttestationMessage msg) {
    Log("Received Attestation result, start to sssemble");

    ra_samp_response_header_t *p_att_result_msg_full = NULL;
    this->assembleAttestationMSG(msg, &p_att_result_msg_full);
    Log("Assemble Success");

    sample_ra_att_result_msg_t *p_att_result_msg_body = (sample_ra_att_result_msg_t *) ((uint8_t*) p_att_result_msg_full + sizeof(ra_samp_response_header_t));

    sgx_status_t status;
    sgx_status_t ret;
    ret = verify_att_result_mac(this->enclave->getID(),
                                &status,
                                session_id,
                                (uint8_t*)&p_att_result_msg_body->platform_info_blob,
                                sizeof(ias_platform_info_blob_t),
                                (uint8_t*)&p_att_result_msg_body->mac,
                                sizeof(sgx_mac_t));


    Messages::InitialMessage *attMsg = new Messages::InitialMessage();
    attMsg->set_type(Messages::Type::RA_APP_ATT_OK);
    attMsg->set_size(0);
    attMsg->set_status(MSG_SUCCESS);

    if ((SGX_SUCCESS != ret) || (SGX_SUCCESS != status)) {
        Log("Error: INTEGRITY FAILED - attestation result message MK based cmac failed. ret:%lx",ret);
        Log("Error: INTEGRITY FAILED - attestation result message MK based cmac failed. status:%lx",status);
        attMsg->set_status(MSG_SGX_FAILED);
        goto cleanup;
    }

    if (0 != p_att_result_msg_full->status[0] || 0 != p_att_result_msg_full->status[1]) {
        Log("Error, attestation mac result message MK based cmac failed", log::error);
        attMsg->set_status(MSG_SGX_FAILED);
    } else {
        ret = verify_secret_data(this->enclave->getID(),
                                 &status,
                                 session_id,
                                 p_att_result_msg_body->secret.payload,
                                 p_att_result_msg_body->secret.payload_size,
                                 p_att_result_msg_body->secret.payload_tag,
                                 MAX_VERIFICATION_RESULT,
                                 NULL);

        /*
        Log("========== input mac ==========");
        for(int i=0;i<MAX_VERIFICATION_RESULT;i++) {
            printf("%u,",p_att_result_msg_body->secret.payload_tag[i]);
        }
        printf("\n");
        */
        //SafeFree(p_att_result_msg_full);

        if (SGX_SUCCESS != ret) {
            Log("Error, attestation result message secret using SK based AESGCM failed1 %d", ret, log::error);
            print_error_message(ret);
            attMsg->set_status(MSG_SGX_FAILED);
        } 
        else if (SGX_SUCCESS != status) {
            Log("Error, attestation result message secret using SK based AESGCM failed2 %d", status, log::error);
            print_error_message(status);
            attMsg->set_status(MSG_SGX_FAILED);
        } 
        else {
            Log("Send attestation okay");
        }
    }


cleanup:

    string s;
    Messages::AllInOneMessage aio_ret_msg;
    aio_ret_msg.set_type(Messages::Type::RA_APP_ATT_OK);
    aio_ret_msg.set_allocated_initmsg(attMsg);
    aio_ret_msg.set_sessionid(session_id);
    if(aio_ret_msg.SerializeToString(&s)) {
        Log("Serialization successful");
        atomic_wr_session(session_id,Messages::Type::PHONE_REG);
    }
    else {
        Log("Serialization failed", log::error);
        s = "";
    }

    //SafeFree(p_att_result_msg_full);

    return s;
}

handler_status_t MessageHandler::sendSMS(uint8_t *p_phone, int phone_size, string sms) {
    handler_status_t status = MSG_SUCCESS;

    char *filepath = new char[FILENAME_MAX];
    getcwd(filepath,FILENAME_MAX);
    string path_str(filepath);
    string cmd_str;
    cmd_str.append("java ")
        .append("-jar ")
        .append(path_str)
        .append("/../")
        .append(SENDMESSAGE_PROGRAM)
        .append(" ")
        .append(ByteArrayToStringNoFill(p_phone,phone_size))
        .append(" ")
        .append(sms);

    string sms_res;
    char *buffer = new char[256];
    FILE *pp;
    if((pp = popen(cmd_str.c_str(),"r")) == NULL) {
        Log("use popen function failed!",log::error);
    }
    else {
        while(fgets(buffer,sizeof(buffer),pp)) {
            sms_res.append(buffer);
        }
    }
    pclose(pp);
    sms_res = sms_res.substr(0,sms_res.length()-1);
    istringstream iss(sms_res);
    boost::property_tree::ptree item;
    boost::property_tree::json_parser::read_json(iss,item);
    int sms_status_code = item.get<int>("result");
    if(sms_status_code != 0) {
        status = MSG_PINCODE_SEND_FAILED;
        Log("Send short message failed! Error code:%d", sms_status_code, log::error);
    }

    return status;
}

string MessageHandler::handleRegisterMSG(sgx_ra_context_t session_id, Messages::RegisterMessage msg) {
    uint8_t *p_cipher = new uint8_t[PHONE_SIZE];
    uint8_t *p_mac = new uint8_t[CIPHER_MAC_SIZE];
    uint8_t *p_decrypted_phone = new uint8_t[PHONE_SIZE];

    sgx_status_t ret = SGX_SUCCESS;
    sgx_status_t status;

    for(int i=0;i<PHONE_SIZE;i++) {
        p_cipher[i] = msg.cipherphone(i);
    }

    for(int i=0;i<CIPHER_MAC_SIZE;i++) {
        p_mac[i] = msg.mac(i);
    }

    memcpy(&(g_session_mapping_um[session_id]->cipher_phone),p_cipher,PHONE_SIZE);
    memcpy(&(g_session_mapping_um[session_id]->cipher_phone_mac),p_mac,CIPHER_MAC_SIZE);

    ret = decrypt_phone(this->enclave->getID(),
                        &status,
                        session_id,
                        p_cipher,
                        PHONE_SIZE,
                        p_mac,
                        p_decrypted_phone);
    
    Messages::PinCodeToMessage *pincodetomsg = new Messages::PinCodeToMessage();
    pincodetomsg->set_type(Messages::Type::PIN_CODE_TO);
    pincodetomsg->set_status(MSG_SUCCESS);

    if(SGX_SUCCESS != ret) {
        Log("Decrypt phone number failed! Error code:%lx", ret, log::error);
        pincodetomsg->set_status(ret);
    }
    else if(SGX_SUCCESS != status) {
        Log("Decrypt phone number failed(Inner failed)! Error code:%lx", status, log::error);
        pincodetomsg->set_status(ret);
    }
    else {
        string pincode = RandomNum(PIN_CODE_SIZE);
        //for test
        Log("pin code:%s",pincode);
        memcpy(g_session_mapping_um[session_id]->pin_code,pincode.c_str(),PIN_CODE_SIZE);
        for(int i=0;i<PIN_CODE_SIZE;i++) {
            g_session_mapping_um[session_id]->pin_code[i] -= '0';
        }
        Log("pincode copied:%s",ByteArrayToStringNoFill(g_session_mapping_um[session_id]->pin_code,6));
        /*
        handler_status_t handler_ret = sendSMS(p_decrypted_phone, PHONE_SIZE, pincode);
        if(MSG_SUCCESS == handler_ret) {
            memcpy(g_session_mapping_um[session_id]->pin_code,pincode.c_str(),PIN_CODE_SIZE);
            for(int i=0;i<PIN_CODE_SIZE;i++) {
                g_session_mapping_um[session_id]->pin_code[i] -= '0';
            }
        }
        else {
            pincodetomsg->set_status(handler_ret);
        }
        */
    }

    string s;
    Messages::AllInOneMessage aio_ret_msg;
    aio_ret_msg.set_type(Messages::Type::PIN_CODE_TO);
    aio_ret_msg.set_allocated_pincodetomsg(pincodetomsg);
    aio_ret_msg.set_sessionid(session_id);
    if(aio_ret_msg.SerializeToString(&s)) {
        Log("Serialization successful");
        atomic_wr_session(session_id,Messages::Type::PIN_CODE_BACK);
    }
    else {
        Log("Serialization failed", log::error);
    }

    return s;
}

string MessageHandler::handlePinCodeBack(sgx_ra_context_t session_id, Messages::PinCodeBackMessage msg) {
    uint8_t *p_cipher = g_session_mapping_um[session_id]->cipher_phone;
    uint8_t *p_mac = g_session_mapping_um[session_id]->cipher_phone_mac;
    uint8_t *p_pincode = g_session_mapping_um[session_id]->pin_code;
    uint8_t *p_user_id = new uint8_t[USER_ID_SIZE];
    uint8_t *p_sealed_phone = new uint8_t[CIPHER_SIZE];
    uint32_t sealed_data_len;

    sgx_status_t ret = SGX_SUCCESS;
    sgx_status_t status;

    Messages::ResponseMessage *resmsg = new Messages::ResponseMessage();
    resmsg->set_type(Messages::Type::PHONE_REG_END);
    resmsg->set_status(MSG_SUCCESS);

    // check if pin code is the same
    for(int i=0;i<PIN_CODE_SIZE;i++) {
        if(p_pincode[i] != msg.pincode(i)) {
            Log("Pin code mismatch!",log::error);
            resmsg->set_status(MSG_SGX_FAILED);
            goto cleanup;
        }
    }

    ret = register_user(this->enclave->getID(),
                        &status,
                        session_id,
                        p_cipher,
                        PHONE_SIZE,
                        p_mac,
                        MAX_VERIFICATION_RESULT,
                        p_user_id,
                        p_sealed_phone,
                        &sealed_data_len);

    Log("========== sealed phone:%d ===========",sealed_data_len);
    for(int i=0; i<sealed_data_len; i++) {
        printf("%u,",p_sealed_phone[i]);
    }
    printf("\n");


    if(!putSealedPhone(session_id, p_user_id, p_sealed_phone, sealed_data_len)){
        resmsg->set_status(MSG_SGX_FAILED);
        goto cleanup;
    }
    

    if (SGX_SUCCESS != ret) {
        Log("Error, attestation result message secret using SK based AESGCM failed1 %d", ret, log::error);
        print_error_message(ret);
        resmsg->set_status(MSG_SGX_FAILED);
    } 
    else if (SGX_SUCCESS != status) {
        Log("Error, attestation result message secret using SK based AESGCM failed2 %d", status, log::error);
        print_error_message(status);
        resmsg->set_status(MSG_SGX_FAILED);
    } 
    else {
        Log("Send attestation okay");
        string userID = ByteArrayToString(p_user_id, USER_ID_SIZE);
        Log("user id:%s",userID);
        for(int i=0; i<USER_ID_SIZE; i++) {
            resmsg->add_userid(p_user_id[i]);
        }
        resmsg->set_size(USER_ID_SIZE);
    }


cleanup:

    string s;
    Messages::AllInOneMessage aio_ret_msg;
    aio_ret_msg.set_type(Messages::Type::PHONE_REG_END);
    aio_ret_msg.set_allocated_resmsg(resmsg);
    aio_ret_msg.set_sessionid(session_id);
    if(aio_ret_msg.SerializeToString(&s)) {
        Log("Serialization successful");
        atomic_wr_session(session_id,Messages::Type::SESSION_CLOSE);
    }
    else {
        Log("Serialization failed", log::error);
    }

    this->enclave->closeSession(session_id);

    return s;
}

bool MessageHandler::putSealedPhone(sgx_ra_context_t session_id, uint8_t *userID, uint8_t *p_sealed_phone, uint32_t sealed_phone_len) {
    bool ret = true;
    string userID_str = ByteArrayToString(userID, USER_ID_SIZE);
    string sealed_phone_str = ByteArrayToString(p_sealed_phone, sealed_phone_len);
    string sql_str = string("INSERT INTO userID2Phone (userID,cipherPhone) VALUE ") + "('" + userID_str + "','" + sealed_phone_str + "')" + " on DUPLICATE KEY UPDATE cipherPhone='" + sealed_phone_str + "'";

    if (mysqlConnector->exeQuery(sql_str, NULL, 0)) {
        Log("Store sealed data successfully");
    } else {
        Log("Store sealed data failed", log::error);
        ret = false;
    }

    return ret;
}

bool MessageHandler::getPhoneByUserID(sgx_ra_context_t session_id, uint8_t *userID, uint8_t *p_unsealed_phone) {
    bool ret_b = true;
    sgx_status_t ret;
    sgx_status_t status;
    string userID_str = ByteArrayToString(userID, USER_ID_SIZE);
    string sql_str = "SELECT cipherPhone from userID2Phone where userID='" + userID_str + "'";
    uint8_t *sealed_data = new uint8_t[CIPHER_SIZE];
    uint32_t sealed_data_len;

    if(mysqlConnector->exeQuery(sql_str, sealed_data, &sealed_data_len)) {
        uint32_t unsealed_phone_len;
        uint8_t *unsealed_data = new uint8_t[32];
        ret = unseal_phone(this->enclave->getID(),
                           &status,
                           session_id,
                           sealed_data,
                           sealed_data_len,
                           unsealed_data,
                           &unsealed_phone_len);

        if (SGX_SUCCESS == ret) {
            memcpy(p_unsealed_phone, unsealed_data, unsealed_phone_len);
        } else {
            Log("Unseal phone number failed!", log::error);
        }
    } else {
        Log("Get sealed data from database failed!", log::error);
        ret = SGX_ERROR_UNEXPECTED;
    }

    if (SGX_SUCCESS != ret) {
        ret_b = false;
    }

    return ret_b;
}

string MessageHandler::handleSMS(sgx_ra_context_t session_id, Messages::SMSMessage msg) {
    uint8_t *p_user_id = new uint8_t[USER_ID_SIZE];
    uint8_t *p_unsealed_phone = new uint8_t[PHONE_SIZE];
    uint32_t sms_size = msg.size();
    uint8_t *sms_data = new uint8_t[sms_size];

    for(int i=0; i<USER_ID_SIZE; i++) {
        p_user_id[i] = msg.userid(i);
    }

    for(int i=0; i<sms_size; i++) {
        sms_data[i] = msg.sms(i);
    }


    Messages::SMSResponseMessage *smsresMsg = new Messages::SMSResponseMessage();
    smsresMsg->set_type(Messages::Type::SMS_RES);
    smsresMsg->set_status(MSG_SUCCESS);

    if(getPhoneByUserID(session_id, p_user_id, p_unsealed_phone)) {
        /*
        Log("========== get phone successfully! ==========");
        for(int i=0; i<PHONE_SIZE; i++) {
            printf("%u,",p_unsealed_phone[i]);
        }
        printf("\n");
        */

        handler_status_t handle_ret = sendSMS(p_unsealed_phone,PHONE_SIZE,ByteArrayToStringNoFill(sms_data,sms_size));
        if(MSG_SUCCESS != handle_ret) {
            smsresMsg->set_status(handle_ret);
            Log("Send short message failed!", log::error);
        }

    } else {
        smsresMsg->set_status(MSG_SGX_FAILED);
    }


    string s;
    Messages::AllInOneMessage aio_ret_msg;
    aio_ret_msg.set_type(Messages::Type::SMS_RES);
    aio_ret_msg.set_allocated_smsresmsg(smsresMsg);
    aio_ret_msg.set_sessionid(session_id);
    if(aio_ret_msg.SerializeToString(&s)) {
        Log("Serialization successful");
    }
    else {
        Log("Serialization failed", log::error);
        s = "";
    }

    this->enclave->closeSession(session_id);

    return s;
}

string MessageHandler::handleMSG0(sgx_ra_context_t session_id, Messages::MessageMSG0 msg) {
    Log("MSG0 response received");

    if (msg.status() == TYPE_OK) {

        auto ret = this->generateMSG1(session_id);

        return ret;
        /*
        sgx_status_t ret = this->initEnclave();

        if (SGX_SUCCESS != ret || this->getEnclaveStatus()) {
            Log("Error, call enclave_init_ra fail", log::error);
        } else {
            Log("Call enclave_init_ra success");
            Log("Sending msg1 to remote attestation service provider. Expecting msg2 back");

            auto ret = this->generateMSG1(session_id);

            return ret;
        }
        */

    } else {
        Log("MSG0 response status was not OK", log::error);
    }

    return "";
}


string MessageHandler::handleVerification(sgx_ra_context_t session_id) {
    Log("Verification request received");
    return this->generateMSG0(session_id);
}


/*
string MessageHandler::createInitMsg(int type, string msg) {
    Messages::SecretMessage init_msg;
    init_msg.set_type(type);
    init_msg.set_size(msg.size());

    string s;
    init_msg.SerializeToString(&s);
    return s;
    //return nm->serialize(init_msg);
}
*/

sgx_ra_context_t MessageHandler::getAddSession(Messages::AllInOneMessage aio_msg, 
        handler_status_t *p_handler_status, 
        sgx_status_t *p_sgx_status) {

    sgx_ra_context_t session_id;
    *p_handler_status = MSG_SUCCESS;

    if(aio_msg.sessionid() == UINT_MAX) {

        session_id = this->enclave->createSession(p_sgx_status);

        if(SGX_SUCCESS != *p_sgx_status) return 0;

        if(g_session_mapping_um.count(session_id) == 0) {
            g_session_mapping_um[session_id] = new SessionData();
        }

        Log("Create new session successfully! Session id:%d",session_id);

    } else {
        session_id = aio_msg.sessionid();

        pthread_rwlock_rdlock(&g_session_mapping_um[session_id]->rwlock);
        if(g_session_mapping_um[session_id]->msg_type != aio_msg.type()) {
            *p_handler_status = MSG_TYPE_NOT_MATCH;
            Log("Message type mismatch!",log::error);
        }
        pthread_rwlock_unlock(&g_session_mapping_um[session_id]->rwlock);
    }

    return session_id;
}

void MessageHandler::atomic_wr_session(sgx_ra_context_t session_id, Messages::Type type) {
    pthread_rwlock_wrlock(&g_session_mapping_um[session_id]->rwlock);
    g_session_mapping_um[session_id]->msg_type = type;
    pthread_rwlock_unlock(&g_session_mapping_um[session_id]->rwlock);
}


vector<string> MessageHandler::handleMessages(unsigned char* bytes, int len) {
    vector<string> res;
    string s;
    bool ret;

    Messages::AllInOneMessage aio_msg;
    //ret = aio_msg.ParseFromString(v);
    ret = aio_msg.ParseFromArray(bytes, len);
    if (! ret) {
        Log("Parse message failed!", log::error);
        //fflush(stdout);
        return res;
    }
    Log("type is:%d", aio_msg.type());
    
    // check session
    handler_status_t handler_status;
    sgx_status_t sgx_status;
    sgx_ra_context_t session_id = getAddSession(aio_msg, &handler_status, &sgx_status);
    if(SGX_SUCCESS != sgx_status) {
        Log("SGX create session failed! Error code:%d", sgx_status, log::error);
        return res;
    }  
    if( MSG_SUCCESS != handler_status) {
        Log("Messages handler failed! Error code:%d", handler_status, log::error);
        return res;
    }

    // handle messages
    switch (aio_msg.type()) {
    case Messages::Type::RA_VERIFICATION: {	//Verification request
        Log("========== Generate Msg0 ==========");
        Messages::InitialMessage init_msg = aio_msg.initmsg();
        s = this->handleVerification(session_id);
        Log("========== Generate Msg0 ok ==========");
    }
    break;
    case Messages::Type::RA_MSG0: {		//Reply to MSG0
        Log("========== Generate Msg1 ==========");
        Messages::MessageMSG0 msg0 = aio_msg.msg0();
        s = this->handleMSG0(session_id,msg0);
        Log("========== Generate Msg1 ok ==========");
    }
    break;
    case Messages::Type::RA_MSG2: {		//MSG2
        Log("========== Generate Msg3 ==========");
        Messages::MessageMSG2 msg2 = aio_msg.msg2();
        s = this->handleMSG2(session_id,msg2);
        Log("========== Generate Msg3 ok ==========");
    }
    break;
    case Messages::Type::RA_ATT_RESULT: {	//Reply to MSG3
        Log("========== Generate att msg ==========");
        Messages::AttestationMessage att_msg = aio_msg.attestmsg();
        s = this->handleAttestationResult(session_id,att_msg);
        Log("========== Generate att msg ok ==========");
    }
    break;
    case Messages::Type::PHONE_REG: {
        Log("========== Register user ==========");
        Messages::RegisterMessage reg_msg = aio_msg.regmsg();
        s = this->handleRegisterMSG(session_id,reg_msg);
        Log("========== Register user ok ==========");
    }
    break;
    case Messages::Type::PIN_CODE_BACK: {
        Log("========== Verify pin code ==========");
        Messages::PinCodeBackMessage reg_msg = aio_msg.pincodebackmsg();
        s = this->handlePinCodeBack(session_id,reg_msg);
        Log("========== Verify pin code ok ==========");
    }
    break;
    case Messages::Type::SMS_SEND: {
        Log("========== send short message ==========");
        Messages::SMSMessage sms_msg = aio_msg.smsmsg();
        s = this->handleSMS(session_id,sms_msg);
        Log("========== send short message end ==========");
    }
    break;
    default:
        Log("Unknown type", log::error);
        break;
    }
    //fflush(stdout);

    res.push_back(s);

    return res;
}
