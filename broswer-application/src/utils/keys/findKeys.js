const { switchEndian, toHex } = require("../hexHelpers");
const {
  myPrivateKey,
  getMyPublicKey,
  getMySharedKey
} = require("./ecdh");


let MY_PRIVATE_KEY, MY_PUBLIC_KEY, MY_SHARED_KEY, SHARED_KEY, SHORT_KEY;

/**
 * @method findKeys
 * @param {Object} ecPublicKey 
 */
function findKeys(ecPublicKey = {}) {

  // typeof MY_PRIVATE_KEY: String
  MY_PRIVATE_KEY = MY_PRIVATE_KEY || myPrivateKey;

  // typeof MY_PUBLIC_KEY: Object
  MY_PUBLIC_KEY = MY_PUBLIC_KEY || getMyPublicKey(MY_PRIVATE_KEY);

  // typeof MY_SHARED_KEY: Object
  MY_SHARED_KEY = MY_SHARED_KEY || getMySharedKey(MY_PRIVATE_KEY, ecPublicKey);

  // typeof SHARED_KEY: String
  SHARED_KEY = SHARED_KEY || MY_SHARED_KEY.X + MY_SHARED_KEY.Y;

  // typeof SHORT_KEY: String
  SHORT_KEY = SHORT_KEY || switchEndian(toHex(MY_SHARED_KEY.X)).slice(0, 32);

  // console.log("my pri:", MY_PRIVATE_KEY);

  // console.log("my pub X:", MY_PUBLIC_KEY.X);
  // console.log("my pub Y:", MY_PUBLIC_KEY.Y);

  // console.log("ec pub X:", ecPublicKey.X);
  // console.log("ec pub Y:", ecPublicKey.Y);

  // console.log("sha:", MY_SHARED_KEY.X);
  // console.log("sha:", MY_SHARED_KEY.Y);


  return {
    MY_PRIVATE_KEY,
    MY_PUBLIC_KEY,
    MY_SHARED_KEY,
    SHARED_KEY,
    SHORT_KEY
  }
}

module.exports = findKeys;
