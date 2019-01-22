######## SGX SDK Settings ########
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= SIM
SGX_ARCH ?= x64

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
        SGX_COMMON_CFLAGS += -O0 -g
else
        SGX_COMMON_CFLAGS += -O2
endif

ifeq ($(SUPPLIED_KEY_DERIVATION), 1)
        SGX_COMMON_CFLAGS += -DSUPPLIED_KEY_DERIVATION
endif

######## App Settings ########

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif


App_Cpp_Files := MessageHandler/MessageHandler.cpp Util/LogBase.cpp GoogleMessages/Messages.pb.cpp \
Util/UtilityFunctions.cpp Enclave/Enclave.cpp Util/Base64.cpp

App_Include_Paths := -IUtil -I$(SGX_SDK)/include -Iheaders -IMessageHandler -IEnclave -IGoogleMessages -I/usr/local/include -IEnclave \

App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(App_Include_Paths)

# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
	App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
	App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

App_Cpp_Flags := $(App_C_Flags) -std=c++11 -DEnableServer
App_Link_Flags := -fPIC -shared $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -L. -lsgx_ukey_exchange -lpthread -Wl,-rpath=$(CURDIR)/sample_libcrypto -Wl,-rpath=$(CURDIR) -llog4cpp -lboost_system -lssl -lcrypto -lboost_thread -lprotobuf -L /usr/local/lib -ljsoncpp
Java_Link_Flags := -I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux 
#Target_Link_Flags := -fPIC -shared $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -L. -lsgx_ukey_exchange -lpthread -Wl,-rpath=$(CURDIR)/sample_libcrypto -Wl,-rpath=$(CURDIR) -llog4cpp -lboost_system -lssl -lcrypto -lboost_thread -lprotobuf -L /usr/local/lib -ljsoncpp

Target_Cpp_Objects := GoogleMessages/Messages.pb.o Enclave/isv_enclave_u.o Enclave/Enclave.o Enclave/EnclaveBridge.o MessageHandler/MessageHandler.o Util/LogBase.o Util/UtilityFunctions.o Util/Base64.o

ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif

App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)

Target_Name := libEnclaveBridge.so
Enclave_OBJ := Enclave/Enclave.o
MessageHandler_OBJ := MessageHandler/MessageHandler.o
Util_OBJ := Util/Base64.o Util/LogBase.o Util/UtilityFunctions.o


######## Enclave Settings ########
ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif
Crypto_Library_Name := sgx_tcrypto

Enclave_Cpp_Files := isv_enclave/isv_enclave.cpp
Enclave_Include_Paths := -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport -I$(SGX_SDK)/include/crypto_px/include -IEnclave/

Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) -std=c++11 -nostdinc++

# To generate a proper enclave, it is recommended to follow below guideline to link the trusted libraries:
#    1. Link sgx_trts with the `--whole-archive' and `--no-whole-archive' options,
#       so that the whole content of trts is included in the enclave.
#    2. For other libraries, you just need to pull the required symbols.
#       Use `--start-group' and `--end-group' to link these libraries.
# Do NOT move the libraries linked with `--start-group' and `--end-group' within `--whole-archive' and `--no-whole-archive' options.
# Otherwise, you may get some undesirable errors.
Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tstdcxx -lsgx_tkey_exchange -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=isv_enclave/isv_enclave.lds

Enclave_Cpp_Objects := $(Enclave_Cpp_Files:.cpp=.o)

Enclave_Name := isv_enclave.so
Signed_Enclave_Name := isv_enclave.signed.so
Enclave_Config_File := isv_enclave/isv_enclave.config.xml

ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif


.PHONY: all run

ifeq ($(Build_Mode), HW_RELEASE)
all: $(Target_Name) $(Enclave_Name)
	@echo "The project has been built in release hardware mode."
	@echo "Please sign the $(Enclave_Name) first with your signing key before you run the $(Target_Name) to launch and access the enclave."
	@echo "To sign the enclave use the command:"
	@echo "   $(SGX_ENCLAVE_SIGNER) sign -key <your key> -enclave $(Enclave_Name) -out <$(Signed_Enclave_Name)> -config $(Enclave_Config_File)"
	@echo "You can also sign the enclave using an external signing tool."
	@echo "To build the project in simulation mode set SGX_MODE=SIM. To build the project in prerelease mode set SGX_PRERELEASE=1 and SGX_MODE=HW."
else
#all: $(Target_Name) $(MessageHandler_OBJ) $(Util_OBJ) $(Enclave_OBJ) $(Signed_Enclave_Name)
all: $(Target_Name) $(Signed_Enclave_Name)
endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/$(Target_Name) 	
	@echo "RUN  =>  $(Target_Name) [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif


######## App Objects ########

Enclave/isv_enclave_u.c: $(SGX_EDGER8R) isv_enclave/isv_enclave.edl
	@cd Enclave && $(SGX_EDGER8R) --untrusted ../isv_enclave/isv_enclave.edl --search-path ../isv_enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

Enclave/isv_enclave_u.o: Enclave/isv_enclave_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

GoogleMessages/Messages.pb.o: GoogleMessages/Messages.pb.cc
	@$(CXX) -c $< -fPIC -shared -std=c++11 -lprotobuf -o $@
	@echo "CXX  <=  $<"

MessageHandler/%.o: MessageHandler/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "app cpp flags <= $(App_Cpp_Flags)"
	@echo "CXX  <=  $<"

Util/%.o: Util/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

Enclave/%.o: Enclave/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

#Enclave/EnclaveBridge.o: EnclaveBridge.c
Enclave/EnclaveBridge.o: com_sgxtrial_yyy_EnclaveBridge.c
	@$(CXX) $(App_Cpp_Flags) $(Java_Link_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Target_Name): $(Target_Cpp_Objects) 
	@$(CXX) $^ -o $@ $(App_Link_Flags) $(Java_Link_Flags) /opt/intel/sgxsdk/lib64/libsgx_capable.so
	@echo "LINK >= $@"


######## Enclave Objects ########

isv_enclave/isv_enclave_t.c: $(SGX_EDGER8R) isv_enclave/isv_enclave.edl
	@cd isv_enclave && $(SGX_EDGER8R) --trusted ../isv_enclave/isv_enclave.edl --search-path ../isv_enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

isv_enclave/isv_enclave_t.o: isv_enclave/isv_enclave_t.c
	@$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

isv_enclave/%.o: isv_enclave/%.cpp
	@$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Enclave_Name): isv_enclave/isv_enclave_t.o $(Enclave_Cpp_Objects)
	@$(CXX) $^ -o $@ $(Enclave_Link_Flags)
	@echo "LINK =>  $@"

$(Signed_Enclave_Name): $(Enclave_Name)
	@$(SGX_ENCLAVE_SIGNER) sign -key isv_enclave/isv_enclave_private.pem -enclave $(Enclave_Name) -out $@ -config $(Enclave_Config_File)
	@echo "SIGN =>  $@"

.PHONY: clean

clean:
	@rm -f $(Target_Name) $(Enclave_Name) $(Signed_Enclave_Name) $(App_Cpp_Objects) Enclave/isv_enclave_u.* $(Enclave_Cpp_Objects) isv_enclave/isv_enclave_t.* libservice_provider.* $(ServiceProvider_Cpp_Objects)
