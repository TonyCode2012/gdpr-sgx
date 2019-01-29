package com.sgxtrial.yyy;


import java.util.logging.*;
import javax.websocket.*;
import javax.websocket.server.ServerEndpoint;

public class EnclaveBridge {
    private static final Logger logger = Logger.getLogger("WebSocketServer");
    static {
        System.loadLibrary("EnclaveBridge");
    }

    private long messageHandlerOBJ;

    public EnclaveBridge() {
        try {
            messageHandlerOBJ = createMessageHandlerOBJ();
        } catch(Throwable t){
            t.printStackTrace();
        }
    }

    private native long createMessageHandlerOBJ();
    private native byte[] handleMessages(long msgHandlerAddr, byte[] msg, String[] data);

    //public byte[] callEnclave(byte[] msg, String[] arr) {
    public byte[] callEnclave(byte[] msg) {
        String[] data = new String[2];
        byte[] bytes = handleMessages(messageHandlerOBJ, msg, data);
        if(data[0] != null) {
            //arr[0] = data[0];
            //arr[1] = data[1];
		    SMSSender sender = new SMSSender();
		    sender.sendMsg(arr[0],arr[1]);
            logger.info("parameter1:"+arr[0]);
            logger.info("parameter2:"+arr[1]);
        }
        return bytes;
    }

}
