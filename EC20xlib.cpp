#include <Arduino.h>
#include "EC20xlib.h"
#include "iolog.h"

#define min _min
#define max _max

uint8_t pubsub_f = 0;

void EC20xlib::attachsubcallback(subcallbackFunction newFunction){
    subdatecallback = newFunction;
}
void EC20xlib::attachtcpcallback(recallbackFunction newFunction){
    getdatecallback = newFunction;
}
void EC20xlib::attacheventcallback(eventcallbackFunction newFunction){
    eventcallback = newFunction;
}
EC20xlib::EC20xlib(Stream &serial) : IOLTE(serial){

}
EC20xlib::~EC20xlib() {

}

byte EC20xlib::begin() {
    int returnValue = IOLTE_NOTOK;
    returnValue = initcom();
    return returnValue;
}

// Get the strength of the GSM signal.
int EC20xlib::Read_Signal() {
    String response = "";
    uint32_t respStart = 0;
    int strength, error  = 0;

    // Issue the command and wait for the response.
    IOLtecommand("AT+CSQ", "OK", "+CSQ", IOLTE_CMD_TIMEOUT, 2, &response);

    respStart = response.indexOf("+CSQ");
    if (respStart < 0) {
        return 0;
    }

    sscanf(response.substring(respStart).c_str(), "+CSQ: %d,%d",
           &strength, &error);

    // Bring value range 0..31 to 0..100%, don't mind rounding..
    strength = (strength * 100) / 31;
    return strength;
}
/************************************************
*函数名：ReadCCID
*功能  ：读取SIM卡CCID号
*参数  ：DataBuf（输出）  CCID号码缓存  应大于20
*返回  : 0 成功 1 失败
************************************************/
byte EC20xlib::Read_CCID(char *DataBuf)
{
    uint8_t re = 0;
    uint32_t respStart = 0;
    String response = "";
    //发送读取IMSI命令
    re=IOLtecommand("AT+CCID\r\n","+CCID","yy",IOLTE_CMD_TIMEOUT, 2, &response);
    //Serial.println(response);
    respStart = response.indexOf("+CCID");
    if (respStart < 0) {
        return 0;
    }
    sscanf(response.substring(respStart).c_str(), "+CCID: %s",DataBuf);

    return re;
}
/************************************************
*函数名：EC20_Read_IMEI
*功能  ：读取模块IMEI号
*参数  ：DataBuf（输出）  IMEI号码缓存  应大于20
*返回  : 0 成功 1 失败
************************************************/
byte EC20xlib::Read_IMEI(char *DataBuf)
{
    uint8_t re = 0;
    String response = "";
    //发送读取IMSI命令
    re = IOLtecommand("AT+GSN","OK","yy",IOLTE_CMD_TIMEOUT, 2, &response);
    //Serial.println(response);
    
    response = response.substring(2, 18);
    response.toCharArray(DataBuf,16,0);
    //Serial.println(DataBuf);
    return re;
}

// Get the real time from the modem. Time will be returned as yy/MM/dd,hh:mm:ss+XX
String EC20xlib::Read_TimeClock() {
    String response = "";

    // Issue the command and wait for the response.
    IOLtecommand("AT+CCLK?", "OK", "yy", IOLTE_CMD_TIMEOUT, 1, &response);
    int respStart = response.indexOf("+CCLK: \"") + 8;
    response.setCharAt(respStart - 1, '-');

    return response.substring(respStart, response.indexOf("\""));
}
// Read some data from the EC20x in a non-blocking manner.
byte EC20xlib::connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password,uint8_t keepalive,uint8_t will_qos,char *will,char *will_payload) {
    char sendbuf[100];
    String response = "";

    uint32_t respStart = 0;
    int client_idx = 0, error = 0,result = 0;
    for(int i = 0; i<2;i++)
    {
        //配置MQTT接收模式
        //AT+QMTCFG="recv/mode",<client_idx>[,<msg_recv_mode>[,<msg_len_enable>]] 
        /*
        <msg_recv_mode>  
            整型。配置 MQTT 消息接收模式。 
            0  从服务器接收的 MQTT 消息以 URC 的形式上报 
            1  从服务器接收的 MQTT 消息不以 URC 的形式上报 

        <msg_len_enable> 
            整型。配置 URC 中是否包含从服务器接收的 MQTT 消息长度。 
            0  不包含 
            1  包含 
        */
        sprintf(sendbuf,"AT+QMTCFG=\"recv/mode\",%d,%d,%d",0,0,1);
        if (IOLtecommand(sendbuf, "OK", "yy", IOLTE_CMD_TIMEOUT+3000, 3, &response) <= IOLTE_OK) return IOLTE_FAILURE; 
        
        //根据需要设置遗嘱消息
        /*
        AT+QMTCFG="will",<client_idx>[,<will_fg>[,<will_qos>,<will_retain>,<will_topic>,<will_msg>]] 
        */
        if((will !=NULL)&&(will_payload != NULL)){
            memset(sendbuf,0,sizeof(sendbuf));
            sprintf(sendbuf,"AT+QMTCFG=\"will\",0,1,1,1,\"%s\",\"%s\"",will,will_payload);   
            if (IOLtecommand(sendbuf, "yy", "OK", IOLTE_CMD_TIMEOUT+3000, 3,  &response) <= IOLTE_OK)  return IOLTE_FAILURE;
            log4g("%s","set will OK."); 
        }
        /*
        AT+QMTCFG="keepalive",<client_idx>[,<keep_alive_time>] 
        */
        memset(sendbuf,0,sizeof(sendbuf));
        sprintf(sendbuf,"AT+QMTCFG=\"keepalive\",0,%d",keepalive);
        if (IOLtecommand(sendbuf, "yy", "OK", IOLTE_CMD_TIMEOUT+3000, 3,  &response) <= IOLTE_OK)  return IOLTE_FAILURE;
        log4g("%s","set keepalive OK.");
        sprintf(sendbuf,"AT+QMTOPEN=0,\"%s\",%s",ip,port);
        if (IOLtecommand(sendbuf, "+QMTOPEN:", "yy", IOLTE_CMD_TIMEOUT+3000, 3, &response) > IOLTE_OK) {
            respStart = response.indexOf("+QMTOPEN:");
            if (respStart < 0) {
                return IOLTE_FAILURE;
            }
            response = response.substring(respStart,response.length());
            respStart = response.indexOf(",");
            error = atoi(response.substring(respStart+1,response.length()).c_str());
            //sscanf(DataBuf, "+QMTOPEN: %d,%d",&client_idx, &error);//未解决负数问题

            log4g("error: %d",error);
            if(error == 0){//连接成功
                if((username != NULL)&&(password != NULL))
                    sprintf(sendbuf,"AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"",clientID,username,password);
                else sprintf(sendbuf,"AT+QMTCONN=0,\"%s\"",clientID);
                if (IOLtecommand(sendbuf, "+QMTCONN:", "yy", IOLTE_CMD_TIMEOUT+3000, 3,  &response) > IOLTE_OK){
                    respStart = response.indexOf("+QMTCONN:");
                    if (respStart < 0) {
                        return IOLTE_FAILURE;
                    }
                    sscanf(response.substring(respStart).c_str(), "+QMTCONN: %d,%d,%d",
                    &client_idx,&result, &error);
                    if(error == 0){
                        connected_state = true;
                        if(eventcallback != NULL)
                            eventcallback(MQTT_CONNECTED);
                        log4g("%s","connect OK.");
                        return IOLTE_OK;
                    }
                    else return IOLTE_FAILURE;
                }
                else return IOLTE_FAILURE;
            }
            else if(error == 2){//MQTT 标识符被占用,需要先断开重新连接
                connected_state = false;
                if (IOLtecommand("AT+QMTCLOSE=0", "+QMTCLOSE: 0,0", "yy", IOLTE_CMD_TIMEOUT+3000, 3,  &response) > IOLTE_OK)
                    return IOLTE_FAILURE;
                if(eventcallback != NULL)
                    eventcallback(MQTT_DISCONNECT);
            }
            else if(error == -1){//MQTT 连接失败
                connected_state = false;
                if(eventcallback != NULL)
                    eventcallback(MQTT_DISCONNECT);
            }
            else return IOLTE_FAILURE;
        }
        else return IOLTE_FAILURE;
    }
    return IOLTE_FAILURE;   
}

byte EC20xlib::connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password,uint8_t keepalive) {
    return connectMqttServer(clientID,ip,port,username,password,keepalive,1,NULL,NULL);
}
byte EC20xlib::connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password) {
    return connectMqttServer(clientID,ip,port,username,password,60,1,NULL,NULL);
}
byte EC20xlib::connectMqttServer(char *clientID,char *ip,char *port) {
    return connectMqttServer(clientID,ip,port,NULL,NULL,60,1,NULL,NULL);
} 
/*
取消订阅主题
*/
byte EC20xlib::UnSubTopic(char *topic,uint8_t qos1)
{
  char data_buff[300];//格式化AT指令时使用
    
  log4g("%s","UnSubTopic");
  //订阅需要的主题
  sprintf(data_buff,"AT+QMTUNS=0,%d,%s",1,topic);
  if (IOLtecommand(data_buff,"OK","yy",5000,5,NULL) <= IOLTE_OK) return IOLTE_FAILURE;
	return IOLTE_OK;
}
/*
订阅主题
*/
byte EC20xlib::SubTopic(char *topic,uint8_t qos1)
{
  char data_buff[300];//格式化AT指令时使用
    
  log4g("%s","SubTopic");
  //订阅需要的主题
  sprintf(data_buff,"AT+QMTSUB=0,%d,\"%s\",%d",1,topic,qos1);
  if (IOLtecommand(data_buff,"+QMTSUB: 0,1,0,0","yy",5000,5,NULL) <= IOLTE_OK) return IOLTE_FAILURE;
	return IOLTE_OK;
}
/*
发布主题
*/
byte EC20xlib::PubTopic(char *topic,char* msg,uint8_t qos1)//typ_NEC_AQData  *AQData_Send,
{
  char data_buff[300];//格式化AT指令时使用
  String response = "";  
  String reply = "";

  log4g("%s","PubTopic");
  pubsub_f = 1;
  sprintf(data_buff,"AT+QMTPUBEX=0,0,%d,0,\"%s\",%d",qos1,topic,strlen(msg));
  if (IOLtecommand(data_buff,">","yy",IOLTE_CMD_TIMEOUT+3000,5,&response)  <= IOLTE_OK) return IOLTE_FAILURE;
  reply+=response;
  if (IOLtecommand(msg,"OK","yy",IOLTE_CMD_TIMEOUT+3000,5,&response) <= IOLTE_OK) return IOLTE_FAILURE;
  reply+=response;
  pubsub_f = 0;
  ParseMsg(reply);

  return IOLTE_OK;
}
/*
MQTT是否连接
*/
boolean EC20xlib::connected() {
    
    return connected_state;
}

/*
TCP断开连接服务器
*/
int EC20xlib::disconnect(uint8_t connectID)
{
    char sdata[30];//格式化AT指令时使用
    uint8_t retVal = IOLTE_NOTOK;
    sprintf(sdata,"AT+QICLOSE=%d",connectID);
    if(IOLtecommand(sdata,"OK","yy",IOLTE_CMD_TIMEOUT,2,NULL)!=0)
        retVal = IOLTE_FAILURE;
    else retVal = IOLTE_OK;

    return retVal;
}

/*
TCP连接服务器
*/
int EC20xlib::connect(char *ip, char *port,uint8_t connectID)
{
    char data_buff[100];//格式化AT指令时使用
    char sdata[30];//格式化AT指令时使用
    uint8_t retVal = IOLTE_NOTOK;
    String reply = "";
    log4g("%s","creat a socket");
    //判断是否连接服务器
    sprintf(data_buff,"AT+QIOPEN=1,%d,\"TCP\",\"%s\",%s,0,1",connectID,ip,port);
    sprintf(sdata,"+QIOPEN: %d,",connectID);
    if(IOLtecommand(data_buff,sdata,"yy",IOLTE_CMD_TIMEOUT+3000,1,&reply) == 0)
    {
        memset(sdata,0,sizeof(sdata));
        sprintf(sdata,"+QIOPEN: %d,0",connectID);
        if(reply.indexOf(sdata) != -1)
        {
            log4g("%s","connected server successfully");
            return IOLTE_OK;
        }
        else //连接失败
        {
            log4g("%s","send failed,close the socket!!");

            if(disconnect(connectID)!=0)
                retVal = IOLTE_FAILURE;
            memset(sdata,0,sizeof(sdata));
            sprintf(sdata,"+QIOPEN: %d,0",connectID);
            if(IOLtecommand(data_buff,sdata,"yy",IOLTE_CMD_TIMEOUT,2,NULL) != 0)
                retVal = IOLTE_FAILURE;
            else
                retVal = IOLTE_OK;
        }
    }
    return retVal;
}

/************************************************
*函数名： EC20_senddata
*功能  ： 发送AQ
*参数  ： buf     数据
*        buflen  数据包长度
*输入  ： 无
*输出  ： 无
************************************************/
int EC20xlib::postdata(char* buf, int buflen,uint8_t connectID)
{
    uint8_t retVal = IOLTE_NOTOK;
    char data_buff[30];//格式化AT指令时使用
    
    // 喂狗操作，防止信号问题引起死机
//    IWDG_Feed();

    //获取数据包长度，合并成字符串
    sprintf(data_buff,"AT+QISEND=%d,%d\r\n",connectID,buflen);

    if(IOLTE_OK == IOLtecommand(data_buff,strlen(data_buff),">","yy",IOLTE_CMD_TIMEOUT+3000,1,NULL)){
        //发送数据
        if(IOLTE_OK == IOLtecommand(buf,buflen,"SEND OK","yy",IOLTE_CMD_TIMEOUT+3000,1,NULL))
            retVal = IOLTE_OK;
        else retVal = IOLTE_FAILURE;
    }else retVal = IOLTE_FAILURE;
    return retVal;
}
/*
MQTT解析收到的数据
*/
byte EC20xlib::ParseMsg(String data){
    String reply = data;
    if(pubsub_f == 0){
        if(data.indexOf("+QMTRECV:")>=0){
            //+QMTRECV: 0,0,"/server/123/17091089680/7619748","{"command_type": 2,"command_id":5,"dzcount":160}"
            log4g("%s","*******************************");
            log4g("%d",reply.length());
            log4g("%s",reply.c_str());
            log4g("%s","*******************************");
            char topic[TOPIC_MAX_LEN];
            char msg[PAYLOAD_MAX_LEN];
            char payload_len[4];
            do{
                memset(topic,0,sizeof(topic));
                memset(msg,0,sizeof(msg));
                memset(payload_len,0,sizeof(payload_len));
                int commaPosition = 0;//存储还没有分离出来的字符串
                commaPosition = reply.indexOf("+QMTRECV:");
                if(commaPosition>=0){
                    reply = reply.substring(commaPosition,reply.length());
                    for(uint8_t i = 0;i<4;i++){
                        commaPosition = reply.indexOf(',');//检测字符串中的逗号
                    
                        if(commaPosition != -1){//如果有逗号存在就向下执行
                            if(i == 2){//获取topic
                                log4g("%s", reply.substring(1,commaPosition-1).c_str());//打印出第一个逗号位置的字符串
                                reply.substring(1,commaPosition-1).toCharArray(topic,reply.substring(1,commaPosition-1).length()+1);
                            }
                            else if(i == 3){//获取msg长度
                                log4g("%s", reply.substring(0,commaPosition));//打印出第一个逗号位置的字符串
                                reply.substring(0,commaPosition).toCharArray(payload_len,reply.substring(0,commaPosition).length()+1);
                            }
                            reply = reply.substring(commaPosition+1, reply.length());//打印字符串，从当前位置+1开始
                        }
                    }
                    //log4g("%s",reply.c_str());
                    if(atoi(payload_len) != 0){//获取topic的内容
                        reply.substring(1,reply.length()).toCharArray(msg,atoi(payload_len)+1);
                        log4g("%s",msg);
                        if(subdatecallback != NULL)
                            subdatecallback(topic,msg,atoi(payload_len),0);
                        //log4g("====================================");
                    }  
                    //log4g("%s",reply.c_str());
                }
            }while(reply.indexOf("+QMTRECV:")>=0);
        }
        /*
            <client_idx>  整型。MQTT 客户端标识符。
            <err_code>  整型。错误代码。详细信息请参考下表。 

            1  连接被服务器断开或者重置       执行 AT+QMTOPEN 重建 MQTT 连接 
            2  发送 PINGREQ 包超时或者失败   首先反激活 PDP，然后再激活 PDP 并重建MQTT 连接。 
            3  发送 CONNECT 包超时或者失败     1.  查看输入的用户名和密码是否正确。 
                                            2.  确保客户端 ID 未被占用。 
                                            3.  重建 MQTT 连接，并尝试再次发送CONNECT 包到服务器。 
            4  接收 CONNACK 包超时或者失败     1.  查看输入的用户名和密码是否正确。 
                                            2.  确保客户端 ID 未被占用。 
                                            3.  重建 MQTT 连接，并尝试再次发送CONNECT 包到服务器。 
            5  客户端向服务器发送 DISCONNECT 包，但
                是服务器主动断开 MQTT 连接     正常流程。 
            6  因为发送数据包总是失败，客户端主动断开MQTT 连接         1.  确保数据正确。 
                                                                2.  可能因为网络拥堵或者其他错误，尝试重建 MQTT 连接。 
            7  链路不工作或者服务器不可用  确保当前链路或者服务器可用。 
            8  客户端主动断开 MQTT 连接  尝试重建连接。 
            9~255  留作将来使用   
        */
        if(data.indexOf("+QMTSTAT: 0,1") >= 0){//连接被服务器断开或者重置
            connected_state = false;
            if(eventcallback != NULL) eventcallback(MQTT_DISCONNECT);
        }
        if(data.indexOf("+QMTSTAT: 0,2") >= 0){//发送 PINGREQ 包超时或者失败
            connected_state = false;
            if(eventcallback != NULL) eventcallback(MQTT_DISCONNECT);
        }
        if(data.indexOf("+QMTSTAT: 0,3") >= 0){//发送 CONNECT 包超时或者失败 
            connected_state = false;
            if(eventcallback != NULL) eventcallback(MQTT_DISCONNECT);
        }
        if(data.indexOf("+QMTSTAT: 0,8") >= 0){//客户端主动断开 MQTT 连接 
            connected_state = false;
            if(eventcallback != NULL) eventcallback(MQTT_DISCONNECT);
        }
    }
   return 0;
}

/*
判断模块返回
*/
void EC20xlib::loop()
{  
    String reply = "";
    IOLtewaitFor(NULL,NULL,0, &reply);
}