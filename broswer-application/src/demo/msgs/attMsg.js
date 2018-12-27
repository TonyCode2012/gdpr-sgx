const encrypt = require("node-aes-gcm").gcm;
const aesCmac = require("node-aes-cmac").aesCmac;

const findKeys = require("../utils/keys");

const {
  RA_ATT_RESULT
} = require("../metadata/messageTypes");
const {
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
} = require("../metadata/constants");


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


function getPayload() {

  const { SHORT_KEY } = findKeys();

  /**
   * @desc calc macSmk <-- cmac(mk, platform_info_blob)
   */
  const macSmk = aesCmac(SHORT_KEY, PLATFORM_INFO_BLOB);

  /**
   * @desc calc encrypted payload & payload_tag
   */
  const bufferKey = new Buffer(SHORT_KEY, 'hex');
  const plainPayload = new Buffer('01', 'hex');
  const aes_gcm_iv = new Buffer('000000000000', 'hex');
  const { ciphertext, auth_tag } = encrypt(bufferKey, aes_gcm_iv, plainPayload, new Buffer([]));

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
    payloadTag: auth_tag,
    payload: ciphertext
  };
}

module.exports = getPayload;
