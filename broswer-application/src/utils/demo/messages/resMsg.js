import { PHONE_RES } from "../../../metadata/messageTypes";
import {
  hexStringToArray
} from "../../hexHelpers";

const getResMsg = () => {
  return {
    type: PHONE_RES,
    userID: 1111111111,
    size: 16
  };
}


export default getResMsg;
