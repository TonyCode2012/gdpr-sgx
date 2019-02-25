import { PHONE_REG_RES } from "../../metadata/messageTypes";
import {
  RES_SIZE,
} from "../../metadata/ecConstants";

const getResMsg = () => {
  return {
    type: PHONE_REG_RES,
    userID: 1111111111,
    size: RES_SIZE
  };
}


export default getResMsg;
