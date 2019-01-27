import {
  PHONE_REG
} from "../../../metadata/messageTypes";
import {
  hexStringToArray
} from "../../hexHelpers";

const crypto = require('crypto');
const findKeys = require("../keys/findKeys");

function encrypt(key, text) {
  const phone = new Buffer(Uint8Array.from(text), 'hex');
  const iv = new Buffer('000000000000000000000000', 'hex');
  const bufferKey = new Buffer(key, 'hex');

  const cipher = crypto.createCipheriv('aes-128-gcm', bufferKey, iv);

  console.log("=====shared key is:", bufferKey);

  var cipherText = cipher.update(phone, 'utf9', 'hex')
  cipherText += cipher.final('hex');
  const mac = cipher.getAuthTag();

  console.log("=====mac is:", mac);
  console.log("=====cipherText is:", cipherText);


  return {
    cipherText: cipherText,
    mac: mac
  };
}

const getRegMsg = (phoneNum) => {
  const {
    SHORT_KEY
  } = findKeys();

  const {
    cipherText,
    mac
  } = encrypt(SHORT_KEY, phoneNum);

  return {
    type: PHONE_REG,
    cipherPhone: hexStringToArray(cipherText, 2),
    mac: mac,
    size: 11
  };
}


export default getRegMsg;
