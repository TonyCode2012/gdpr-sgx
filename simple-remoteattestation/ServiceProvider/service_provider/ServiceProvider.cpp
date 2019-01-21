#include "ServiceProvider.h"
#include "sample_libcrypto.h"
#include "../GeneralSettings.h"

// This is the private EC key of SP, the corresponding public EC key is
// hard coded in isv_enclave. It is based on NIST P-256 curve.
static const sample_ec256_private_t g_sp_priv_key = {
    {
        0x90, 0xe7, 0x6c, 0xbb, 0x2d, 0x52, 0xa1, 0xce,
        0x3b, 0x66, 0xde, 0x11, 0x43, 0x9c, 0x87, 0xec,
        0x1f, 0x86, 0x6a, 0x3b, 0x65, 0xb6, 0xae, 0xea,
        0xad, 0x57, 0x34, 0x53, 0xd1, 0x03, 0x8c, 0x01
    }
};

ServiceProvider::ServiceProvider(WebService *ws) : ws(ws) {}

ServiceProvider::~ServiceProvider() {}

void ServiceProvider::setIsVerify(int isVerify) {
    this->isVerify = isVerify;
}


int ServiceProvider::sp_ra_proc_msg0_req(const uint32_t id) {
    int ret = -1;

    if (!this->g_is_sp_registered || (this->extended_epid_group_id != id)) {
        Log("Received extended EPID group ID: %d", id);

        extended_epid_group_id = id;
        this->g_is_sp_registered = true;
        ret = SP_OK;
    }

    return ret;
}


int ServiceProvider::sp_ra_proc_msg1_req(Messages::MessageMSG1 msg1, Messages::MessageMSG2 *msg2) {
    ra_samp_response_header_t **pp_msg2;
    int ret = 0;
    ra_samp_response_header_t* p_msg2_full = NULL;
    sgx_ra_msg2_t *p_msg2 = NULL;
    sample_ecc_state_handle_t ecc_state = NULL;
    sample_status_t sample_ret = SAMPLE_SUCCESS;
    bool derive_ret = false;

    if (!g_is_sp_registered) {
        return SP_UNSUPPORTED_EXTENDED_EPID_GROUP;
    }

    do {
        // =====================  RETRIEVE SIGRL FROM IAS =======================
        uint8_t GID[4];

        for (int i=0; i<4; i++)
            GID[i] = msg1.gid(i);

        reverse(begin(GID), end(GID));

        string sigRl;
        bool error = false;
        if(this->isVerify == 1) {
            error = this->ws->getSigRL(ByteArrayToString(GID, 4), &sigRl);
        }

        if (error) {
            return SP_RETRIEVE_SIGRL_ERROR;
        }

        uint8_t *sig_rl;
        uint32_t sig_rl_size = StringToByteArray(sigRl, &sig_rl);
        //=====================================================================

        uint8_t gaXLittleEndian[32];
        uint8_t gaYLittleEndian[32];

        for (int i=0; i<32; i++) {
            gaXLittleEndian[i] = msg1.gax(i);
            gaYLittleEndian[i] = msg1.gay(i);
        }

        sample_ec256_public_t client_pub_key = {{0},{0}};

        for (int x=0; x<DH_SHARED_KEY_LEN; x++) {
            client_pub_key.gx[x] = gaXLittleEndian[x];
            client_pub_key.gy[x] = gaYLittleEndian[x];
        }

        // Need to save the client's public ECCDH key to local storage
        if (memcpy_s(&g_sp_db.g_a, sizeof(g_sp_db.g_a), &client_pub_key, sizeof(client_pub_key))) {
            Log("Error, cannot do memcpy", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }
        Log("========== [SP INFO] ==========");

        // Generate the Service providers ECCDH key pair.

        //@@@@@@@@@@@@@@@@@@ Use Stanford JS module, change endian before publicKey been sending.
        sample_ret = sample_ecc256_open_context(&ecc_state);
        if(SAMPLE_SUCCESS != sample_ret) {
            Log("Error, cannot get ECC context", log::error);
            ret = -1;
            break;
        }
        Log("\tafter use self-defined ecc state");
        
        //@@@@@@@@@@@@@@@@@@ JS code does not have state, you could ignore
        // print ecc state
        unsigned char statbuf[sizeof(ecc_state)];
        memcpy(statbuf, (unsigned char*)ecc_state, sizeof(ecc_state));
        Log("\tecc state:%s",ByteArrayToString(statbuf,sizeof(statbuf)));

        /*
        unsigned char fakestate[16] = {0x50,0x43,0x45,0x20,0x00,0x00,0x00,0x00};
        memcpy(ecc_state, fakestate, sizeof(fakestate));
        memcpy(statbuf, (unsigned char*)ecc_state, sizeof(ecc_state));
        Log("\tecc state:%s",ByteArrayToString(statbuf,sizeof(statbuf)));
        */


        sample_ec256_public_t pub_key = {{0},{0}};
        sample_ec256_private_t priv_key = {{0}};
        sample_ret = sample_ecc256_create_key_pair(&priv_key, &pub_key, ecc_state);
        /*
        */
        /*
        sample_ec256_public_t pub_key = {
            {
                0xfb,0x0a,0x19,0xe2,0x81,0x19,0x1d,0xdf,0x2c,0x6d,0xb6,0xf7,
                0x32,0x2c,0x96,0x78,0xea,0xa3,0xe5,0x34,0xaa,0x18,0xec,0x66,
                0x35,0x49,0xaf,0xb0,0x68,0x9c,0x4d,0xf2
            },
            {
                0xce,0x20,0x32,0xad,0x51,0x37,0x15,0x30,0xa6,0xc4,0x26,0xd1,
                0xf2,0x57,0x5d,0x7c,0xdb,0x1f,0xb0,0xdf,0x88,0xb6,0x7b,0xa7,0x78,0x88,0x3c,0x5c,0x9d,0xbe,0x5e,0x33
            }
        };
        sample_ec256_private_t priv_key = {
            {
                0xb0,0xc4,0x1a,0x89,0x95,0x6b,0xe1,0x77,0xce,0x0c,0x7a,0x99,
                0x83,0xa3,0x7d,0xfd,0x2f,0x6d,0xd4,0xf9,0x86,0xcc,0x6f,0x2d,
                0xbd,0x9d,0x5e,0x21,0xd0,0xe4,0xc5,0x19
            }
        };
        */

        /*
        // read public and sealed private key from file
        ifstream pri_stream(Settings::ec_pri_key_path_server);
        ifstream pub_stream(Settings::ec_pub_key_path_server);
        string pri_str,pub_str;
        getline(pri_stream,pri_str);
        getline(pub_stream,pub_str);
        uint8_t *ppub;
        uint8_t *ppri;
        HexStringToByteArray(pub_str,&ppub);
        HexStringToByteArray(pri_str,&ppri);
        memcpy(&pub_key,ppub,sizeof(sample_ec256_public_t));
        memcpy(&priv_key,ppri,sizeof(sample_ec256_private_t));
        Log("\tpublic  key:%s",pub_str);
        Log("\tprivate key:%s",pri_str);
        */

        if (SAMPLE_SUCCESS != sample_ret) {
            Log("Error, cannot get key pair", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }


        // Need to save the SP ECCDH key pair to local storage.
        if (memcpy_s(&g_sp_db.b, sizeof(g_sp_db.b), &priv_key,sizeof(priv_key)) ||
                memcpy_s(&g_sp_db.g_b, sizeof(g_sp_db.g_b), &pub_key,sizeof(pub_key))) {
            Log("Error, cannot do memcpy", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        //@@@@@@@@@@@@@@@@@@ >>>>>>>>>>>>>>>folowing is for console print, you could ignore>>>>>>>>>>>>>>>>>>>>>>>

        unsigned char tmp_b_buf[sizeof(sgx_ec256_private_t)];
        memcpy(tmp_b_buf,(unsigned char *)&g_sp_db.b,sizeof(sgx_ec256_private_t));
        Log("\tb  : (%s)", ByteArrayToString(tmp_b_buf, sizeof(sgx_ec256_private_t)));

        sgx_ec256_public_t *tmp_ga = &g_sp_db.g_a;
        unsigned char tmp_ga_buf[sizeof(sgx_ec256_public_t)];
        memcpy(tmp_ga_buf,(unsigned char *)(tmp_ga),sizeof(sgx_ec256_public_t));
        Log("\tga : (%s)", ByteArrayToString(tmp_ga_buf, sizeof(sgx_ec256_public_t)));

        sgx_ec256_public_t *tmp_gb = &g_sp_db.g_b;
        unsigned char tmp_gb_buf[sizeof(sgx_ec256_public_t)];
        memcpy(tmp_gb_buf,(unsigned char *)(tmp_gb),sizeof(sgx_ec256_public_t));
        Log("\tgb : (%s)", ByteArrayToString(tmp_gb_buf, sizeof(sgx_ec256_public_t)));
        //@@@@@@@@@@@@@@@@@@ <<<<<<<<<<<<<<<<<<<<<< Upper is for console print >>>>>>>>>>>>>>>>>>>>

        // Generate the client/SP shared secret
        sample_ec_dh_shared_t dh_key = {{0}};

        sample_ret = sample_ecc256_compute_shared_dhkey(&priv_key, (sample_ec256_public_t *)&client_pub_key,
                     (sample_ec256_dh_shared_t *)&dh_key,
                     ecc_state);

        if (SAMPLE_SUCCESS != sample_ret) {
            Log("Error, compute share key fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        //@@@@@@@@@@@@@@@@@@ My new simplified keys derivation logic
        memcpy(&g_sp_db.smk_key, &dh_key, sizeof(sgx_ec_key_128bit_t));
        memcpy(&g_sp_db.mk_key, &dh_key, sizeof(sgx_ec_key_128bit_t));
        memcpy(&g_sp_db.sk_key, &dh_key, sizeof(sgx_ec_key_128bit_t));
        memcpy(&g_sp_db.vk_key, &dh_key, sizeof(sgx_ec_key_128bit_t));

/*
        // smk is only needed for msg2 generation.
        derive_ret = derive_key(&dh_key, SAMPLE_DERIVE_KEY_SMK, &g_sp_db.smk_key);
        if (derive_ret != true) {
            Log("Error, derive key fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        // The rest of the keys are the shared secrets for future communication.
        derive_ret = derive_key(&dh_key, SAMPLE_DERIVE_KEY_MK, &g_sp_db.mk_key);
        if (derive_ret != true) {
            Log("Error, derive key fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        derive_ret = derive_key(&dh_key, SAMPLE_DERIVE_KEY_SK, &g_sp_db.sk_key);
        if (derive_ret != true) {
            Log("Error, derive key fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }
        unsigned char bufsk[sizeof(sgx_ec_key_128bit_t)];
        memcpy(bufsk, (unsigned char*)&g_sp_db.sk_key, sizeof(sgx_ec_key_128bit_t));
        Log("========== SK =========");
        Log("\tSK:%s",ByteArrayToString(bufsk,sizeof(sgx_ec_key_128bit_t)));

        derive_ret = derive_key(&dh_key, SAMPLE_DERIVE_KEY_VK, &g_sp_db.vk_key);
        if (derive_ret != true) {
            Log("Error, derive key fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }
*/

        uint32_t msg2_size = sizeof(sgx_ra_msg2_t) + sig_rl_size;
        p_msg2_full = (ra_samp_response_header_t*)malloc(msg2_size + sizeof(ra_samp_response_header_t));

        if (!p_msg2_full) {
            Log("Error, Error, out of memory", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        memset(p_msg2_full, 0, msg2_size + sizeof(ra_samp_response_header_t));
        p_msg2_full->type = RA_MSG2;
        p_msg2_full->size = msg2_size;

        p_msg2_full->status[0] = 0;
        p_msg2_full->status[1] = 0;
        p_msg2 = (sgx_ra_msg2_t *) p_msg2_full->body;


        int spidsize = 16;
        uint8_t *spidBa;
        HexStringToByteArray(Settings::spid, &spidBa);
        for(int i=0;i<16;i++) {
            printf("%d,",spidBa[i]);
        }
        printf("\n");
        unsigned char* buftmp = new unsigned char[spidsize];
        memcpy(buftmp, (unsigned char*)spidBa, spidsize);
        Log("=========== spidBa:%s",ByteArrayToString(buftmp,spidsize));

        for (int i=0; i<16; i++)
            p_msg2->spid.id[i] = spidBa[i];
        unsigned char* buftmp1 = new unsigned char[spidsize];
        memcpy(buftmp1, (unsigned char*)p_msg2->spid.id, spidsize);
        Log("=========== spidBa:%s",ByteArrayToString(buftmp1,spidsize));


        // Assemble MSG2
        if(memcpy_s(&p_msg2->g_b, sizeof(p_msg2->g_b), &g_sp_db.g_b, sizeof(g_sp_db.g_b))) {
            Log("Error, memcpy failed", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        p_msg2->quote_type = SAMPLE_QUOTE_LINKABLE_SIGNATURE;
        p_msg2->kdf_id = AES_CMAC_KDF_ID;

        // Create gb_ga
        sgx_ec256_public_t gb_ga[2];
        if (memcpy_s(&gb_ga[0], sizeof(gb_ga[0]), &g_sp_db.g_b, sizeof(g_sp_db.g_b)) ||
                memcpy_s(&gb_ga[1], sizeof(gb_ga[1]), &g_sp_db.g_a, sizeof(g_sp_db.g_a))) {
            Log("Error, memcpy failed", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        // ------------- test Signature algorithm
        sample_ec256_private_t test_text = {1};
        sample_ec256_signature_t test_signature = {{0},{0}};


        unsigned char test_sign_buf[sizeof(sample_ec256_signature_t)];
        memcpy(test_sign_buf,(unsigned char *)&test_signature,sizeof(sample_ec256_signature_t));
        Log("\t Before Signature : (%s)", ByteArrayToString(test_sign_buf, sizeof(sample_ec256_signature_t)));

        sample_ret = sample_ecdsa_sign((uint8_t *)&test_text, sizeof(sample_ec256_private_t),
                                       (sample_ec256_private_t *)&g_sp_priv_key,
                                       (sample_ec256_signature_t *)&test_signature,
                                       ecc_state);
        
        unsigned char test_test_buf[sizeof(sgx_ec256_private_t)];
        memcpy(test_test_buf,(unsigned char *)&test_text,sizeof(sgx_ec256_private_t));
        Log("\t Plant text: (%s)", ByteArrayToString(test_test_buf, sizeof(sgx_ec256_private_t)));

        unsigned char test_plantext_buf[sizeof(sgx_ec256_private_t)];
        memcpy(test_plantext_buf,(unsigned char *)&g_sp_priv_key,sizeof(sgx_ec256_private_t));
        Log("\t Private Key: (%s)", ByteArrayToString(test_plantext_buf, sizeof(sgx_ec256_private_t)));


        //unsigned char test_sign_buf[sizeof(sample_ec256_signature_t)];
        memcpy(test_sign_buf,(unsigned char *)&test_signature,sizeof(sample_ec256_signature_t));
        Log("\t After Signature: (%s)", ByteArrayToString(test_sign_buf, sizeof(sample_ec256_signature_t)));

        // Sign gb_ga
        sample_ret = sample_ecdsa_sign((uint8_t *)&gb_ga, sizeof(gb_ga),
                                       (sample_ec256_private_t *)&g_sp_priv_key,
                                       (sample_ec256_signature_t *)&p_msg2->sign_gb_ga,
                                       ecc_state);

        unsigned char sign_buf[sizeof(sample_ec256_signature_t)];
        memcpy(sign_buf,(unsigned char *)&p_msg2->sign_gb_ga,sizeof(sample_ec256_signature_t));
        Log("\t SP Signature: (%s)", ByteArrayToString(sign_buf, sizeof(sample_ec256_signature_t)));

        if (SAMPLE_SUCCESS != sample_ret) {
            Log("Error, sign ga_gb fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }


        // Generate the CMACsmk for gb||SPID||TYPE||KDF_ID||Sigsp(gb,ga)

        
        //Test AES method
        /* 
        uint8_t test_v[16] = {
            0x2b,0xc3,0xcb,0x9c,0xfb,0x80,0xb3,0x4f,
            0xc6,0x5c,0x03,0x3a,0x29,0x3a,0x0b,0x71
            };

        sgx_ec_key_128bit_t test_k = {
            0x2b,0xc3,0xcb,0x9c,0xfb,0x80,0xb3,0x4f,
            0xc6,0x5c,0x03,0x3a,0x29,0x3a,0x0b,0x71
            };
        Log("\t Aes Mac size: %d", sizeof(sample_aes_gcm_128bit_tag_t));
        //uint8_t aes_mac[SAMPLE_AESGCM_MAC_SIZE] = {0};
        uint8_t aes_gcm_iv[SAMPLE_SP_IV_SIZE] = {0};
        uint8_t test_des[16] = {0};
        sample_aes_gcm_128bit_tag_t aes_mac = {0};
        ret = sample_rijndael128GCM_encrypt(&test_k,
                                                &test_v[0],
                                                sizeof(sgx_ec_key_128bit_t),
                                                &test_des[0],
                                                &aes_gcm_iv[0],
                                                SAMPLE_SP_IV_SIZE,
                                                NULL,
                                                0,
                                                &aes_mac);


        sgx_ec_key_128bit_t *tmp_des = &test_des;
        unsigned char tmp_des_buf[sizeof(sgx_ec_key_128bit_t)];
        memcpy(tmp_des_buf,(unsigned char *)(tmp_des),sizeof(sgx_ec_key_128bit_t));
        Log("\tEncrypted as : (%s)", ByteArrayToString(tmp_des_buf, sizeof(sgx_ec_key_128bit_t)));

        sample_aes_gcm_128bit_tag_t *tmp_mac = &aes_mac;
        unsigned char tmp_mac_buf[sizeof(sample_aes_gcm_128bit_tag_t)];
        memcpy(tmp_mac_buf,(unsigned char *)(tmp_mac),sizeof(sample_aes_gcm_128bit_tag_t));
        Log("\taes mac : (%s)", ByteArrayToString(tmp_mac_buf, sizeof(sample_aes_gcm_128bit_tag_t)));
        */

        // Test CMAC method
        /*
        uint8_t tmac[SAMPLE_EC_MAC_SIZE] = {0};
        sample_rijndael128_cmac_msg(&test_k, (uint8_t *)&test_v, sizeof(_sgx_ec256_private_t), &tmac);

        _sgx_ec256_private_t *tool_gb = &test_v;
        unsigned char tool_gb_buf[32];
        memcpy(tool_gb_buf,(unsigned char *)(tool_gb),sizeof(_sgx_ec256_private_t));
        Log("\t msg: (%s)", ByteArrayToString(tool_gb_buf, sizeof(_sgx_ec256_private_t)));

        sgx_ec_key_128bit_t *tmp_smk = &test_k;
        unsigned char tmp_smk_buf[sizeof(sgx_ec_key_128bit_t)];
        memcpy(tmp_smk_buf,(unsigned char *)(tmp_smk),sizeof(sgx_ec_key_128bit_t));
        Log("\t key : (%s)", ByteArrayToString(tmp_smk_buf, sizeof(sgx_ec_key_128bit_t)));

        sample_mac_t *tmp_mac = &tmac;
        unsigned char tmp_mac_buf[sizeof(sample_mac_t)];
        memcpy(tmp_mac_buf,(unsigned char *)(tmp_mac),sizeof(sample_mac_t));
        Log("\tmac : (%s)", ByteArrayToString(tmp_mac_buf, sizeof(sample_mac_t)));
        */

        
        uint8_t mac[SAMPLE_EC_MAC_SIZE] = {0};
        //@@@@@@@@@@@@@@@@@@ Please use node-aes-cmac function instead of sample_rijndael128_cmac_msg
        //uint32_t cmac_size = offsetof(sgx_ra_msg2_t, mac);
        //uint32_t cmac_size = sizeof(sample_ec256_public_t);
        //sample_ret = sample_rijndael128_cmac_msg(&g_sp_db.smk_key, (uint8_t *)&p_msg2->g_b, cmac_size, &mac);
        //uint8_t mac[SAMPLE_EC_MAC_SIZE] = {0};
        uint32_t cmac_size = offsetof(sgx_ra_msg2_t, mac);
        sample_ret = sample_rijndael128_cmac_msg(&g_sp_db.smk_key, (uint8_t *)&p_msg2->g_b, cmac_size, &mac);

        if (SAMPLE_SUCCESS != sample_ret) {
            Log("Error, cmac fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        if (memcpy_s(&p_msg2->mac, sizeof(p_msg2->mac), mac, sizeof(mac))) {
            Log("Error, memcpy failed", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        if (memcpy_s(&p_msg2->sig_rl[0], sig_rl_size, sig_rl, sig_rl_size)) {
            Log("Error, memcpy failed", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        p_msg2->sig_rl_size = sig_rl_size;

    } while(0);


    if (ret) {
        *pp_msg2 = NULL;
        SafeFree(p_msg2_full);
    } else {

        //=================   SET MSG2 Fields   ================
        msg2->set_size(p_msg2_full->size);

        for (auto x : p_msg2->g_b.gx)
            msg2->add_public_key_gx(x);

        for (auto x : p_msg2->g_b.gy)
            msg2->add_public_key_gy(x);

        int spidsize = 16;
        uint8_t tmpbuf0[spidsize] = {0};
        memcpy(tmpbuf0, (uint8_t*)p_msg2->spid.id,spidsize);
        Log("==========sp spid:%s",ByteArrayToString(tmpbuf0,spidsize));
        for (auto x : p_msg2->spid.id)
            msg2->add_spid(x);
        for(int i=0;i<16;i++) {
            printf("%d,",msg2->spid(i));
        }
        printf("\n");
        //uint8_t tmpbuf[spidsize] = {0};
        //memcpy(tmpbuf, (uint8_t*)&msg2->spid(),spidsize);
        //Log("==========sp spid:%s",ByteArrayToString(tmpbuf,spidsize));

        msg2->set_quote_type(SAMPLE_QUOTE_LINKABLE_SIGNATURE);
        msg2->set_cmac_kdf_id(AES_CMAC_KDF_ID);

        for (auto x : p_msg2->sign_gb_ga.x) {
            msg2->add_signature_x(x);
        }

        for (auto x : p_msg2->sign_gb_ga.y)
            msg2->add_signature_y(x);

        for (auto x : p_msg2->mac)
            msg2->add_smac(x);

        msg2->set_size_sigrl(p_msg2->sig_rl_size);

        for (int i=0; i<p_msg2->sig_rl_size; i++)
            msg2->add_sigrl(p_msg2->sig_rl[i]);
        //=====================================================
    }

    if (ecc_state) {
        sample_ecc256_close_context(ecc_state);
    }

    return ret;
}


sgx_ra_msg3_t* ServiceProvider::assembleMSG3(Messages::MessageMSG3 msg) {
    sgx_ra_msg3_t *p_msg3 = (sgx_ra_msg3_t*) malloc(msg.size());

    for (int i=0; i<SGX_MAC_SIZE; i++)
        p_msg3->mac[i] = msg.sgx_mac(i);

    for (int i=0; i<SGX_ECP256_KEY_SIZE; i++) {
        p_msg3->g_a.gx[i] = msg.gax_msg3(i);
        p_msg3->g_a.gy[i] = msg.gay_msg3(i);
    }

    for (int i=0; i<256; i++)
        p_msg3->ps_sec_prop.sgx_ps_sec_prop_desc[i] = msg.sec_property(i);

    for (int i=0; i<1116; i++)
        p_msg3->quote[i] = msg.quote(i);

    return p_msg3;
}



// Process remote attestation message 3
int ServiceProvider::sp_ra_proc_msg3_req(Messages::MessageMSG3 msg, Messages::AttestationMessage *att_msg) {
    int ret = 0;
    sample_status_t sample_ret = SAMPLE_SUCCESS;
    const uint8_t *p_msg3_cmaced = NULL;
    sgx_quote_t *p_quote = NULL;
    sample_sha_state_handle_t sha_handle = NULL;
    sample_report_data_t report_data = {0};
    sample_ra_att_result_msg_t *p_att_result_msg = NULL;
    ra_samp_response_header_t* p_att_result_msg_full = NULL;
    uint32_t i;
    sgx_ra_msg3_t *p_msg3 = NULL;
    uint32_t att_result_msg_size;
    int len_hmac_nonce = 0;

    p_msg3 = assembleMSG3(msg);

    // Check to see if we have registered?
    if (!g_is_sp_registered) {
        Log("Unsupported extended EPID group", log::error);
        return -1;
    }

    do {

        // Compare g_a in message 3 with local g_a.
        if (memcmp(&g_sp_db.g_a, &p_msg3->g_a, sizeof(sgx_ec256_public_t))) {
            Log("Error, g_a is not same", log::error);
            ret = SP_PROTOCOL_ERROR;
            break;
        }
        //@@@@@@@@@@@@@@@@@@ >>>>>>>>>>>>>>>>>>>> let's ignore all Following verification >>>>>>>>>>>>>>>>>>>>>>

        //Make sure that msg3_size is bigger than sample_mac_t.
        uint32_t mac_size = msg.size() - sizeof(sample_mac_t);
        p_msg3_cmaced = reinterpret_cast<const uint8_t*>(p_msg3);
        p_msg3_cmaced += sizeof(sample_mac_t);

        // Verify the message mac using SMK
        sample_cmac_128bit_tag_t mac = {0};
        sample_ret = sample_rijndael128_cmac_msg(&g_sp_db.smk_key, p_msg3_cmaced, mac_size, &mac);

        if (SAMPLE_SUCCESS != sample_ret) {
            Log("Error, cmac fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        if (memcmp(&p_msg3->mac, mac, sizeof(mac))) {
            Log("Error, verify cmac fail", log::error);
            ret = SP_INTEGRITY_FAILED;
            break;
        }

        if (memcpy_s(&g_sp_db.ps_sec_prop, sizeof(g_sp_db.ps_sec_prop), &p_msg3->ps_sec_prop, sizeof(p_msg3->ps_sec_prop))) {
            Log("Error, memcpy fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        p_quote = (sgx_quote_t *) p_msg3->quote;


        // Verify the report_data in the Quote matches the expected value.
        // The first 32 bytes of report_data are SHA256 HASH of {ga|gb|vk}.
        // The second 32 bytes of report_data are set to zero.
        sample_ret = sample_sha256_init(&sha_handle);
        if (sample_ret != SAMPLE_SUCCESS) {
            Log("Error, init hash failed", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        sample_ret = sample_sha256_update((uint8_t *)&(g_sp_db.g_a), sizeof(g_sp_db.g_a), sha_handle);
        if (sample_ret != SAMPLE_SUCCESS) {
            Log("Error, udpate hash failed", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        sample_ret = sample_sha256_update((uint8_t *)&(g_sp_db.g_b), sizeof(g_sp_db.g_b), sha_handle);
        if (sample_ret != SAMPLE_SUCCESS) {
            Log("Error, udpate hash failed", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        sample_ret = sample_sha256_update((uint8_t *)&(g_sp_db.vk_key), sizeof(g_sp_db.vk_key), sha_handle);
        if (sample_ret != SAMPLE_SUCCESS) {
            Log("Error, udpate hash failed", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        sample_ret = sample_sha256_get_hash(sha_handle, (sample_sha256_hash_t *)&report_data);
        if (sample_ret != SAMPLE_SUCCESS) {
            Log("Error, Get hash failed", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        if (memcmp((uint8_t *)&report_data, (uint8_t *)&(p_quote->report_body.report_data), sizeof(report_data))) {
            Log("Error, verify hash failed", log::error);
            ret = SP_INTEGRITY_FAILED;
            break;
        }

        // Verify quote with attestation server.
        ias_att_report_t attestation_report = {0};
        ret = ias_verify_attestation_evidence(p_msg3->quote, p_msg3->ps_sec_prop.sgx_ps_sec_prop_desc, &attestation_report, ws, this->isVerify);

        if (0 != ret) {
            ret = SP_IAS_FAILED;
            break;
        }

        Log("Atestation Report:");
        Log("\tid: %s", attestation_report.id);
        Log("\tstatus: %d", attestation_report.status);
        Log("\trevocation_reason: %u", attestation_report.revocation_reason);
        Log("\tpse_status: %d",  attestation_report.pse_status);

        Log("Enclave Report:");
        Log("\tSignature Type: 0x%x", p_quote->sign_type);
        Log("\tSignature Basename: %s", ByteArrayToNoHexString(p_quote->basename.name, 32));
        Log("\tattributes.flags: 0x%0lx", p_quote->report_body.attributes.flags);
        Log("\tattributes.xfrm: 0x%0lx", p_quote->report_body.attributes.xfrm);
        Log("\tmr_enclave: %s", ByteArrayToString(p_quote->report_body.mr_enclave.m, SGX_HASH_SIZE));
        Log("\tmr_signer: %s", ByteArrayToString(p_quote->report_body.mr_signer.m, SGX_HASH_SIZE));
        Log("\tisv_prod_id: 0x%0x", p_quote->report_body.isv_prod_id);
        Log("\tisv_svn: 0x%0x", p_quote->report_body.isv_svn);
        Log("=== Quote structure ===");
        Log("\tversion: 0x%0lx", p_quote->version);
        Log("\tsign type: 0x%0lx", p_quote->sign_type);
        unsigned char buf_qe_svn[sizeof(sgx_isv_svn_t)];
        memcpy(buf_qe_svn,(unsigned char *)(&p_quote->qe_svn),sizeof(sgx_isv_svn_t));
        Log("\tqe svn : (%s)", ByteArrayToString(buf_qe_svn, sizeof(sgx_isv_svn_t)));
        unsigned char buf_pce_svn[sizeof(sgx_isv_svn_t)];
        memcpy(buf_pce_svn,(unsigned char *)(&p_quote->pce_svn),sizeof(sgx_isv_svn_t));
        Log("\tpce svn : (%s)", ByteArrayToString(buf_pce_svn, sizeof(sgx_isv_svn_t)));
        Log("\txeid: 0x%0lx", p_quote->xeid);
        unsigned char buf_basename[sizeof(sgx_basename_t)];
        memcpy(buf_basename,(unsigned char *)(&p_quote->basename),sizeof(sgx_basename_t));
        Log("\tbasename : (%s)", ByteArrayToString(buf_basename, sizeof(sgx_basename_t)));
        unsigned char buf_reportbody[sizeof(sgx_report_body_t)];
        memcpy(buf_reportbody,(unsigned char *)(&p_quote->report_body),sizeof(sgx_report_body_t));
        Log("\treport body : (%s)", ByteArrayToString(buf_reportbody, sizeof(sgx_report_body_t)));
        Log("\tsignature_len : 0x%0lx", p_quote->signature_len);
        int s_len = p_quote->signature_len;
        unsigned char buf_signature[s_len];
        memcpy(buf_signature,(unsigned char *)(&p_quote->signature),s_len);
        Log("\tsignature : (%s)", ByteArrayToString(buf_signature, s_len));

        Log("\t === report data ===");
        sgx_report_body_t *p_report_body = &p_quote->report_body;
        unsigned char buf_report_data[sizeof(sgx_report_data_t)];
        memcpy(buf_report_data,(unsigned char *)(&p_report_body->report_data),sizeof(sgx_report_data_t));
        Log("\treport data : (%s)", ByteArrayToString(buf_report_data, sizeof(sgx_report_data_t)));

        unsigned char t_buf[sizeof(sgx_quote_t)];
        memcpy(t_buf,(unsigned char *)p_quote,sizeof(sgx_quote_t));
        Log("\tquote content: (%s)", ByteArrayToString(t_buf, sizeof(sgx_quote_t)));
        //@@@@@@@@@@@@@@@@@@ <<<<<<<<<<<<<<<<<<<<< let's ignore all Upper verification <<<<<<<<<<<<<<<<<<<<<<<<<


        // Respond the client with the results of the attestation.
        att_result_msg_size = sizeof(sample_ra_att_result_msg_t);
        Log("=============== sizeof att_result_msg ==============");
        Log("\t%d",att_result_msg_size);

        p_att_result_msg_full = (ra_samp_response_header_t*) malloc(att_result_msg_size + sizeof(ra_samp_response_header_t) + sizeof(validation_result));
        if (!p_att_result_msg_full) {
            Log("Error, out of memory", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        memset(p_att_result_msg_full, 0, att_result_msg_size + sizeof(ra_samp_response_header_t) + sizeof(validation_result));
        p_att_result_msg_full->type = RA_ATT_RESULT;
        p_att_result_msg_full->size = att_result_msg_size;

        if (IAS_QUOTE_OK != attestation_report.status) {
            p_att_result_msg_full->status[0] = 0xFF;
        }

        if (IAS_PSE_OK != attestation_report.pse_status) {
            p_att_result_msg_full->status[1] = 0xFF;
        }

        p_att_result_msg = (sample_ra_att_result_msg_t *)p_att_result_msg_full->body;

        bool isv_policy_passed = true;

        p_att_result_msg->platform_info_blob = attestation_report.info_blob;

        // Generate mac based on the mk key.
        mac_size = sizeof(ias_platform_info_blob_t);
        sample_ret = sample_rijndael128_cmac_msg(&g_sp_db.mk_key,
                     (const uint8_t*)&p_att_result_msg->platform_info_blob,
                     mac_size,
                     &p_att_result_msg->mac);

        if (SAMPLE_SUCCESS != sample_ret) {
            Log("Error, cmac fail", log::error);
            ret = SP_INTERNAL_ERROR;
            break;
        }

        // Generate shared secret and encrypt it with SK, if attestation passed.
        uint8_t aes_gcm_iv[SAMPLE_SP_IV_SIZE] = {0};
        p_att_result_msg->secret.payload_size = MAX_VERIFICATION_RESULT;

        Log("============ IAS STATUS ===========");
        Log("\treport status:%d",attestation_report.status);
        Log("\tpse status   :%d",attestation_report.pse_status);

        if ((IAS_QUOTE_OK == attestation_report.status) &&
                (IAS_PSE_OK == attestation_report.pse_status) &&
                (isv_policy_passed == true)) {
            memset(validation_result, '\0', MAX_VERIFICATION_RESULT);
            validation_result[0] = 6;
            validation_result[1] = 7;

            Log("========== sk key ==========");
            unsigned char skbuf[sizeof(sgx_ec_key_128bit_t)];
            memcpy(skbuf, (unsigned char*)&g_sp_db.sk_key, sizeof(sgx_ec_key_128bit_t));
            Log("\tsp sk_key:%s",ByteArrayToString(skbuf,sizeof(sgx_ec_key_128bit_t)));

            ret = sample_rijndael128GCM_encrypt(&g_sp_db.sk_key,
                                                &validation_result[0],
                                                p_att_result_msg->secret.payload_size,
                                                p_att_result_msg->secret.payload,
                                                &aes_gcm_iv[0],
                                                SAMPLE_SP_IV_SIZE,
                                                NULL,
                                                0,
                                                &p_att_result_msg->secret.payload_tag);
            unsigned char payloadbuf[2];
            memcpy(payloadbuf, (unsigned char*)&p_att_result_msg->secret.payload, 2);
            Log("\tsp payload:%s",ByteArrayToString(payloadbuf,2);

            unsigned char tagbuf[16];
            memcpy(tagbuf, (unsigned char*)&p_att_result_msg->secret.payload_tag, 16);
            Log("\tsp tag:%s",ByteArrayToString(tagbuf,16);

            uint8_t *decrypted = (uint8_t*) malloc(sizeof(uint8_t) * 2);
            sgx_rijndael128GCM_decrypt(&g_sp_db.sk_key,
                                         &p_att_result_msg->secret.payload[0],
                                         p_att_result_msg->secret.payload_size,
                                         decrypted,
                                         &aes_gcm_iv[0],
                                         12,
                                         NULL,
                                         0,
                                         &p_att_result_msg->secret.payload_tag);

            unsigned char decriptbuf[2];
            memcpy(decriptbuf, (unsigned char*)decrypted, 2);
            Log("\tsp decripted:%s",ByteArrayToString(decriptbuf,2));

        }

    } while(0);

    if (ret) {
        SafeFree(p_att_result_msg_full);
        return -1;
    } else {
        att_msg->set_size(att_result_msg_size);

        ias_platform_info_blob_t platform_info_blob = p_att_result_msg->platform_info_blob;
        att_msg->set_epid_group_status(platform_info_blob.sample_epid_group_status);
        att_msg->set_tcb_evaluation_status(platform_info_blob.sample_tcb_evaluation_status);
        att_msg->set_pse_evaluation_status(platform_info_blob.pse_evaluation_status);

        for (int i=0; i<PSVN_SIZE; i++)
            att_msg->add_latest_equivalent_tcb_psvn(platform_info_blob.latest_equivalent_tcb_psvn[i]);

        for (int i=0; i<ISVSVN_SIZE; i++)
            att_msg->add_latest_pse_isvsvn(platform_info_blob.latest_pse_isvsvn[i]);

        for (int i=0; i<PSDA_SVN_SIZE; i++)
            att_msg->add_latest_psda_svn(platform_info_blob.latest_psda_svn[i]);

        for (int i=0; i<GID_SIZE; i++)
            att_msg->add_performance_rekey_gid(platform_info_blob.performance_rekey_gid[i]);

        for (int i=0; i<SAMPLE_NISTP256_KEY_SIZE; i++) {
            att_msg->add_ec_sign256_x(platform_info_blob.signature.x[i]);
            att_msg->add_ec_sign256_y(platform_info_blob.signature.y[i]);
        }

        for (int i=0; i<SAMPLE_MAC_SIZE; i++)
            att_msg->add_mac_smk(p_att_result_msg->mac[i]);

        att_msg->set_result_size(p_att_result_msg->secret.payload_size);

        for (int i=0; i<12; i++)
            att_msg->add_reserved(p_att_result_msg->secret.reserved[i]);

        for (int i=0; i<16; i++)
            att_msg->add_payload_tag(p_att_result_msg->secret.payload_tag[i]);

        for (int i=0; i<p_att_result_msg->secret.payload_size; i++)
            att_msg->add_payload(p_att_result_msg->secret.payload[i]);
        Log("========== ATT SECRET ==========");
        unsigned char secretbuf[sizeof(sp_aes_gcm_data_t)];
        memcpy(secretbuf, &p_att_result_msg->secret, sizeof(sp_aes_gcm_data_t));
        Log("\tatt secret:%s",ByteArrayToString(secretbuf,sizeof(sp_aes_gcm_data_t)));
    }

    return ret;
}

