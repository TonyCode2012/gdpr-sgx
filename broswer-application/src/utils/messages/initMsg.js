import { RA_VERIFICATION } from "../../metadata/messageTypes";


const getInitMsg = () => {
  return {
    type: RA_VERIFICATION,
    size: 1
  };
}


export default getInitMsg;
