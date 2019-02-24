#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include <string>

using namespace std;

namespace Settings {
	//static int rh_port = 22222;
	static int rh_port = 8080;
	static string rh_host = "localhost";
	
	//static string server_crt = "../cert/real/client.crt"; //certificate for the HTTPS connection between the SP and the App
	//static string server_key = "../cert/real/client.key"; //private key for the HTTPS connection
	static string server_crt = "../cert/tmp/server.crt"; //certificate for the HTTPS connection between the SP and the App
	static string server_key = "../cert/tmp/server.key"; //private key for the HTTPS connection

	static string spid = "4F2229E41A0ECA12BAE6EDB36E87EAFE"; //SPID provided by Intel after registration for the IAS service
	static const char *ias_crt = "../cert/real/client.pem"; //location of the certificate send to Intel when registring for the IAS
	//static string ias_url = "https://test-as.sgx.trustedservices.intel.com:443/attestation/sgx/v1/";
	static string ias_url = "https://test-as.sgx.trustedservices.intel.com:443/attestation/sgx/v3/";
    static const char *ec_pri_key_path = "./ec256_private_key";
    static const char *ec_pub_key_path = "./ec256_public_key";
    static const char *ec_pri_key_path_u = "./ec256_private_key_unseal";
    static const char *ec_pub_key_path_u = "./ec256_public_key_unseal";
    static const char *ec_pub_key_path_server = "./ec256_public_key";
    static const char *ec_pri_key_path_server = "./ec256_private_key";
}

#endif
