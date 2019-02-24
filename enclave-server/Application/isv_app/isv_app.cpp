#include <iostream>
#include <unistd.h>

#include "LogBase.h"
#include "sgx_capable.h"
#include "sgx_tseal.h"

using namespace util;

#include "MessageHandler.h"

int Main(int argc, char* argv[]) {
    LogBase::Inst();

    int ret = 0;
    printf("========== msg life is started ==========\n");
    int keymode = 0;
    if(argc > 1) {
        keymode = atoi(argv[1]);
    }

    MessageHandler msg;
    //msg.init(keymode);
    msg.init();
    msg.start();


    return ret;
}


int main( int argc, char **argv ) {
    // enable sgx enclave {{{
    //sgx_cap_get_status();
    printf("Checking if sgx is enabled\n");
    sgx_device_status_t sgx_device_status;
    sgx_status_t sgx_ret = sgx_cap_enable_device(&sgx_device_status);
    switch(sgx_device_status) {
        case SGX_ENABLED: 
            cout<<"sgx has been enabled!"<<endl; 
            break;
        case SGX_DISABLED_REBOOT_REQUIRED: 
            cout<<"sgx disabled reboot required"<<endl; 
            break;
        case SGX_DISABLED_MANUAL_ENABLE: 
            cout<<"sgx disabled manual enable"<<endl; 
            break;
        case SGX_DISABLED_HYPERV_ENABLED: 
            cout<<"sgx disabled hyperv enabled"<<endl; 
            break;
        case SGX_DISABLED_LEGACY_OS: 
            cout<<"sgx disabled legacy os"<<endl; 
            break;
        case SGX_DISABLED_UNSUPPORTED_CPU: 
            cout<<"sgx disabled unsupported cpu"<<endl; 
            break;
        case SGX_DISABLED: 
            cout<<"sgx disabled"<<endl; 
            break;
    }
    switch(sgx_ret) {
        case SGX_SUCCESS: 
            cout<<"sgx success"<<endl; 
            break;
        case SGX_ERROR_INVALID_PARAMETER: 
            cout<<"sgx error invalid parameter"<<endl; 
            break;
        case SGX_ERROR_NO_PRIVILEGE: 
            cout<<"sgx error no privilege"<<endl; 
            break;
        case SGX_ERROR_UNEXPECTED: 
            cout<<"sgx error unexpected"<<endl; 
            break;
    }
    // }}}
    try {
        return Main(argc, argv);
    } catch (std::exception& e) {
        Log("exception: %s", e.what());
    } catch (...) {
        Log("unexpected exception") ;
    }

    return -1;
}







