#!/bin/bash
usage()
{
cat << EOF
    ./genApp.sh [option] -d <maketype>
        options:
                -h: use hardware mode to build
                -s: use simulation mode to build
                -d: make type which can only be server, app and clean
EOF
}

function verbose()
{
    local type=$1
    local info=$2
    local tips=$3
    local color=$GREEN
    local time=`date "+%Y/%m/%d %T.%3N"`
    if [ x"$tips" !=  x"" ]; then tips=H; fi
    case $type in
        INFO)   eval color=\$${tips}GREEN;;
        WARN)   eval color=\$${tips}YELLOW;;
        ERROR)  eval color=\$${tips}RED;;
        ?)      echo "[WARN] wrong color type"
    esac
    echo -e "${color}$time [$type] $info${NC}"
}

function makeServer()
{
    verbose INFO "Making service provider..."
    cd $SerDir
    make clean
    make
    if [ $? -ne 0 ]; then
        verbose ERROR "Make serviceProvider failed!" h
        exit 1
    fi
}

function makeApp()
{
    verbose INFO "Making enclave application..."
    cd $AppDir
    make clean
    make SGX_MODE=$runtype SGX_PRERELEASE=1
    if [ $? -ne 0 ]; then
        verbose ERROR "Make application failed!" h
        exit 1
    fi
}

function makeClean() 
{
    cd $SerDir
    make clean
    cd $AppDir
    make clean
}


########## MAIN BODY ##########
basedir=`dirname $0`
basedir=`cd $basedir;pwd`
AppDir=$basedir/../Application
SerDir=$basedir/../ServiceProvider

GREEN='\033[0;32m'
HGREEN='\033[1;32m'
YELLOW='\033[0;33m'
HYELLOW='\033[1;33m'
RED='\033[0;31m'
HRED='\033[1;31m'
NC='\033[0m'

runtype="HW"
maketype=""

while getopts 'hsd:' OPT; do
    case $OPT in
        h)
            runtype="HW";;
        s)
            runtype="SIM";;
        d)
            maketype=$OPTARG;;
        h)
            usage;;
        ?)
            usage; exit 1 ;;
    esac
done

verbose INFO "Type -h to get help info"

if [ x"$runtype" = x"" ]; then
    verbose INFO "Use HW mode to build app. If you don't have sgx hardware, please use '-s' to set simulation mode."
fi

if [ x"$maketype" = x"server" ]; then
    makeServer
elif [ x"$maketype" = x"app" ]; then
    makeApp
elif [ x"$maketype" = x"clean" ]; then
    makeClean
else
    makeServer
    makeApp
fi
