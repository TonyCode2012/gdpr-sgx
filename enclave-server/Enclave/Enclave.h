#ifndef ENCLAVE_H
#define ENCLAVE_H

#include <string>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <deque> 

#include "LogBase.h"
#include "UtilityFunctions.h"
#include "isv_enclave_u.h"

// Needed to call untrusted key exchange library APIs, i.e. sgx_ra_proc_msg2.
#include "sgx_ukey_exchange.h"

// Needed to query extended epid group id.
#include "sgx_uae_service.h"

// Need store sealed data
//#include "sgx_tseal.h"

class Enclave {

public:
    static Enclave* getInstance();

    virtual ~Enclave();
    sgx_status_t createEnclave();
    sgx_ra_context_t createSession(sgx_status_t *status);
    sgx_status_t closeSession(sgx_ra_context_t session_id);
    sgx_enclave_id_t getID();
    sgx_status_t getStatus();
    sgx_ra_context_t getContext();

private:
    Enclave();
    static Enclave *instance;
    const char *enclave_path = "isv_enclave.signed.so";
    sgx_enclave_id_t enclave_id;
    sgx_status_t status;
    //sgx_ra_context_t context;
    std::deque<sgx_ra_context_t> g_session_dq;
};

#endif
