import React from "react";
import protobuf from "protobufjs";
import proto from "../utils/demo/Messages.proto";
import registry from "../utils/demo/messageRegistry";
import { buf2hexString } from "../utils/hexHelpers"
import {
  RA_VERIFICATION, 
  RA_MSG0, 
  RA_MSG1,
  RA_MSG3, 
  RA_MSG2, 
  RA_ATT_RESULT,
  RA_APP_ATT_OK,
  PHONE_REG,
  PHONE_RES
} from "../metadata/messageTypes";

let PROTO, WEB_SOCKET;

/**
 * @desc load .proto file
 */
protobuf.load(proto)
  .then(root => {
    PROTO = root;
  }).catch(error => {
    console.log("error", error);
  });


class GDPRDemo extends React.Component {
  constructor() {
    super();
    this.setupWebSocket();
  }

  setupWebSocket() {
    if (WEB_SOCKET) return;

    WEB_SOCKET = new WebSocket("ws://localhost:8080/com.sgxtrial/websocketendpoint");

    WEB_SOCKET.onopen = () => {
      console.log("Connection open ...");

      const initMsg = this.assemble(RA_VERIFICATION);
      WEB_SOCKET.send(initMsg);
      console.log("======== Initial message sent ========\n\n\n\n\n");
    };

    WEB_SOCKET.onmessage = evt => {
      const reader = new FileReader();
      reader.readAsArrayBuffer(evt.data);

      reader.onload = () => {
        const ecMsg = new Uint8Array(reader.result);
        console.log("onmessage", ecMsg)

        if (ecMsg) {
          this.handleMessage(ecMsg);

        } else {
          WEB_SOCKET.close();
        }
      }
    };

    WEB_SOCKET.onclose = () => {
      console.log("Connection closing...");
    };
  }

  assemble(type, ecPublicKey = {}) {
    const { defName, getPayload, fieldName } = registry[type];

    /**
     * @desc assemble wrapped message
     */
    const wrappedMsgDef = PROTO.lookupType(`Messages.${defName}`);
    const wrappedPayload = getPayload(ecPublicKey);
    const wrappedMsg = wrappedMsgDef.create(wrappedPayload);

    /**
     * @desc assemble all-in-one message
     */
    const allInOneMsgDef = PROTO.lookupType("Messages.AllInOneMessage");
    const allInOnePayload = {
      type,
      [fieldName]: wrappedMsg
    };
    const allInOneMsg = allInOneMsgDef.create(allInOnePayload);

    /**
     * @desc encode message and send to web socket
     */
    const buffer = allInOneMsgDef.encode(allInOneMsg).finish();
    console.log("Message to sent:", allInOneMsg);
    console.log("Buffer to sent:", buffer);

    return buffer;
  }


  disassemble(buffer) {
    const msgDef = PROTO.lookupType(`Messages.AllInOneMessage`);
    const message = msgDef.decode(buffer);
    return message;
  }


  handleMessage(buffer) {
    const message = this.disassemble(buffer);

    const { type } = message;
    const { defName } = registry[type];

    console.log("======== ", defName, "received ========");
    console.log("Buffer received:", buffer);
    console.log("Message Received:", message);

    let msgToSent;

    switch (type) {
      case RA_MSG0:
        msgToSent = this.assemble(RA_MSG0);
        break;

      case RA_MSG1:
        const { msg1 } = message;
        const { GaX, GaY } = msg1;
        const ecPublicKey = {
          X: GaX,
          Y: GaY
        }
        msgToSent = this.assemble(RA_MSG2, ecPublicKey);
        break;

      case RA_MSG3:
        msgToSent = this.assemble(RA_ATT_RESULT);
        break;

      case RA_APP_ATT_OK:
        msgToSent = this.assemble(PHONE_REG);
        break;

      case PHONE_RES:
        const { resMsg } = message;
        const { userID } = resMsg;
        console.log("==========userid:",buf2hexString(userID),"==========");
        break;

      default:
        break;
    }

    if (!msgToSent) return;

    WEB_SOCKET.send(msgToSent);
    console.log("======== Message sent ========\n\n\n\n\n");
  }


  render() {
    return (
      <div className="text-center">
        <h5>Demo</h5>
      </div>
    );
  }
}


export default GDPRDemo;
