import { RA_MSG0 } from "../../../metadata/messageTypes";


const getMsg0 = () => {
  return {
    type: RA_MSG0,
    epid: 0,
    status: 1
  };
}


export default getMsg0;