import React from "react";
import protobuf from "protobufjs";
import { Row, Col, Input, Alert } from "reactstrap";

import registry from "../../utils/messages";
import proto from "../../utils/messages/Messages.proto";
import { buf2hexString } from "../../utils/hexHelpers";
import {
  RA_VERIFICATION,
  RA_MSG0,
  RA_MSG1,
  RA_MSG3,
  RA_MSG2,
  RA_ATT_RESULT,
  RA_APP_ATT_OK,
  PHONE_REG,
  PIN_CODE_TO,
  PIN_CODE_BACK,
  PHONE_REG_RES,
  SMS_SEND,
  SMS_RES
} from "../../metadata/messageTypes";

let PROTO, WEB_SOCKET;

/**
 * @desc load .proto file
 */
protobuf
  .load(proto)
  .then(root => {
    PROTO = root;
  })
  .catch(error => {
    console.log("error", error);
  });

const origin = {
  phone: "",
  pinCode: "",
  userID: "",
  content: "",
  alert: "",
  alertType: ""
};

class GDPRDemo extends React.Component {
  constructor() {
    super();
    this.state = { ...origin };
    this.session_id = 4294967295;

    this.handleOnChange = this.handleOnChange.bind(this);
    this.handleRegister = this.handleRegister.bind(this);
  }

  setupWebSocket() {
    const serverUrl = `ws://${window.location.hostname}:8080`;
    WEB_SOCKET = new WebSocket(serverUrl);

    WEB_SOCKET.onopen = () => {
      console.log("Connection open ...");

      const { end } = this.props.match.params;
      const msgToSent =
        end === "personal"
          ? this.assemble(RA_VERIFICATION)
          : this.assemble(SMS_SEND, this.state);

      WEB_SOCKET.send(msgToSent);
      console.log("======== Initial message sent ========\n\n\n\n\n");
    };

    WEB_SOCKET.onmessage = evt => {
      const reader = new FileReader();
      reader.readAsArrayBuffer(evt.data);

      reader.onload = () => {
        const ecMsg = new Uint8Array(reader.result);

        if (ecMsg) {
          this.handleMessage(ecMsg);
        } else {
          WEB_SOCKET.close();
        }
      };
    };

    WEB_SOCKET.onclose = () => {
      console.log("Connection closing...");

      this.setState({
        alertType: "warning",
        alert: this.state.alert || "Unknown error happened. Please try later."
      });
    };
  }

  assemble(type, param = {}) {
    const { defName, getPayload, fieldName } = registry[type];

    /**
     * @desc assemble wrapped message
     */
    const wrappedMsgDef = PROTO.lookupType(`Messages.${defName}`);
    const wrappedPayload = getPayload(param);
    const wrappedMsg = wrappedMsgDef.create(wrappedPayload);

    /**
     * @desc assemble all-in-one message
     */
    const allInOneMsgDef = PROTO.lookupType("Messages.AllInOneMessage");
    const allInOnePayload = {
      type,
      [fieldName]: wrappedMsg,
      sessionID: this.session_id
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

    console.log("Buffer received:", buffer);
    console.log("Message Received:", message);

    let msgToSent;

    switch (type) {
      case RA_MSG0:
        this.session_id = message.sessionID;
        msgToSent = this.assemble(RA_MSG0);
        break;

      case RA_MSG1:
        const { msg1 } = message;
        const { GaX, GaY } = msg1;
        const ecPublicKey = {
          X: GaX,
          Y: GaY
        };
        msgToSent = this.assemble(RA_MSG2, ecPublicKey);
        break;

      case RA_MSG3:
        msgToSent = this.assemble(RA_ATT_RESULT);
        break;

      case RA_APP_ATT_OK:
        const { phone } = this.state;
        msgToSent = this.assemble(PHONE_REG, phone);
        break;

      case PIN_CODE_TO:
        const { status } = message.pincodetoMsg.status;
        if (status !== 200) {
          this.setState({
            alert:
              "Unknown error occured, please refresh the page and try again",
            alertType: "warning"
          });
        }
        break;

      case PHONE_REG_RES:
        const { userID } = message.resMsg;
        this.setState({
          alert: `Register Succeed. User ID is ${buf2hexString(userID)}`,
          alertType: "success"
        });
        break;

      case SMS_RES:
        const { statusCode } = message.smsresMsg;
        if (statusCode === 200) {
          this.setState({
            alert: "Message sent successfully!",
            alertType: "success"
          });
        }
        break;

      default:
        break;
    }

    if (!msgToSent) return;

    WEB_SOCKET.send(msgToSent);
    console.log("======== Message sent ========\n\n\n\n\n");
  }

  render() {
    const { end } = this.props.match.params;

    if (end === "personal") return this.renderPersonal();
    if (end === "business") return this.renderBusiness();

    return null;
  }

  renderPersonal() {
    const { phone, pinCode, alert, alertType } = this.state;

    return (
      <Col xs={12} md={{ size: 8, offset: 2 }} className="base-margin-top">
        <Alert color={alertType} isOpen={!!alert}>
          {alert}
        </Alert>
        <Row className="base-margin-bottom">
          <Col xs={12}>
            <Input
              name="phone"
              value={phone}
              placeholder="Phone Number"
              onChange={this.handleOnChange}
            />
          </Col>
        </Row>
        <Row className="base-margin-bottom">
          <Col>
            <Input
              name="pinCode"
              value={pinCode}
              placeholder="Pin Code"
              onChange={this.handleOnChange}
            />
          </Col>
          <button disabled={!phone} onClick={this.setupWebSocket.bind(this)}>
            Get Pin Code
          </button>
        </Row>
        <Col className="text-center">
          <button disabled={!phone || !pinCode} onClick={this.handleRegister}>
            Register
          </button>
        </Col>
      </Col>
    );
  }

  renderBusiness() {
    const { userID, content, alert, alertType } = this.state;

    return (
      <Col xs={12} md={{ size: 8, offset: 2 }} className="base-margin-top">
        <Alert color={alertType} isOpen={!!alert}>
          {alert}
        </Alert>
        <Row className="base-margin-bottom">
          <Input
            name="userID"
            value={userID}
            placeholder="User ID"
            onChange={this.handleOnChange}
          />
        </Row>
        <Row className="base-margin-bottom">
          <Input
            name="content"
            type="textarea"
            value={content}
            placeholder="Text content..."
            onChange={this.handleOnChange}
          />
        </Row>
        <Col className="text-center">
          <button onClick={this.setupWebSocket.bind(this)}>Send Message</button>
        </Col>
      </Col>
    );
  }

  handleRegister() {
    const { pinCode } = this.state;
    const msgToSent = this.assemble(PIN_CODE_BACK, { pinCode });
    WEB_SOCKET.send(msgToSent);
    console.log("======== Message sent ========\n\n\n\n\n");
  }

  handleOnChange(event) {
    const { value, name } = event.target;
    this.setState({ [name]: value });
  }
}

export default GDPRDemo;
