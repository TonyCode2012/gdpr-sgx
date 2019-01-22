import { RA_ATT_RESULT } from "../../../metadata/messageTypes";
import {
  ATT_SIZE,
  IAS_EPID_GROUP_STATUS_REVOKED_BIT_POS,
  IAS_EPID_GROUP_STATUS_REKEY_AVAILABLE_BIT_POS,
  IAS_TCB_EVAL_STATUS_CPUSVN_OUT_OF_DATE_BIT_POS,
  IAS_TCB_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS,
  IAS_PSE_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS,
  IAS_PSE_EVAL_STATUS_EPID_GROUP_REVOKED_BIT_POS,
  IAS_PSE_EVAL_STATUS_PSDASVN_OUT_OF_DATE_BIT_POS,
  IAS_PSE_EVAL_STATUS_SIGRL_OUT_OF_DATE_BIT_POS,
  IAS_PSE_EVAL_STATUS_PRIVRL_OUT_OF_DATE_BIT_POS,
  PLATFORM_INFO_BLOB,
  LATEST_EQUIVALENT_TCB_PSVN,
  LATEST_PSE_ISVSVN,
  LATEST_PSDA_SVN,
  PERFORMANCE_REKEY_GID,
  SIGNATURE_X,
  SIGNATURE_Y,
  RESULT_SIZE,
  RESERVED
} from "../../../metadata/ecConstants";
import {
  switchEndian,
  hexStringToArray
} from "../../hexHelpers";

const crypto = require('crypto');
const aesCmac = require("node-aes-cmac").aesCmac;
const findKeys = require("../keys/findKeys");


const epidGroupStatus = (
  parseInt(IAS_EPID_GROUP_STATUS_REVOKED_BIT_POS).toString(2) |
  parseInt(IAS_EPID_GROUP_STATUS_REKEY_AVAILABLE_BIT_POS).toString(2)
);

const tcbEvaluationStatus = (
  parseInt(IAS_TCB_EVAL_STATUS_CPUSVN_OUT_OF_DATE_BIT_POS).toString(2) |
  parseInt(IAS_TCB_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS).toString(2)
);

const pseEvaluationStatus = (
  parseInt(IAS_PSE_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS).toString(2) |
  parseInt(IAS_PSE_EVAL_STATUS_EPID_GROUP_REVOKED_BIT_POS).toString(2) |
  parseInt(IAS_PSE_EVAL_STATUS_PSDASVN_OUT_OF_DATE_BIT_POS).toString(2) |
  parseInt(IAS_PSE_EVAL_STATUS_SIGRL_OUT_OF_DATE_BIT_POS).toString(2) |
  parseInt(IAS_PSE_EVAL_STATUS_PRIVRL_OUT_OF_DATE_BIT_POS).toString(2)
);


function encrypt(key) {
  const text = new Buffer('0607', 'hex');
  const iv = new Buffer('000000000000000000000000', 'hex');
  const bufferKey = new Buffer(key, 'hex');

  const cipher = crypto.createCipheriv('aes-128-gcm', bufferKey, iv);

  console.log("=====shared key is:",bufferKey);

  var encrypted = cipher.update(text, 'utf9', 'hex')
  encrypted += cipher.final('hex');
  const tag = cipher.getAuthTag();
  console.log("=====tag is:",tag);
  console.log("=====secrat is:",encrypted);


  return {
    content: encrypted,
    tag: tag
  };
}


const getAttMsg = () => {
  const { SHORT_KEY } = findKeys();

  /**
   * @desc calc macSmk <-- cmac(mk, platform_info_blob)
   */
  const macSmk = aesCmac(SHORT_KEY, PLATFORM_INFO_BLOB);

  /**
   * @desc calc encrypted payload & payload_tag
   */
  const { content, tag } = encrypt(SHORT_KEY);

  console.log("=============== mac is:",tag);
  console.log("=============== payload is:",hexStringToArray(content,1));
  console.log("=============== payload is:",content);

  return {
    type: RA_ATT_RESULT,
    size: ATT_SIZE,

    epidGroupStatus,
    tcbEvaluationStatus,
    pseEvaluationStatus,
    latestEquivalentTcbPsvn: LATEST_EQUIVALENT_TCB_PSVN,
    latestPseIsvsvn: LATEST_PSE_ISVSVN,
    latestPsdaSvn: LATEST_PSDA_SVN,
    performanceRekeyGid: PERFORMANCE_REKEY_GID,
    ecSign256X: SIGNATURE_X,
    ecSign256Y: SIGNATURE_Y,

    macSmk,
    resultSize: RESULT_SIZE,
    reserved: RESERVED,
    payloadTag: tag,
    payload: hexStringToArray(content,1)
  };
}


export default getAttMsg;
