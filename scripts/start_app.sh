#!/bin/bash
function usage()
{
cat << EOF
    ./start_app.sh [option]
        -a: rebuild and restart all components
        -n: restart web ui
        -s: rebuild enclave
        -m: enclave build mode, default is SIM mode
        -W: rebuild and restart web server
        -w: restart web server
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

function checkJob()
{
    [[ x"$f_rebuild_enclave" = x"false" ]] && [[ x"$f_restart_ui" = x"false" ]] && \
        [[ x"$f_restart_web" = x"false" ]] && [[ x"$f_rebuild_web" = x"false" ]] && \
        { verbose ERROR "NOT A VALID USE!" h; usage; exit 1; }
}

function setAllTrue()
{
    f_rebuild_enclave=true
    f_restart_ui=true
    f_restart_web=true
    f_rebuild_web=true
}

function rebuild_enclave()
{
    cd $sgxdir
    ./scripts/build.sh -m $sgx_mode
    if [ $? -ne 0 ]; then
        verbose ERROR "rebuild enclave jni lib failed!" h
        exit 1
    fi
    cp $jnilibName $tomcatdir/shared/lib
    if [ $? -ne 0 ]; then
        verbose ERROR "cp jni library failed!" h
        exit 1
    fi
    cp $jniSignedName $tomcatdir
    if [ $? -ne 0 ]; then
        verbose ERROR "cp jni signed lib failed!" h
        exit 1
    fi
    cd -
}

function restart_ui()
{
    cd $uidir
    npm run client
    if [ $? -ne 0 ]; then
        verbose ERROR "start ui failed!" h
        exit 1
    fi
    cd -
}

function rebuild_web()
{
    cd $webdir
    mvn package
    if [ $? -ne 0 ]; then
        verbose ERROR "build web server war failed!" h
        exit 1
    fi
    cp target/$webwarName $tomcatdir/webapps
    if [ $? -ne 0 ]; then
        verbose ERROR "copy war package to tomcat failed!" h
        exit 1
    fi
    cd -
}

function restart_web()
{
    cd $tomcatdir
    ./bin/shutdown.sh
    ./bin/startup.sh
    tail -f logs/catalina.out
}


############### MAIN BODY ###############


basedir=$(cd `dirname $0`;pwd)
webdir=$basedir/../comsgxtrial
uidir=$basedir/../broswer-application
sgxdir=$basedir/../EnclaveCPP
tomcatdir=$basedir/../apache-tomcat-9.0.14
webwarName="com.sgxtrial.war"
jnilibName="libEnclaveBridge.so"
jniSignedName="isv_enclave.signed.so"
sgx_mode=SIM

GREEN='\033[0;32m'
HGREEN='\033[1;32m'
YELLOW='\033[0;33m'
HYELLOW='\033[1;33m'
RED='\033[0;31m'
HRED='\033[1;31m'
NC='\033[0m'

f_rebuild_enclave=false
f_restart_ui=false
f_restart_web=false
f_rebuild_web=false

while getopts "awWnsm:" OPT; do
    case $OPT in
        a)
            setAllTrue
            ;; 
        w)
            f_restart_web=true
            ;;
        W)
            f_restart_web=true
            f_rebuild_web=true
            ;;
        n)
            f_restart_ui=true
            ;;
        s)
            f_rebuild_enclave=true
            ;;
        m)
            sgx_mode=$OPTARG
            if [ x"$sgx_mode" != x"SIM" -a x"$sgx_mode" != x"HW" ]; then
                verbose WARN "given sgx build mode is not valid, use SIM mode!" h
                sgx_mode=SIM
            fi
            ;;
        ?)
            usage
            exit 1
            ;;
   esac
done

checkJob

if [ x"$f_rebuild_enclave" = x"true" ]; then
    rebuild_enclave
fi

if [ x"$f_restart_ui" = x"true" ]; then
    restart_ui
fi

if [ x"$f_rebuild_web" = x"true" ]; then
    rebuild_web
fi

if [ x"$f_restart_web" = x"true" ]; then
    restart_web
fi
