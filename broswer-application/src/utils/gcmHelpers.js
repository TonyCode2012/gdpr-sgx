
const crypto = require('crypto');

export function encrypt(key, text) {
  const plainText = new Buffer(Uint8Array.from(text), 'hex');
  const iv = new Buffer('000000000000000000000000', 'hex');
  const bufferKey = new Buffer(key, 'hex');

  const cipher = crypto.createCipheriv('aes-128-gcm', bufferKey, iv);

  var encrypted = cipher.update(plainText, 'utf9', 'hex');
  encrypted += cipher.final('hex');
  const tag = cipher.getAuthTag();

  return { encrypted, tag };
}