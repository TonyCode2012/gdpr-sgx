#include "MessageHandler.h"
#include "sgx_tseal.h"
#include <inttypes.h>

using namespace util;

MessageHandler::MessageHandler(int port) {
    this->local_ec256_fix_data.g_key_flag = 1;
}

MessageHandler::~MessageHandler() {
    delete this->enclave;
}


/*
int MessageHandler::init() {
    this->nm->Init();
    this->nm->connectCallbackHandler([this](string v, int type) {
        return this->incomingHandler(v, type);
    });
}


void MessageHandler::start() {
    this->nm->startService();
}
*/


sgx_status_t MessageHandler::initEnclave() {
    Log("========== STATUS IS ==========");
    Log("\tmy flag is:%d",this->my_flag);
    this->enclave = Enclave::getInstance();
    sgx_status_t ret = this->enclave->createEnclave();
    if(this->my_flag == 0) {
        this->my_flag = 1;
    } 
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


string MessageHandler::generateMSG0() {
    Log("Call MSG0 generate");
    string s;

    uint32_t extended_epid_group_id;
    int ret = this->getExtendedEPID_GID(&extended_epid_group_id);

    Messages::MessageMSG0 *msg = new Messages::MessageMSG0();
    msg->set_type(Messages::Type::RA_MSG0);

    if (ret == SGX_SUCCESS) {
        msg->set_epid(extended_epid_group_id);
    } else {
        msg->set_status(TYPE_TERMINATE);
        msg->set_epid(0);
    }
    Messages::AllInOneMessage aio_ret_msg;
    aio_ret_msg.set_type(Messages::Type::RA_MSG0);
    aio_ret_msg.set_allocated_msg0(msg);
    if(aio_ret_msg.SerializeToString(&s)) {
        Log("Serialization successful");
    }
    else {
        Log("Serialization failed", log::error);
        s = "";
    }

    return s;
}


string MessageHandler::generateMSG1() {
    int retGIDStatus = 0;
    int count = 0;
    sgx_ra_msg1_t sgxMsg1Obj;
    Log("========== SEALED ENCLAVE PUB KEY ==========");
    local_ec256_fix_data.g_key_flag = 0;

    while (1) {
        retGIDStatus = sgx_ra_get_msg1(this->enclave->getContext(),
                                       this->enclave->getID(),
                                       sgx_ra_get_ga,
                                       &sgxMsg1Obj,
                                       &local_ec256_fix_data);
        
        /*
        unsigned char pubbuf[sizeof(sgx_ec256_public_t)];
        memcpy(pubbuf, (unsigned char*)&local_ec256_fix_data.ec256_public_key, sizeof(sgx_ec256_public_t));
        Log("\tenclave public key:%s",ByteArrayToString(pubbuf,sizeof(pubbuf)));
        unsigned char pribuf_r[sizeof(sgx_ec256_private_t)];
        memcpy(pribuf_r, (unsigned char*)&local_ec256_fix_data.ec256_private_key, sizeof(sgx_ec256_private_t));
        Log("\tenclave privat key:%s",ByteArrayToString(pribuf_r,sizeof(pribuf_r)));
        Log("\tsealed data size  :%d",local_ec256_fix_data.sealed_data_size);
        unsigned char psealedbuf[local_ec256_fix_data.sealed_data_size];
        memcpy(psealedbuf, (unsigned char*)local_ec256_fix_data.p_sealed_data, local_ec256_fix_data.sealed_data_size);
        Log("\tp sealed data is  :%s",ByteArrayToString(psealedbuf,sizeof(psealedbuf)));
        */
        

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


    if (SGX_SUCCESS == retGIDStatus) {
        Messages::MessageMSG1 *msg = new Messages::MessageMSG1();
        msg->set_type(Messages::Type::RA_MSG1);

        for (auto x : sgxMsg1Obj.g_a.gx)
            msg->add_gax(x);

        for (auto x : sgxMsg1Obj.g_a.gy)
            msg->add_gay(x);

        for (auto x : sgxMsg1Obj.gid) {
            msg->add_gid(x);
        }

        string s;
        Messages::AllInOneMessage aio_ret_msg;
        aio_ret_msg.set_type(Messages::Type::RA_MSG1);
        aio_ret_msg.set_allocated_msg1(msg);
        if(aio_ret_msg.SerializeToString(&s)) {
            Log("Serialization successful");
        }
        else {
            Log("Serialization failed", log::error);
            s = "";
        }
        return s;
    }

    return "";
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
    Log("\tpub key gx:%s",ByteArrayToString(pub_key_gx,32));
    Log("\tpub key gy:%s",ByteArrayToString(pub_key_gy,32));

    for (int i=0; i<16; i++) {
        spid.id[i] = msg.spid(i);
    }
    Log("\tspid:%s",ByteArrayToString(spid.id,16));

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
    Log("\tquote type:%d",p_msg2->quote_type);
    Log("\tkdf id    :%d",p_msg2->kdf_id);

    uint8_t smac[16];
    for (int i=0; i<16; i++)
        smac[i] = msg.smac(i);
    Log("\tsmac:%s",ByteArrayToString(smac,16));

    memcpy(&p_msg2->mac, &smac, sizeof(smac));

    p_msg2->sig_rl_size = msg.sizesigrl();
    uint8_t *sigrl = (uint8_t*) malloc(sizeof(uint8_t) * msg.sizesigrl());
    Log("\tsig rl size:%d",p_msg2->sig_rl_size);

    for (int i=0; i<msg.sizesigrl(); i++)
        sigrl[i] = msg.sigrl(i);
    Log("\tsigrl:%s",ByteArrayToString(sigrl, p_msg2->sig_rl_size));

    memcpy(&p_msg2->sig_rl, &sigrl, msg.sizesigrl());

    *pp_msg2 = p_msg2;
}


string MessageHandler::handleMSG2(Messages::MessageMSG2 msg) {
    Log("Received MSG2");

    uint32_t size = msg.size();

    sgx_ra_msg2_t *p_msg2;
    this->assembleMSG2(msg, &p_msg2);

    sgx_ra_msg3_t *p_msg3 = NULL;
    uint32_t msg3_size;
    int ret = 0;

    Log("========== sgx ra proc msg2 para ==========");
    /*
    uint8_t cbuf1[sizeof(sgx_ra_context_t)];
    sgx_ra_context_t tmp_context = this->enclave->getContext();
    memcpy(cbuf1, (uint8_t*)&tmp_context,sizeof(sgx_ra_context_t));
    Log("\tcontext:%s",ByteArrayToString(cbuf1,sizeof(sgx_ra_context_t)));
    Log("\tsize:%d",size);
    uint8_t cbuf2[sizeof(sgx_ra_msg2_t)];
    memcpy(cbuf2, (uint8_t*)p_msg2, sizeof(sgx_ra_msg2_t));
    Log("\tp_msg2:%s",ByteArrayToString(cbuf2,sizeof(sgx_ra_msg2_t)));
    printf("%" PRIu64 "\n",this->enclave->getID());
    */

    do {
        ret = sgx_ra_proc_msg2(this->enclave->getContext(),
                               this->enclave->getID(),
                               sgx_ra_proc_msg2_trusted,
                               sgx_ra_get_msg3_trusted,
                               p_msg2,
                               size,
                               &p_msg3,
                               &msg3_size);
    } while (SGX_ERROR_BUSY == ret && busy_retry_time--);
    SafeFree(p_msg2);

    if (SGX_SUCCESS != (sgx_status_t)ret) {
        Log("Error, call sgx_ra_proc_msg2 fail, error code: %lx", ret);
    } else {
        Log("Call sgx_ra_proc_msg2 success");

        Messages::MessageMSG3 *msg3 = new Messages::MessageMSG3();

        msg3->set_type(Messages::Type::RA_MSG3);
        msg3->set_size(msg3_size);

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

        SafeFree(p_msg3);

        string s;
        Messages::AllInOneMessage aio_ret_msg;
        aio_ret_msg.set_type(Messages::Type::RA_MSG3);
        aio_ret_msg.set_allocated_msg3(msg3);
        if(aio_ret_msg.SerializeToString(&s)) {
            Log("Serialization successful");
        }
        else {
            Log("Serialization failed", log::error);
            s = "";
        }
        return s;
    }

    SafeFree(p_msg3);

    return "";
}


void MessageHandler::assembleAttestationMSG(Messages::AttestationMessage msg, ra_samp_response_header_t **pp_att_msg) {
    Log("\t========= assemble attestation msg 1");
    sample_ra_att_result_msg_t *p_att_result_msg = NULL;
    ra_samp_response_header_t* p_att_result_msg_full = NULL;
    int msg_size = msg.size();    
    Log("Att msg size %d", msg_size);

    int msg_resultsize = msg.resultsize();
    Log("Att msg result size %d", msg_resultsize);

    int total_size = msg.size() + sizeof(ra_samp_response_header_t) + msg.resultsize();
    Log("Att result total size %d", total_size);

    p_att_result_msg_full = (ra_samp_response_header_t*) malloc(total_size);

    memset(p_att_result_msg_full, 0, total_size);

    p_att_result_msg_full->type = Messages::Type::RA_ATT_RESULT;
    p_att_result_msg_full->size = msg.size();
    Log("Att result type: %d", p_att_result_msg_full->type);
    Log("Att platform_info_blob_t size: %d", sizeof(ias_platform_info_blob_t));

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
    Log("Att result payload_size: %d", p_att_result_msg->secret.payload_size);

    for (int i=0; i<12; i++)
        p_att_result_msg->secret.reserved[i] = msg.reserved(i);
    Log("Att result reserved");

    for (int i=0; i<SAMPLE_SP_TAG_SIZE; i++) {
        p_att_result_msg->secret.payload_tag[i] = msg.payloadtag(i);
    }

    for (int i=0; i<msg.resultsize(); i++) {
        p_att_result_msg->secret.payload[i] = (uint8_t)msg.payload(i);
    }

    *pp_att_msg = p_att_result_msg_full;
}


string MessageHandler::handleAttestationResult(Messages::AttestationMessage msg) {
    Log("Received Attestation result, start to sssemble");

    ra_samp_response_header_t *p_att_result_msg_full = NULL;
    this->assembleAttestationMSG(msg, &p_att_result_msg_full);
    Log("Assemble Success");

    sample_ra_att_result_msg_t *p_att_result_msg_body = (sample_ra_att_result_msg_t *) ((uint8_t*) p_att_result_msg_full + sizeof(ra_samp_response_header_t));

    sgx_status_t status;
    sgx_status_t ret;
    ret = verify_att_result_mac(this->enclave->getID(),
                                &status,
                                this->enclave->getContext(),
                                (uint8_t*)&p_att_result_msg_body->platform_info_blob,
                                sizeof(ias_platform_info_blob_t),
                                (uint8_t*)&p_att_result_msg_body->mac,
                                sizeof(sgx_mac_t));


    if ((SGX_SUCCESS != ret) || (SGX_SUCCESS != status)) {
        Log("Error: INTEGRITY FAILED - attestation result message MK based cmac failed. ret:%lx",ret);
        Log("Error: INTEGRITY FAILED - attestation result message MK based cmac failed. status:%lx",status);
        return "";
    }

    if (0 != p_att_result_msg_full->status[0] || 0 != p_att_result_msg_full->status[1]) {
        Log("Error, attestation mac result message MK based cmac failed", log::error);
    } else {
        ret = verify_secret_data(this->enclave->getID(),
                                 &status,
                                 this->enclave->getContext(),
                                 p_att_result_msg_body->secret.payload,
                                 p_att_result_msg_body->secret.payload_size,
                                 p_att_result_msg_body->secret.payload_tag,
                                 MAX_VERIFICATION_RESULT,
                                 NULL);

        SafeFree(p_att_result_msg_full);

        if (SGX_SUCCESS != ret) {
            Log("Error, attestation result message secret using SK based AESGCM failed1 %d", ret, log::error);

            print_error_message(ret);
        } 
        else if (SGX_SUCCESS != status) {
            Log("Error, attestation result message secret using SK based AESGCM failed2 %d", status, log::error);

            print_error_message(status);
        } 
        else {
            Log("Send attestation okay");

            Messages::InitialMessage *msg = new Messages::InitialMessage();
            msg->set_type(Messages::Type::RA_APP_ATT_OK);
            msg->set_size(0);

            string s;
            Messages::AllInOneMessage aio_ret_msg;
            aio_ret_msg.set_type(Messages::Type::RA_APP_ATT_OK);
            aio_ret_msg.set_allocated_initmsg(msg);
            if(aio_ret_msg.SerializeToString(&s)) {
                Log("Serialization successful");
            }
            else {
                Log("Serialization failed", log::error);
                s = "";
            }
            return s;
        }
    }

    SafeFree(p_att_result_msg_full);

    return "";
}

string MessageHandler::handleRegisterMSG(Messages::RegisterMessage msg) {
    uint8_t *p_cipher = new uint8_t[11];
    uint8_t *p_mac = new uint8_t[16];
    uint8_t *p_user_id = new uint8_t[16];
    //uint8_t *p_sealed_phone = new uint8_t[580];
    uint32_t sealed_data_len;
    
    for(int i=0;i<11;i++) {
        p_cipher[i] = msg.cipherphone(i);
    }

    for(int i=0;i<16;i++) {
        p_mac[i] = msg.mac(i);
    }

    sgx_status_t status;
    sgx_status_t ret = register_user(this->enclave->getID(),
                                     &status,
                                     this->enclave->getContext(),
                                     p_cipher,
                                     11,
                                     p_mac,
                                     MAX_VERIFICATION_RESULT,
                                     p_user_id,
                                     NULL,
                                     //p_sealed_phone,
                                     &sealed_data_len);

    Log("========== sealed phone:%d ===========",sealed_data_len);
    //for(int i=0; i<sealed_data_len; i++) {
    //    printf("%u,",p_sealed_phone[i]);
    //}
    printf("\n");

    if (SGX_SUCCESS != ret) {
        Log("Error, attestation result message secret using SK based AESGCM failed1 %d", ret, log::error);
        print_error_message(ret);
    } 
    else if (SGX_SUCCESS != status) {
        Log("Error, attestation result message secret using SK based AESGCM failed2 %d", status, log::error);
        print_error_message(status);
    } 
    else {
        Log("Send attestation okay");
        string userID = ByteArrayToString(p_user_id, 16);
        Log("user id:%s",userID);
        Messages::ResponseMessage *msg = new Messages::ResponseMessage();
        msg->set_type(Messages::Type::PHONE_RES);
        for(int i=0; i<16; i++) {
            msg->add_userid(p_user_id[i]);
        }
        msg->set_size(16);
        string s;
        Messages::AllInOneMessage aio_ret_msg;
        aio_ret_msg.set_type(Messages::Type::PHONE_RES);
        aio_ret_msg.set_allocated_resmsg(msg);
        if(aio_ret_msg.SerializeToString(&s)) {
            Log("Serialization successful");
        }
        else {
            Log("Serialization failed", log::error);
            s = "";
        }
        return s;
    }

    return "";
}


string MessageHandler::handleMSG0(Messages::MessageMSG0 msg) {
    Log("MSG0 response received");

    if (msg.status() == TYPE_OK) {
        sgx_status_t ret = this->initEnclave();

        if (SGX_SUCCESS != ret || this->getEnclaveStatus()) {
            Log("Error, call enclave_init_ra fail", log::error);
        } else {
            Log("Call enclave_init_ra success");
            Log("Sending msg1 to remote attestation service provider. Expecting msg2 back");

            auto ret = this->generateMSG1();

            return ret;
        }

    } else {
        Log("MSG0 response status was not OK", log::error);
    }

    return "";
}


string MessageHandler::handleVerification() {
    Log("Verification request received");
    return this->generateMSG0();
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


string MessageHandler::handleMessages(unsigned char* bytes, int len) {
    string res;
    bool ret;

    Messages::AllInOneMessage aio_msg;
    //ret = aio_msg.ParseFromString(v);
    ret = aio_msg.ParseFromArray(bytes, len);
    if (! ret) {
        Log("Parse message failed!", log::error);
        fflush(stdout);
        return res;
    }
    Log("type is:%d", aio_msg.type());

    switch (aio_msg.type()) {
    case Messages::Type::RA_VERIFICATION: {	//Verification request
        Log("========== Generate Msg0 ==========");
        Messages::InitialMessage init_msg = aio_msg.initmsg();
        res = this->handleVerification();
        Log("========== Generate Msg0 ok ==========");
    }
    break;
    case Messages::Type::RA_MSG0: {		//Reply to MSG0
        Log("========== Generate Msg1 ==========");
        Messages::MessageMSG0 msg0 = aio_msg.msg0();
        res = this->handleMSG0(msg0);
        Log("========== Generate Msg1 ok ==========");
    }
    break;
    case Messages::Type::RA_MSG2: {		//MSG2
        Log("========== Generate Msg3 ==========");
        Messages::MessageMSG2 msg2 = aio_msg.msg2();
        res = this->handleMSG2(msg2);
        Log("========== Generate Msg3 ok ==========");
    }
    break;
    case Messages::Type::RA_ATT_RESULT: {	//Reply to MSG3
        Log("========== Generate att msg ==========");
        Messages::AttestationMessage att_msg = aio_msg.attestmsg();
        res = this->handleAttestationResult(att_msg);
        Log("========== Generate att msg ok ==========");
    }
    break;
    case Messages::Type::PHONE_REG: {
        Log("========== Generate user ID ==========");
        Messages::RegisterMessage reg_msg = aio_msg.regmsg();
        res = this->handleRegisterMSG(reg_msg);
        Log("========== Generate user ID ok ==========");
    }
    break;
    default:
        Log("Unknown type", log::error);
        break;
    }
    fflush(stdout);

    return res;
}
