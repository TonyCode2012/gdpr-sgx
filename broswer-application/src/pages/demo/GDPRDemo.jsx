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
  PHONE_REG_END,
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
  alertType: "",
  timer: 0
};

class GDPRDemo extends React.Component {
  constructor() {
    super();
    this.state = { ...origin };
    this.session_id = 4294967295;

    this.handleOnChange = this.handleOnChange.bind(this);
    this.handleRegister = this.handleRegister.bind(this);
    this.handleGetPinCode = this.handleGetPinCode.bind(this);
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
    let status;

    switch (type) {
      case RA_MSG0:
        status = message.msg0.status;
        if (status === 200) {
          this.session_id = message.sessionID;
          msgToSent = this.assemble(RA_MSG0);
          this.setState({
            alertType: "success"
          });
        } else {
          this.setState({
            alert: "Unknown error occured(msg0), please refresh the page and try again",
            alertType: "warning"
          });
        }
        break;

      case RA_MSG1:
        const { msg1 } = message;
        status = message.msg1.status;
        if (status === 200) {
          const { GaX, GaY } = msg1;
          const ecPublicKey = {
            X: GaX,
            Y: GaY
          };
          msgToSent = this.assemble(RA_MSG2, ecPublicKey);
          this.setState({
            alertType: "success"
          });
        } else {
          this.setState({
            alert: "Unknown error occured(msg1), please refresh the page and try again",
            alertType: "warning"
          });
        }
        break;

      case RA_MSG3:
        status = message.msg3.status;
        if (status === 200) {
          msgToSent = this.assemble(RA_ATT_RESULT);
          this.setState({
            alertType: "success"
          });
        } else {
          this.setState({
            alert: "Unknown error occured(msg3), please refresh the page and try again",
            alertType: "warning"
          });
        }
        break;

      case RA_APP_ATT_OK:
        status = message.initMsg.status;
        if (status === 200) {
          const { phone } = this.state;
          msgToSent = this.assemble(PHONE_REG, phone);
          this.setState({
            alertType: "success"
          });
        } else {
          this.setState({
            alert: "Unknown error occured(attmsg), please refresh the page and try again",
            alertType: "warning"
          });
        }
        break;

      case PIN_CODE_TO:
        status = message.pincodetoMsg.status;
        if (status === 200) {
          this.setState({
            alert: "Pin code sent out successfully.",
            alertType: "success"
          });
        } else {
          this.setState({
            alert:
              "Unknown error occured, please refresh the page and try again",
            alertType: "warning"
          });
        }
        break;

      case PHONE_REG_END:
        status = message.resMsg.status;
        if (status === 200) {
          const { userID } = message.resMsg;
          this.setState({
            alert: `Register Succeed. User ID is ${buf2hexString(userID)}`,
            alertType: "success"
          });
        } else {
          this.setState({
            alert: "Unknown error occured, please refresh the page and try again",
            alertType: "warning"
          });
        }
        break;

      case SMS_RES:
        status = message.smsresMsg.status;
        if (status === 200) {
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
    const { phone, pinCode, alert, alertType, timer } = this.state;

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
          <button disabled={!phone || !!timer} onClick={this.handleGetPinCode}>
            Get Pin Code {!!timer && `(${timer})`}
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

  handleGetPinCode() {
    this.setupWebSocket();

    this.setState({ timer: 60 }, () => {
      const countDown = setInterval(() => {
        const { timer } = this.state;
        if (timer > 0) {
          this.setState({ timer: timer - 1 });
        } else {
          clearInterval(countDown);
        }
      }, 1000);
    });
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
