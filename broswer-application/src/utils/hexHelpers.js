const bigInt = require("big-integer");

/**
 * @method switchEndian
 * @param {String} string needed to be transformed
 * @returns {String} result
 */
export function switchEndian(string) {
  if (string.length % 2) {
      string += '0'
  }
  //throw new Error();

  let result = []
  for (let i = 0; i < string.length; i = i + 2) {
    const item = string.slice(i, i + 2);
    result.push(item);
  }

  return result.reverse().join("");
}

/**
 * @method toHex
 * @param {String} input needed to be transformed
 * @returns {String} result
 */
export function toHex(input) {
  return bigInt(input).toString(16);
}

/**
 * @method hexStringToArray
 * @param {String} str
 * @returns {Array} result
 */
export function hexStringToArray(str, step) {
  if (str.length % step){
      const gap = step - str.length % step
      str += "0".repeat(gap)
  } 
  //throw new Error();

  let result = [];
  for (let i = 0; i < str.length; i = i + step) {
    const chunk = str.slice(i, i + step);
    const num = parseInt(chunk, 16);
    result.push(num);
  }

  return result;
}

export function buf2hexString(buffer) { // buffer is an ArrayBuffer
  return Array.prototype.map.call(new Uint8Array(buffer), x => ('00'  + x.toString(16)).slice(-2)).join('');
}
