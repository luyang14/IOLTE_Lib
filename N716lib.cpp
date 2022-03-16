#include <Arduino.h>
#include "N716lib.h"
#include "iolog.h"

#define min _min
#define max _max

uint8_t pubsub_f = 0;

/////////////////////////////////////////////
// Public methods.
//
void N716lib::attachsubcallback(subcallbackFunction newFunction){
    subdatecallback = newFunction;
}
void N716lib::attachtcpcallback(recallbackFunction newFunction){
    getdatecallback = newFunction;
}
void N716lib::attacheventcallback(eventcallbackFunction newFunction){
    eventcallback = newFunction;
}

N716lib::N716lib(Stream &serial)  : IOLTE(serial){
    
}


N716lib::~N716lib() {

    //delete N716conn;
}
byte N716lib::begin() {
    int returnValue = IOLTE_NOTOK;
    returnValue = initcom();
    log4g("%d",returnValue);
    if(returnValue == IOLTE_OK){//激活网络
    returnValue = IOLtecommand("AT+XIIC=1", "yy", "OK", IOLTE_CMD_TIMEOUT+3000, 3, NULL);
        if ( returnValue <= IOLTE_OK) return returnValue;
    }
    return returnValue;
}

// Get the strength of the GSM signal.
int N716lib::Read_Signal() {
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
byte N716lib::Read_CCID(char *DataBuf)
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
byte N716lib::Read_IMEI(char *DataBuf)
{
    uint8_t re = 0;
    uint32_t respStart = 0;
    String response = "";
    //发送读取IMSI命令
    re = IOLtecommand("AT+GSN","OK","yy",IOLTE_CMD_TIMEOUT, 2, &response);
    //Serial.println(response);
    
   //Serial.println(response);
    respStart = response.indexOf("+GSN:");
    if (respStart < 0) {
        return 0;
    }
    sscanf(response.substring(respStart).c_str(), "+GSN: %s",DataBuf);
    return re;
}

// Get the real time from the modem. Time will be returned as yy/MM/dd,hh:mm:ss+XX
String N716lib::Read_TimeClock() {
    String response = "";

    // Issue the command and wait for the response.
    IOLtecommand("AT+CCLK?", "OK", "yy", IOLTE_CMD_TIMEOUT, 1, &response);
    int respStart = response.indexOf("+CCLK: \"") + 8;
    response.setCharAt(respStart - 1, '-');

    return response.substring(respStart, response.indexOf("\""));
}

// Read some data from the N716 in a non-blocking manner.
byte N716lib::connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password,uint8_t keepalive,uint8_t will_qos,char *will,char *will_payload) {
    char sendbuf[100];
    String response = "";

    for(int i = 0; i<2;i++)
    {
        if((username != NULL)&&(password != NULL))
            sprintf(sendbuf,"AT+MQTTCONNPARAM=\"%s\",\"%s\",\"%s\"",clientID,username,password);
        else sprintf(sendbuf,"AT+MQTTCONNPARAM=\"%s\",\"%s\",\"%s\"",clientID,username,password);
        if (IOLtecommand(sendbuf, "yy", "OK", IOLTE_CMD_TIMEOUT+3000, 3, &response) > IOLTE_OK) 
        {
            //根据需要设置遗嘱消息
            if((will !=NULL)&&(will_payload != NULL)){
                memset(sendbuf,0,sizeof(sendbuf));
                sprintf(sendbuf,"AT+MQTTWILLPARAM=0,1,\"%s\",\"%s\"",will,will_payload);   
                if (IOLtecommand(sendbuf, "yy", "OK", IOLTE_CMD_TIMEOUT+3000, 3,  &response) <= IOLTE_OK) return IOLTE_FAILURE;
                log4g("%s","set will OK."); 
            }
            memset(sendbuf,0,sizeof(sendbuf));
            sprintf(sendbuf,"AT+MQTTCONN=\"%s:%s\",%d,%d",ip,port,0,keepalive);
            if (IOLtecommand(sendbuf, "yy", "OK", IOLTE_CMD_TIMEOUT+3000, 3,  &response) > IOLTE_OK)
            {
                log4g("%s","connect OK.");
                connected_state = true;
                return IOLTE_OK;
            }
            else return IOLTE_FAILURE;
        }
        else return IOLTE_FAILURE;
    }
    return IOLTE_FAILURE;       
    /*
    sprintf(sendbuf,"AT+QMTOPEN=0,\"%s\",%s",ip,port);
    if (IOLTE_OK != IOLtecommand(sendbuf, "+QMTOPEN: 0,0", "yy", IOLTE_CMD_TIMEOUT+3000, 3, &response)) 
    {
        if (IOLTE_OK != IOLtecommand("AT+QMTCLOSE=0", "+QMTCLOSE: 0,0", "yy", IOLTE_CMD_TIMEOUT+3000, 3,  &response))
            return IOLTE_FAILURE;
        if (IOLTE_OK != IOLtecommand(sendbuf, "+QMTOPEN: 0,0", "yy", IOLTE_CMD_TIMEOUT+3000, 3,  &response))
            return IOLTE_FAILURE;
    }
    
    memset(sendbuf,0,sizeof(sendbuf));
    sprintf(sendbuf,"AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"",clientID,username,password);
    if (IOLTE_OK != IOLtecommand(sendbuf, "+QMTCONN: 0,0,0", "yy", IOLTE_CMD_TIMEOUT, 3,  &response)) return IOLTE_FAILURE;
    log4g("%s","connect OK.");
    
    return IOLTE_OK;
    */
}
byte N716lib::connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password,uint8_t keepalive) {
    return connectMqttServer(clientID,ip,port,username,password,keepalive,1,NULL,NULL);
}
byte N716lib::connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password) {
    return connectMqttServer(clientID,ip,port,username,password,60,1,NULL,NULL);
}
byte N716lib::connectMqttServer(char *clientID,char *ip,char *port) {
    return connectMqttServer(clientID,ip,port,NULL,NULL,60,1,NULL,NULL);
}
/*
取消订阅主题
*/
byte N716lib::UnSubTopic(char *topic,uint8_t qos1)
{
  char data_buff[300];//格式化AT指令时使用
  String response = "";  
  log4g("%s","UnSubTopic");
  if (IOLTE_OK == IOLtecommand("AT+MQTTSTATE?","+MQTTSTATE: 1","yy",5000,5,NULL)) 
  {
    //订阅需要的主题
    sprintf(data_buff,"AT+MQTTUNSUB=\"%s\"",topic);
    if (IOLtecommand(data_buff,"OK","yy",5000,5,&response) <= IOLTE_OK) return IOLTE_FAILURE;
        return IOLTE_OK;
  }
  return IOLTE_FAILURE;
}
/*
订阅主题
AT+MQTTSUB=<"topicname">,<qos><CR> 

响应时间 
    最大响应时间 30s。 
参数 
    <"topicname">  订阅的主题，最大长度 128。 
    <qos>  服务质量，目前支持 Qos=0,1 和 2 

*/
byte N716lib::SubTopic(char *topic,uint8_t qos1)
{
  char data_buff[300];//格式化AT指令时使用
    
  log("SubTopic: ");
  log4g("%s",topic);
  if (IOLtecommand("AT+MQTTSTATE?","+MQTTSTATE: 1","yy",5000,5,NULL) > IOLTE_OK) 
  {
    //订阅需要的主题
    sprintf(data_buff,"AT+MQTTSUB=\"%s\",%d",topic,qos1);
    if (IOLtecommand(data_buff,"OK","yy",5000,5,NULL) <= IOLTE_OK) return IOLTE_FAILURE;
        return IOLTE_OK;
  }
  return IOLTE_FAILURE;     
}
/*
发布主题

 AT+MQTTPUB=<retained>,<qos>,<"topicname">,<"message"><CR>
  响应时间  
    最大响应时间 30s。
  参数 
    <retained>     保留标志，数字类型，0 和 1。 
    <qos>          服务质量，目前仅支持 Qos=0，1 和 2。 
    <"topicname">  发布的主题，最大长度 128。 
    <"message">    发布的消息，最大长度 1024。 
*/
byte N716lib::PubTopic(char *topic,char* msg,uint8_t qos1)//typ_NEC_AQData  *AQData_Send,
{
  char data_buff[300];//格式化AT指令时使用
  String response = "";  
  String reply = "";

  log4g("%s","PubTopic");
  pubsub_f = 1;
  if (IOLtecommand("AT+MQTTSTATE?","+MQTTSTATE: 1","yy",5000,5,NULL) > IOLTE_OK) {
    sprintf(data_buff,"AT+MQTTPUBS=%d,%d,\"%s\",%d",1,qos1,topic,strlen(msg));
    if (IOLtecommand(data_buff,">","yy",5000,5,&response) <= IOLTE_OK) return IOLTE_FAILURE;
    reply+=response;
    if (IOLtecommand(msg,"OK","yy",5000,5,&response) <= IOLTE_OK) return IOLTE_FAILURE;
    reply+=response;
    pubsub_f = 0;
    ParseMsg(reply);
    return IOLTE_OK;
  }
  return IOLTE_FAILURE;
}
bool N716lib::connected(){
    return connected_state;
}
byte N716lib::ParseMsg(String data){
    String reply = data;
    if(pubsub_f == 0){
        if(data.indexOf("+MQTTSUB:") >= 0){
            log4g("%s","*******************************");
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
                commaPosition = reply.indexOf("+MQTTSUB:");
                if(commaPosition>=0){
                    reply = reply.substring(commaPosition,reply.length());
                    for(uint8_t i = 0;i<3;i++){
                        commaPosition = reply.indexOf(',');//检测字符串中的逗号
                    
                        if(commaPosition != -1){//如果有逗号存在就向下执行
                            if(i == 1){//获取topic
                                log4g("%s", reply.substring(1,commaPosition-1).c_str());//打印出第一个逗号位置的字符串
                                reply.substring(1,commaPosition-1).toCharArray(topic,reply.substring(1,commaPosition-1).length()+1);
                            }
                            else if(i == 2){//获取msg长度
                                log4g("%s", reply.substring(0,commaPosition));//打印出第一个逗号位置的字符串
                                reply.substring(0,commaPosition).toCharArray(payload_len,reply.substring(0,commaPosition).length()+1);
                            }
                            reply = reply.substring(commaPosition+1, reply.length());//打印字符串，从当前位置+1开始
                            log4g("%s",reply.c_str());
                        }
                    }
                    //log4g("%s",reply.c_str());
                    if(atoi(payload_len) != 0){//获取topic的内容
                        reply.substring(0,reply.length()).toCharArray(msg,atoi(payload_len)+1);
                        log4g("%s",msg);
                        if(subdatecallback != NULL)
                            subdatecallback(topic,msg,atoi(payload_len),0);
                        //log4g("====================================");
                    }  
                    //log4g("%s",reply.c_str());
                }
            }while(reply.indexOf("+MQTTSUB:")>=0);
        }
        if(data.indexOf("+MQTTDISCONNED: Link Closed")>=0){
            connected_state = false;
            if(eventcallback != NULL) eventcallback(MQTT_DISCONNECT);
        }
    }
   return 0;
}
void N716lib::loop()//typ_NEC_AQData  *AQData_Send,
{    
    String reply = "";
    IOLtewaitFor(NULL,NULL,0, &reply);
}
byte N716lib::http_send(String url,String port,String api,byte mode,String data,uint32_t datalen,byte data_type,uint32_t timeout, int repetitions, String *response,uint32_t offsize,uint32_t size){
    char data_buff[300];//格式化AT指令时使用
    String reply = "";
    String urlapi = url+api;
    log4g("%s",urlapi);
    sprintf(data_buff,"AT+HTTPPARA=url,%s",urlapi.c_str());
    if (IOLtecommand(data_buff,"OK","yy",5000,5,NULL) <= IOLTE_OK) return IOLTE_FAILURE;
    else{
        memset(data_buff,0,sizeof(data_buff));
        sprintf(data_buff,"AT+HTTPPARA=port,%s",port.c_str());
        if (IOLtecommand(data_buff,"OK","yy",5000,5,NULL) <= IOLTE_OK) return IOLTE_FAILURE;
        else{//设置http连接
            if (IOLtecommand("AT+HTTPSETUP","OK","yy",5000,5,NULL) <= IOLTE_OK) return IOLTE_FAILURE;
            else{
                memset(data_buff,0,sizeof(data_buff));
                sprintf(data_buff,"AT+HTTPACTION=%d,%d,%d",mode,datalen,data_type);
                if (IOLtecommand(data_buff,">","yy",5000,5,NULL) <= IOLTE_OK) return IOLTE_FAILURE;
                else{
                    if (IOLtecommand(data.c_str(),"OK","yy",5000,5,NULL) <= IOLTE_OK) return IOLTE_FAILURE;
                    else{
                        if(IOLtewaitFor("+HTTPRECV","+HTTPCLOSED",timeout, &reply) > IOLTE_OK){
                            if(reply.indexOf("+HTTPRECV")>=0){
                                log4g("%s","*******************************");
                                log4g("%s",reply);
                                log4g("%s","*******************************");
                                *response = reply;
                                return IOLTE_OK+1;
                            }
                        }
                        else return IOLTE_FAILURE;//等待数据返回
                    }
                }
            }
        }
    }
    return IOLTE_FAILURE;
}
byte N716lib::http_post(String url,String port,String api,String data,uint32_t datalen,byte data_type,String *response){
    return http_send(url,port,api,POST,data,datalen,data_type,30000, 0, response,0,0);
}
byte N716lib::http_post(String url,String port,uint32_t datalen,byte data_type){
    return 0;
}

