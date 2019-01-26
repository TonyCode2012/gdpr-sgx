package com.sgxtrial.yyy;

import com.github.qcloudsms.SmsSingleSender;
import com.github.qcloudsms.SmsSingleSenderResult;
import com.github.qcloudsms.httpclient.HTTPException;
import org.json.JSONException;

import java.io.IOException;


public class SMSSender {

	// SDK AppID
	private int appid = 1400183407; // 1400开头

	// SDK AppKey
	private String appkey = "9de52fe14435fcb5331e4c9a741df4eb";

	private int templateId = 273273; // NOTE: templateId is band to a wechat account and is not changable
	private String smsSign = " CRDC诺宝游泳健身预约 "; // NOTE: sign is band to a wechat account and is not changable


	public void sendMsg(String phoneNum, String code){

		try {
			String[] params = {code};
			SmsSingleSender ssender = new SmsSingleSender(appid, appkey);
			SmsSingleSenderResult result = ssender.sendWithParam("86", phoneNum,
					templateId, params, smsSign, "", "");  // 签名参数未提供或者为空时，会使用默认签名发送短信
			System.out.println(result);
		} catch (HTTPException e) {
			// HTTP响应码错误
			e.printStackTrace();
		} catch (JSONException e) {
			// json解析错误
			e.printStackTrace();
		} catch (IOException e) {
			// 网络IO错误
			e.printStackTrace();
		}
	}

	public static void main(String[] args){
		SMSSender sender = new SMSSender();
		sender.sendMsg("15366038076", "4321");
	}

}
