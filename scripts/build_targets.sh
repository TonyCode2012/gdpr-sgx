#!/bin/bash
usage()
{
cat << EOF
    ./build_targets.sh [option]
        options:
                -s: build sdk
                -r: build remote attestation
                -j: build enclave cpp jni library
                -m: remote attestation build mode, could just be HW/SIM.
                    HW is hardware mod while SIM is simulation mode.
                    Default value is HW.
EOF
}

verbose()
{
    local type="$1"
    local info="$2"
    local time=`date "+%Y/%m/%d %T.%3N"`
    if [ x"$type" = x"ERROR" ]; then
        echo -e "${RED}$time [$type] $info${NC}" >&2 
        return
    fi
    echo -e "${GREEN}$time [$type] $info${NC}"
}

function setTestPath()
{
    verbose INFO "Set sdk and attestation path to simple project path"
    sdkdir=$homedir/simple-sdk
    attestdir=$homedir/simple-remoteattestation
}

function checkPrerequisite()
{
    if ! echo "$LD_LIBRARY_PATH" | grep "libsample_libcrypto.so"; then
        verbose INFO "Add sample libcrypto diretory"
        export LD_LIBRARY_PATH=$libcryptodir:$LD_LIBRARY_PATH
    fi
}

function buildSDK()
{
    cd $sdkdir
    verbose INFO "Building SGX SDK..."
    ./scripts/build_sdk.sh
    if [ $? -ne 0 ]; then
        verbose ERROR "Build SGX SDK failed!"
        cd -
        exit 1
    fi
    cd -
}

function buildEnclaveJNI()
{
    cd $jnidir
    verbose INFO "Building jni library..."
    ./scripts/build.sh -r -d "$targetdir"
    cd -
}

function buildRemoteAttestation()
{
    checkPrerequisite
    cd $attestdir
    if [ x"$attestMode" = x"HW" ]; then
        verbose INFO "Build attestation app with hardware mode..."
        ./scripts/genApp.sh -h
    elif [ x"$attestMode" = x"SIM" ]; then
        verbose INFO "Build attestation app with software mode..."
        ./scripts/genApp.sh -s
    else
        verbose ERROR "Please indicate correct type for building remote attestation code(HW/SIM)!" >&2
        cd -
        return
    fi

    local sgx_lib_path="/opt/intel/sgxsdk/sdk_libs"
    if ! grep "$sgx_lib_path" ~/.bashrc &>/dev/null; then
        echo "export LD_LIBRARY_PATH=$sgx_lib_path:$LD_LIBRARY_PATH" >> ~/.bashrc
        verbose INFO "<<< Attention!!! >>>\tIf tip \"libsgx_xxx.so: No such file or directory\"\n\
            \tPlease run 'source ~/.bashrc' to refresh LD_LIBRARY_PATH\n<<< Attention!!! >>>"
    fi
    cd -
}

############### MAIN BODY ###############

basedir=`dirname $0`
basedir=`cd $basedir; pwd`
homedir=`cd $basedir/..; pwd`
sdkdir=$homedir/linux-sgx
attestdir=$homedir/linux-sgx-remoteattestation
jnidir=$homedir/EnclaveCPP
targetdir=$homedir/target
libcryptodir=$attestdir/ServiceProvider/sample_libcrypto

attestMode="HW"
doBuildSDK=no
doBuildAttest=no
doBuildJNI=no
test_simple=no

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

# clean previous packages
if [ -e "$targetdir" ]; then
    rm -rf $targetdir
fi
mkdir -p $targetdir || { verbose ERROR "Create $targetdir failed!"; exit 1; }

while getopts "tsrjhm:" OPT; do
    case $OPT in
        s)
            doBuildSDK="yes"
            ;;
        r)
            doBuildAttest="yes"
            ;;
        j)
            doBuildJNI="yes"
            ;;
        m)
            attestMode=$OPTARG
            ;;
        t)
            if ! setTestPath; then exit 1; fi
            ;;
        h)
            usage;;
        ?)
            usage; exit 1;;
    esac
done

if [ x"$doBuildSDK" = x"no" -a x"$doBuildAttest" = x"no" -a x"$doBuildJNI" = x"no" ]; then
    usage
    exit 1
fi

if [ x"$doBuildSDK" = x"yes" ]; then
    buildSDK
fi

if [ x"$doBuildAttest" = x"yes" ]; then
    buildRemoteAttestation
fi

if [ x"$doBuildJNI" = x"yes" ]; then
    buildEnclaveJNI
fi

