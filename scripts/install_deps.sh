#!/bin/bash
usage()
{
cat << EOF
    ./install_deps.sh [option]
        options:
                -s: install sdk dependencies
                -r: install remote attestation dependencies
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

function apply_pkg_result()
{
    local ret=$1
    local pkg=$2
    if [ $ret -ne 0 ]; then
        verbose ERROR "Install $pkd failed!"
        exit 1
    fi
}

function install_attest_deps()
{
    verbose INFO "Installing remote attestation dependencies..."
    sudo apt-get install libboost-thread-dev libboost-system-dev curl libcurl4-openssl-dev libssl-dev liblog4cpp5-dev libjsoncpp-dev
    apply_pkg_result $? "common tools"
    sudo dpkg -i $lib9v5pkg
    apply_pkg_result $? "$lib9v5pkg"
    sudo dpkg -i $liblite9v5pkg
    apply_pkg_result $? "$liblite9v5pkg"
    sudo dpkg -i $libdevpkg
    apply_pkg_result $? "$libdevpkg"
    sudo dpkg -i $libc9v5pkg
    apply_pkg_result $? "$libc9v5pkg"
    sudo dpkg -i $compilerpkg
    apply_pkg_result $? "$compilerpkg"

    sudo apt-get install mysql-server
    sudo mysql_secure_installation
    sudo apt-get install libmysqld-dev
}

function install_sdk_deps()
{
    local driver_url="https://download.01.org/intel-sgx/linux-2.3.1/ubuntu16.04/sgx_linux_x64_driver_4d69b9c.bin"
    local psw_url="https://download.01.org/intel-sgx/linux-2.3.1/ubuntu16.04/libsgx-enclave-common_2.3.101.46683-1_amd64.deb"
    local driver_file=`basename $driver_url`
    local psw_file=`basename $psw_url`

    verbose INFO "Installing SGX SDK dependencies..."
    verbose INFO "===== Installing SGX SDK tools..."
    sudo apt-get install build-essential ocaml automake autoconf libtool wget python libssl-dev libcurl4-openssl-dev
    verbose INFO "===== Installing SGX Driver..."
    wget $driver_url || { verbose ERROR "Get SDK Driver failed!"; exit 1; }
    sudo chmod +x $driver_file && sudo ./$driver_file || { verbose ERROR "Install SGX Driver failed!"; exit 1; }
    verbose INFO "===== Installing SGX PSW..."
    wget $psw_url || { verbose ERROR "Get SGX PSW failed!"; exit 1; }
    sudo dpkg -i $psw_file || { verbose ERROR "Install SGX PSW failed!"; exit 1; }
    
    rm $driver_file $psw_file

    verbose INFO "Downloading prebuilt binaries to prebuilt folder..."
    cd $sdkdir
    ./download_prebuilt.sh
    cd -
}

function install_web_deps()
{
    local boost_url="https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz"
    local boost_pkg=`basename $boost_url`
    local boost_dir=${boost_pkg%%\.*}
    cd $basedir/..
    wget $boost_url
    tar -xvf $boost_pkg
    cd $boost_dir
    sudo apt --purge remove libboost-dev
    sudo apt --purge remove libboost-all-dev
    sudo rm -rf /usr/lib/libboost_*
    sudo rm -rf /usr/include/boost
    ./bootstrap.sh
    sudo ./b2 install -j4
    sudo echo '/usr/local/lib' > /etc/ld.so.conf.d/boost.conf
    sudo ldconfig
    cd $basedir
    cat /usr/local/include/boost/version.hpp | grep "BOOST_LIB_VERSION"
}

############### MAIN BODY ###############

HOSTNAME=`hostname`
basedir=`dirname $0`
basedir=`cd $basedir; pwd`
sdkdir=$basedir/../linux-sgx
sourcedir=$basedir/../sources
sourcedir=`cd $sourcedir;pwd`
#boostdir=$basedir/../
#boostdir=`cd $boostdir;pwd`

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

lib9v5pkg=$sourcedir/`ls $sourcedir | grep "f9v5" | grep "2.6.1"`
liblite9v5pkg=$sourcedir/`ls $sourcedir | grep "\-lite9v5" | grep "2.6.1"`
libdevpkg=$sourcedir/`ls $sourcedir | grep "\-dev" | grep "2.6.1"`
libc9v5pkg=$sourcedir/`ls $sourcedir | grep "libprotoc9v5" | grep "2.6.1"`
compilerpkg=$sourcedir/`ls $sourcedir | grep "compiler" | grep "2.6.1"`

installed="no"

while getopts "rs" OPT; do
    case $OPT in
        r)
            install_attest_deps
            installed="yes"
            ;;
        s)
            install_sdk_deps
            installed="yes"
            ;;
        b)
            install_web_deps
            installed="yes"
            ;;
        ?)
            usage
            ;;
    esac
done

if [ x"$installed" = x"no" ]; then usage ; fi
