import {
  PHONE_REG
} from "../../metadata/messageTypes";
import {
  REG_SIZE,
} from "../../metadata/ecConstants";
import {
  encrypt
} from "../gcmHelpers";
import {
  hexStringToArray
} from "../hexHelpers";

const findKeys = require("../keys/findKeys");

const getRegMsg = (phoneNum) => {
  const {
    SHORT_KEY
  } = findKeys();

  const {
    encrypted,
    tag
  } = encrypt(SHORT_KEY, phoneNum);

  return {
    type: PHONE_REG,
    cipherPhone: hexStringToArray(encrypted, 2),
    mac: tag,
    size: REG_SIZE
  };
}


export default getRegMsg;
