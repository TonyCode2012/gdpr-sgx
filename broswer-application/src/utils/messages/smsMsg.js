import {
  SMS_SEND
} from "../../metadata/messageTypes";
import {
  hexStringToArray
} from "../hexHelpers";

const getSmsMsg = ({ userID, content }) => {
  return {
    type: SMS_SEND,
    userID: Uint8Array.from(hexStringToArray(userID, 2)),
    sms: content,
    size: 0
  };
}

export default getSmsMsg;
