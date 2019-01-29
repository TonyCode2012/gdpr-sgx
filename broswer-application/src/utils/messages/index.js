import initMsg from "./initMsg";
import msg0 from "./msg0";
import msg2 from "./msg2";
import attMsg from "./attMsg";
import regMsg from "./regMsg";
import resMsg from "./resMsg";
import smsMsg from "./smsMsg";

import {
  RA_MSG0,
  RA_MSG1,
  RA_MSG2,
  RA_MSG3,
  RA_ATT_RESULT,
  RA_VERIFICATION,
  RA_APP_ATT_OK,
  PHONE_REG,
  PHONE_RES,
  SMS_SEND
} from "../../metadata/messageTypes";


const registry = {
  [RA_MSG0]: {
    defName: "MessageMSG0",
    fieldName: "msg0",
    getPayload: msg0
  },
  [RA_MSG1]: {
    defName: "MessageMSG1",
    fieldName: "msg1",
    getPayload: (EC_PUBLIC_KEY) => ({
      type: RA_MSG1,
      GaX: EC_PUBLIC_KEY.X,
      GaY: EC_PUBLIC_KEY.Y,
      Gid: 3
    })
  },
  [RA_MSG2]: {
    defName: "MessageMSG2",
    fieldName: "msg2",
    getPayload: msg2
  },
  [RA_MSG3]: {
    defName: "MessageMSG3",
    fieldName: "msg3",
    getPayload: () => { }
  },
  [RA_ATT_RESULT]: {
    defName: "AttestationMessage",
    fieldName: "attestMsg",
    getPayload: attMsg
  },
  [RA_VERIFICATION]: {
    defName: "InitialMessage",
    fieldName: "initMsg",
    getPayload: initMsg
  },
  [RA_APP_ATT_OK]: {
    defName: "SecretMessage",
    fieldName: "secretMsg",
    getPayload: () => { }
  },
  [PHONE_REG]: {
    defName: "RegisterMessage",
    fieldName: "regMsg",
    getPayload: regMsg
  },
  [PHONE_RES]: {
    defName: "ResponseMessage",
    fieldName: "resMsg",
    getPayload: resMsg
  },
  [SMS_SEND]: {
    defName: "SMSMessage",
    fieldName: "smsMsg",
    getPayload: smsMsg
  },
}


export default registry;
