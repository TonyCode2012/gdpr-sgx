#!/bin/bash

basedir=$(cd `dirname $0`;pwd)
libfile="/home/vdeadmin/errands/sgx/target/libEnclaveBridge.so"
signedfile="/home/vdeadmin/errands/sgx/target/isv_enclave.signed.so"

cp $libfile $basedir/shared/lib
if [ $? -ne 0 ]; then 
    echo "[ERROR] copy lib file failed!" >&2
    exit 1
fi
cp $signedfile $basedir
if [ $? -ne 0 ]; then 
    echo "[ERROR] copy signed file failed!" >&2
    exit 1
fi
$basedir/bin/shutdown.sh
$basedir/bin/startup.sh
tail -f $basedir/logs/catalina.out
