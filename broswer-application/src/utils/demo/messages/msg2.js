import {
  MSG2_SIZE,
  SPID,
  SIGRL,
  SIZE_SIGRL,
  AES_CMAC_KDF_ID,
  SAMPLE_QUOTE_LINKABLE_SIGNATURE
} from "../../../metadata/ecConstants";
import {
  RA_MSG2
} from "../../../metadata/messageTypes";
import {
  switchEndian,
  toHex,
  hexStringToArray
} from "../../hexHelpers";

const aesCmac = require("node-aes-cmac").aesCmac;
const EC = require('elliptic').ec;
// const ec = new EC('p256');
const bigInt = require("big-integer");
const findKeys = require("../keys/findKeys");


function handleEcdhParam(decArray) {
  const hexStrArray = decArray.map(num => {
    const hex = num.toString(16);
    return (hex.length < 2) ? '0' + hex : hex;
  });
  const hexString = hexStrArray.join("");
  const switchedHexString = switchEndian(hexString);
  const decimalString = bigInt(switchedHexString, 16).toString();
  return decimalString;
}


const getMsg2 = ecPublicKey => {
  const X = handleEcdhParam(ecPublicKey.X);
  const Y = handleEcdhParam(ecPublicKey.Y);

  const {
    // MY_PRIVATE_KEY,
    MY_PUBLIC_KEY,
    SHORT_KEY
  } = findKeys({ X, Y });

  /**
   * @desc get signature: sign publck keys with my private key
   */
  // const signKey = ec.keyFromPrivate(MY_PRIVATE_KEY);
  // const sign_x = signKey.sign(MY_PUBLIC_KEY.X + ecPublicKey.X);
  // const sign_y = signKey.sign(MY_PUBLIC_KEY.Y + ecPublicKey.Y);
  // const signatureX = sign_x.toDER();
  // const signatureY = sign_y.toDER();

  /**
   * @desc get smac
   */
  const publicKeyGx = toHex(MY_PUBLIC_KEY.X);
  const publicKeyGy = toHex(MY_PUBLIC_KEY.Y);
  const sMyPublicKey = switchEndian(bigInt(MY_PUBLIC_KEY.X).toString(16), 2) + switchEndian(bigInt(MY_PUBLIC_KEY.Y).toString(16), 2);

  const smac = aesCmac(SHORT_KEY, sMyPublicKey);

  return {
    type: RA_MSG2,
    size: MSG2_SIZE,
    publicKeyGx: hexStringToArray(switchEndian(publicKeyGx), 2),
    publicKeyGy: hexStringToArray(switchEndian(publicKeyGy), 2),
    quoteType: SAMPLE_QUOTE_LINKABLE_SIGNATURE,
    spid: hexStringToArray(SPID, 2),
    cmacKdfId: AES_CMAC_KDF_ID,
    signatureX: hexStringToArray(publicKeyGx, 2),
    signatureY: hexStringToArray(publicKeyGy, 2),
    smac: hexStringToArray(smac, 2),

    sizeSigrl: SIZE_SIGRL,
    sigrl: SIGRL
  }
}

export default getMsg2;
