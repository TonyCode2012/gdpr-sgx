public class EnclaveBridge {
    static {
        System.loadLibrary("EnclaveBridge");
    }

    public long messageHandlerOBJ;

    public EnclaveBridge() {
        messageHandlerOBJ = createMessageHandlerOBJ();
    }

    public native long createMessageHandlerOBJ();
    public native byte[] handleMessages(long msgHandlerAddr, byte[] msg, String[] data);

}
