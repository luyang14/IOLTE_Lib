# IOLTE_Lib

## 简介

支持MQTT和HTTP协议操作
理论上支持所有带串口模块进行AT指令操作，但需要逐步完善

## 使用示例

```c
EC20xlib M4G(Serial2);
···
···
void setup() {
	//EC20串口初始化
	Serial2.begin(115200);
	//4G模块电源
	M4G.powerOn(M4G_POWKEY);
	//订阅回调
	M4G.attachsubcallback(subdatacallback);
	//模块连接状态回调
	M4G.attacheventcallback(eventcallback);
	logi("Init 4G");
	uint8_t restatus = 0;
	//模块初始化
	restatus = M4G.begin();
	if(restatus == 0) logi("Init 4G module -- OK");
	//读取CCID
	M4G.Read_CCID(simnum);
	logi("CCID:%s",simnum);
	//读取IMEI
	M4G.Read_IMEI(imei_num);
	logi("IMEI:%s",imei_num);
	//连接MQTT
	M4G.connectMqttServer(mqtt_pubtopic,mqtt_server,mqtt_port,mqtt_user,mqtt_pass,60,1,mqtt_willtopic,"offline");
	//订阅主题
	M4G.SubTopic(mqtt_subtopic,M_QOS);
	//发布主题
	M4G.PubTopic(mqtt_willtopic,"online",M_QOS);
}

```