#include "com_sgxtrial_yyy_EnclaveBridge.h"

/*
char* jstringToChar(JNIEnv* env, jstring jstr) {
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("GB2312");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char*) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}
*/

jstring charTojstring(JNIEnv* env, const char* pat) {
    //定义java String类 strClass
    jclass strClass = (env)->FindClass("Ljava/lang/String;");
    //获取String(byte[],String)的构造器,用于将本地byte[]数组转换为一个新String
    jmethodID ctorID = (env)->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
    //建立byte数组
    jbyteArray bytes = (env)->NewByteArray(strlen(pat));
    //将char* 转换为byte数组
    (env)->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*) pat);
    // 设置String, 保存语言类型,用于byte数组转换至String时的参数
    jstring encoding = (env)->NewStringUTF("GB2312");
    //将byte数组转换为java String,并输出
    return (jstring) (env)->NewObject(strClass, ctorID, bytes, encoding);
}

/*
string jstring2string(JNIEnv *env, jstring jStr) {
    if (!jStr)
        return "";

    const jclass stringClass = env->GetObjectClass(jStr);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    std::string ret = std::string((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);
    return ret;
}
*/

jbyteArray string2jbyteArray(JNIEnv *env, string str) {
    jbyteArray arr = env->NewByteArray(str.length());
    env->SetByteArrayRegion(arr, 0, str.length(), (jbyte*)str.c_str());
    return arr;
}


//=============== MAIN FUNCTION ===============//

JNIEXPORT jlong JNICALL Java_com_sgxtrial_yyy_EnclaveBridge_createMessageHandlerOBJ(JNIEnv *env, jobject obj)
{
    return (jlong) new MessageHandler();
}

JNIEXPORT jbyteArray JNICALL Java_com_sgxtrial_yyy_EnclaveBridge_handleMessages(JNIEnv *env, jobject obj, jlong msgHandlerAddr, jbyteArray msg, jobjectArray data)
{
    printf("========== receive serivce provider message ==========\n");

    /*
    jbyte *bytes = env->GetByteArrayElements(msg, 0);
    int chars_len = env->GetArrayLength(msg);
    printf("===== msg:(%d)\n",chars_len);
    char *chars = (char*)malloc(chars_len + 1);
    memset(chars,0,chars_len + 1);
    memcpy(chars, bytes, chars_len);
    chars[chars_len] = 0;
    env->ReleaseByteArrayElements(msg, bytes, 0);
    */
    int len = env->GetArrayLength (msg);
    unsigned char* buf = new unsigned char[len];
    env->GetByteArrayRegion (msg, 0, len, reinterpret_cast<jbyte*>(buf)); 
 
    /*
    unsigned char *p_data[2];
    p_data[0] = (unsigned char*)malloc(32);
    p_data[1] = (unsigned char*)malloc(32);
    memset(p_data[0],0,32);
    memset(p_data[1],0,32);
    */
    //unsigned char *p_data = (unsigned char*)malloc(32);
    //memset(p_data,0,32);
    int size = 0;
    //string res = ((MessageHandler*)msgHandlerAddr)->handleMessages(buf, len, p_data, &size);
    string res = ((MessageHandler*)msgHandlerAddr)->handleMessages(buf, len, NULL, &size);
    if(size != 0) {
        printf("========================================================================\n");
        /*
        int tmpsize = 32;
        char *p1 = (char*)malloc(tmpsize);
        memset(p1,0,tmpsize);
        memcpy(p1,p_data,11);
        string phoneNum(p1);
        memset(p1,0,tmpsize);
        //printf("%s\n",p1);
        memcpy(p1,p_data+11,7);
        string sms(p1);
        //printf("%s\n",p1);
        */
        env->SetObjectArrayElement(data, 0, charTojstring(env, "15021128363"));
        env->SetObjectArrayElement(data, 1, charTojstring(env, "test test test"));
        //env->SetObjectArrayElement(data, 0, charTojstring(env, phoneNum.c_str()));
        //env->SetObjectArrayElement(data, 1, charTojstring(env, sms.c_str()));
        //free(p1);
    }
    fflush(stdout);
    //free(buf);
    //free(p_data);
    return string2jbyteArray(env, res);

}
