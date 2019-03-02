import { PIN_CODE_BACK } from "../../metadata/messageTypes";


const getPinCodeBackMsg = ({ pinCode }) => {
  return {
    type: PIN_CODE_BACK,
    pinCode,
    size: 0
  };
}


export default getPinCodeBackMsg;
