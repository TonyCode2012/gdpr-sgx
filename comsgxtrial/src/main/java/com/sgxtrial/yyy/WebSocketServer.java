package com.sgxtrial.yyy;

import com.google.protobuf.InvalidProtocolBufferException;
import java.util.logging.*;
import javax.websocket.*;
import javax.websocket.server.ServerEndpoint;
import java.nio.ByteBuffer;


@ServerEndpoint("/websocketendpoint")
public class WebSocketServer {
    private static final Logger logger = Logger.getLogger("WebSocketServer");

    private int messageSeq = 0;

    @OnOpen
    public void onOpen(){
        logger.info("websocket connected." + Thread.currentThread().toString());
    }

    @OnClose
    public void onClose(){
        logger.info("websocket closed");
    }

    @OnMessage
    public void onMessage(byte[] message, Session session){
        logger.info("receive msg with thread: " + Thread.currentThread().toString());
        logger.info("js msg:" + message.toString());
        logger.info("js msg lenght:" + message.length);
        byte[] enclaveResults = EnclaveThreadManager.getInstance().getBridge().callEnclave(message);
        logger.info("enclave msg:" + enclaveResults.toString());
        try {
            session.getBasicRemote().sendBinary(ByteBuffer.wrap(enclaveResults));
        } catch (java.io.IOException e) {
            logger.info("error occured when send msg back to js " + e.toString());
        }
    }

    @OnError
    public void onError(Throwable e){
        logger.info(e.toString());
    }


}
