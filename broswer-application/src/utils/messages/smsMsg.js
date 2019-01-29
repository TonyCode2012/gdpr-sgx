import {
  SMS_SEND
} from "../../metadata/messageTypes";

const getSmsMsg = ({ userID, content }) => {
  return {
    type: SMS_SEND,
    userID,
    sms: content,
    size: 0
  };
}

export default getSmsMsg;
