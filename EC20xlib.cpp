#include <Arduino.h>
#include "EC20xlib.h"

#define min _min
#define max _max


/////////////////////////////////////////////
// Public methods.
//

EC20xlib::EC20xlib(Stream &serial) {
#ifndef ESP32
#ifdef ESP8266
    EC20xconn = new SoftwareSerial(receivePin, transmitPin, false, 1024);
#else
    EC20xconn = new SoftwareSerial(receivePin, transmitPin, false);

#endif
    EC20xconn->setTimeout(100);
#else
    EC20xconn = &serial;
    
    EC20xconn->setTimeout(100);//阻塞时间
#endif
}


EC20xlib::~EC20xlib() {

    //delete EC20xconn;
}


// Block until the module is ready.
byte EC20xlib::blockUntilReady(long baudRate) {

    byte response = EC20x_NOTOK;
    while (EC20x_OK != response) {
        response = begin();
        // This means the modem has failed to initialize and we need to reboot
        // it.
        if (EC20x_FAILURE == response) {
            return EC20x_FAILURE;
        }
        delay(1000);
        logln("Waiting for module to be ready...");
    }
    return EC20x_OK;
}


// Initialize the software serial connection and change the baud rate from the
// default (autodetected) to the desired speed.
byte EC20xlib::begin() {

    EC20xconn->flush();

    // Factory reset.
    if (EC20x_OK != EC20xcommand("AT", "OK", "yy", EC20x_CMD_TIMEOUT, 20, NULL)) return EC20x_FAILURE;

    // Echo off.
    if (EC20x_OK != EC20xcommand("ATE0", "OK", "yy", EC20x_CMD_TIMEOUT, 2, NULL)) return EC20x_FAILURE;

    // Echo off.
    if (EC20x_OK != EC20xcommand("ATI", "OK", "yy", EC20x_CMD_TIMEOUT, 2, NULL)) return EC20x_FAILURE;
    // 检查SIM卡是否在位
    if (EC20x_OK != EC20xcommand("AT+CPIN?", "+CPIN: READY", "yy", EC20x_CMD_TIMEOUT, 2, NULL)) return EC20x_FAILURE;

    // 检查CSQ
    if (EC20x_OK != EC20xcommand("AT+CSQ", "OK", "yy", EC20x_CMD_TIMEOUT, 2, NULL)) return EC20x_FAILURE;

    // 查看是否注册GSM网络
    if (EC20x_OK != EC20xcommand("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", EC20x_CMD_TIMEOUT, 20, NULL)) return EC20x_FAILURE;

    // 查看是否注册GPRS网络
    if (EC20x_OK != EC20xcommand("AT+CGREG?", "+CGREG: 0,1", "+CGREG: 0,5", EC20x_CMD_TIMEOUT, 20, NULL)) return EC20x_FAILURE;

    return EC20x_OK;
}


// Reboot the module by setting the specified pin HIGH, then LOW. The pin should
// be connected to a P-MOSFET, not the EC20x's POWER pin.
void EC20xlib::powerCycle(int pin) {
    logln("Power-cycling module...");

    powerOff(pin);

    delay(2000);

    powerOn(pin);

    // Give the module some time to settle.
    logln("Done, waiting for the module to initialize...");
    delay(20000);
    logln("Done.");

    EC20xconn->flush();
}


// Turn the modem power completely off.
void EC20xlib::powerOff(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}


// Turn the modem power on.
void EC20xlib::powerOn(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

// Get the strength of the GSM signal.
int EC20xlib::getSignalStrength() {
    String response = "";
    uint32_t respStart = 0;
    int strength, error  = 0;

    // Issue the command and wait for the response.
    EC20xcommand("AT+CSQ", "OK", "+CSQ", EC20x_CMD_TIMEOUT, 2, &response);

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
    re=EC20xcommand("AT+CCID\r\n","+CCID","yy",EC20x_CMD_TIMEOUT, 2, &response);
    Serial.println(response);
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
    re = EC20xcommand("AT+GSN","OK","yy",EC20x_CMD_TIMEOUT, 2, &response);
    Serial.println(response);
    
    response = response.substring(2, 18);
    Serial.println(response);
    response.toCharArray(DataBuf,16,0);
    Serial.println(DataBuf);
    return re;
}

// Get the real time from the modem. Time will be returned as yy/MM/dd,hh:mm:ss+XX
String EC20xlib::getRealTimeClock() {
    String response = "";

    // Issue the command and wait for the response.
    EC20xcommand("AT+CCLK?", "OK", "yy", EC20x_CMD_TIMEOUT, 1, &response);
    int respStart = response.indexOf("+CCLK: \"") + 8;
    response.setCharAt(respStart - 1, '-');

    return response.substring(respStart, response.indexOf("\""));
}

/////////////////////////////////////////////
// Private methods.
//

// Read some data from the EC20x in a non-blocking manner.
String EC20xlib::read() {
    String reply = "";
    if (EC20xconn->available()) {
        reply = EC20xconn->readString();
    }

    // XXX: Replace NULs with \xff so we can match on them.
    for (int x = 0; x < reply.length(); x++) {
        if (reply.charAt(x) == 0) {
            reply.setCharAt(x, 255);
        }
    }
    return reply;
}

// 发送AT命令
// Issue a command.
byte EC20xlib::EC20xcommand(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response) {
    byte returnValue = EC20x_NOTOK;
    byte count = 0;

    // Get rid of any buffered output.
    EC20xconn->flush();

    while (count < repetitions && returnValue != EC20x_OK) {
        log("Issuing command: ");
        logln(command);

        EC20xconn->write(command);
        EC20xconn->write('\r');

        if (EC20xwaitFor(resp1, resp2, timeout, response) == EC20x_OK) {
            returnValue = EC20x_OK;
        } else {
            returnValue = EC20x_NOTOK;
        }
        count++;
    }
    return returnValue;
}


// Wait for responses.
byte EC20xlib::EC20xwaitFor(const char *resp1, const char *resp2, int timeout, String *response) {
    unsigned long entry = millis();
    String reply = "";
    byte retVal = 99;
    do {
        reply += read();
#ifdef ESP8266
        yield();
#endif
    } while (((reply.indexOf(resp1) + reply.indexOf(resp2)) == -2) && ((millis() - entry) < timeout));

    if (reply != "") {
        log("Reply in ");
        log((millis() - entry));
        log(" ms: ");
        logln(reply);
    }

    if (response != NULL) {
        *response = reply;
    }

    if ((millis() - entry) >= timeout) {
        retVal = EC20x_TIMEOUT;
        logln("Timed out.");
    } else {
        if (reply.indexOf(resp1) + reply.indexOf(resp2) > -2) {
            logln("Reply OK.");
            retVal = EC20x_OK;
        } else {
            logln("Reply NOT OK.");
            retVal = EC20x_NOTOK;
        }
    }
    return retVal;
}

// Read some data from the EC20x in a non-blocking manner.
byte EC20xlib::connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password) {
    char sendbuf[100];
    String response = "";

    uint32_t respStart = 0;
    int client_idx = 0, error = 0,result = 0;
    for(int i = 0; i<2;i++)
    {
        sprintf(sendbuf,"AT+QMTOPEN=0,\"%s\",%s",ip,port);
        if (EC20x_OK == EC20xcommand(sendbuf, "+QMTOPEN:", "yy", EC20x_CMD_TIMEOUT+3000, 3, &response)) 
        {
            respStart = response.indexOf("+QMTOPEN:");
            if (respStart < 0) {
                return EC20x_FAILURE;
            }
            sscanf(response.substring(respStart).c_str(), "+QMTOPEN: %d,%d",
            &client_idx, &error);
            
            if(error == EC20x_OK)//连接成功
            {
                sprintf(sendbuf,"AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"",clientID,username,password);
                if (EC20x_OK == EC20xcommand(sendbuf, "+QMTCONN:", "yy", EC20x_CMD_TIMEOUT+3000, 3,  &response))
                {
                    respStart = response.indexOf("+QMTCONN:");
                    if (respStart < 0) {
                        return EC20x_FAILURE;
                    }
                    sscanf(response.substring(respStart).c_str(), "+QMTCONN: %d,%d,%d",
                    &client_idx,&result, &error);
                    if(error == EC20x_OK)
                    {
                        return EC20x_OK;
                        logln("connect OK.");
                    }
                    else return EC20x_FAILURE;
                }
                else return EC20x_FAILURE;
            }
            else if(error == 2)//MQTT 标识符被占用,需要先断开重新连接
            {
                if (EC20x_OK != EC20xcommand("AT+QMTCLOSE=0", "+QMTCLOSE: 0,0", "yy", EC20x_CMD_TIMEOUT+3000, 3,  &response))
                    return EC20x_FAILURE;
            }
            else return EC20x_FAILURE;
        }
        else return EC20x_FAILURE;
    }
    return EC20x_FAILURE;       
    /*
    sprintf(sendbuf,"AT+QMTOPEN=0,\"%s\",%s",ip,port);
    if (EC20x_OK != EC20xcommand(sendbuf, "+QMTOPEN: 0,0", "yy", EC20x_CMD_TIMEOUT+3000, 3, &response)) 
    {
        if (EC20x_OK != EC20xcommand("AT+QMTCLOSE=0", "+QMTCLOSE: 0,0", "yy", EC20x_CMD_TIMEOUT+3000, 3,  &response))
            return EC20x_FAILURE;
        if (EC20x_OK != EC20xcommand(sendbuf, "+QMTOPEN: 0,0", "yy", EC20x_CMD_TIMEOUT+3000, 3,  &response))
            return EC20x_FAILURE;
    }
    
    memset(sendbuf,0,sizeof(sendbuf));
    sprintf(sendbuf,"AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"",clientID,username,password);
    if (EC20x_OK != EC20xcommand(sendbuf, "+QMTCONN: 0,0,0", "yy", EC20x_CMD_TIMEOUT, 3,  &response)) return EC20x_FAILURE;
    logln("connect OK.");
    
    return EC20x_OK;
    */
}

/*
取消订阅主题
*/
byte EC20xlib::UnSubTopic(char *topic,uint8_t qos1)
{
  char data_buff[300];//格式化AT指令时使用
    
  logln("UnSubTopic");
  //订阅需要的主题
  sprintf(data_buff,"AT+QMTUNS=0,%d,%s",1,topic);
  if (EC20x_OK != EC20xcommand(data_buff,"OK","yy",5000,5,NULL)) return EC20x_FAILURE;
	return EC20x_OK;
}
/*
订阅主题
*/
byte EC20xlib::SubTopic(char *topic,uint8_t qos1)
{
  char data_buff[300];//格式化AT指令时使用
    
  logln("SubTopic");
  //订阅需要的主题
  sprintf(data_buff,"AT+QMTSUB=0,%d,\"%s\",%d",1,topic,qos1);
  if (EC20x_OK != EC20xcommand(data_buff,"OK","yy",5000,5,NULL)) return EC20x_FAILURE;
	return EC20x_OK;
}
/*
发布主题
*/
byte EC20xlib::PubTopic(char *topic,char* msg,uint8_t qos1)//typ_NEC_AQData  *AQData_Send,
{
  char data_buff[300];//格式化AT指令时使用
    
  logln("PubTopic");
  sprintf(data_buff,"AT+QMTPUBEX=0,0,%d,0,\"%s\",%d",qos1,topic,strlen(msg));
  if (EC20x_OK != EC20xcommand(data_buff,">","yy",5000,5,NULL)) return EC20x_FAILURE;
  if (EC20x_OK != EC20xcommand(msg,"OK","yy",5000,5,NULL)) return EC20x_FAILURE;
  return EC20x_OK;
}

/*
发布主题
*/
byte EC20xlib::getMessage(char *topic,char* msg,uint8_t qos1)//typ_NEC_AQData  *AQData_Send,
{  
    unsigned long entry = millis();
    int commaPosition,last_commaPosition;//存储还没有分离出来的字符串
    String reply = "";
    byte retVal = 99;
        reply += read();
#ifdef ESP32
        yield();
#endif
    if (reply != "") {
        log("Reply in ");
        log((millis() - entry));
        log(" ms: ");
        logln(reply);
        if(reply.indexOf("+QMTRECV:") != -1)
        {
            logln("recv data");
            uint8_t i = 0;
            for(i = 0;i<3;i++)
            {
                commaPosition = reply.indexOf(',');//检测字符串中的逗号
            
                if(commaPosition != -1)//如果有逗号存在就向下执行
                {
                    if(i == 2)//获取topic
                    {
                       logln( reply.substring(1,commaPosition-1));//打印出第一个逗号位置的字符串
                        reply.substring(1,commaPosition-1).toCharArray(topic,reply.substring(1,commaPosition-1).length()+1);
                    }
                    
                    reply = reply.substring(commaPosition+1, reply.length());//打印字符串，从当前位置+1开始
                }
            }
            if(1)//获取topic的内容
            {
                commaPosition = reply.indexOf('"');
                last_commaPosition = reply.lastIndexOf('"');
                logln( reply.substring(commaPosition+1,last_commaPosition));//打印出第一个逗号位置的字符串
                reply.substring(commaPosition+1,last_commaPosition).toCharArray(msg,reply.substring(commaPosition+1,last_commaPosition).length()+1);
            }  
            retVal = EC20x_OK;
        }
        if(reply.indexOf("+QMTSTAT:") != -1)
        {
            logln("mqtt disconnect");
            /*
            <client_idx>  整型。MQTT 客户端标识符。
            <err_code>  整型。错误代码。详细信息请参考下表。 

            1  连接被服务器断开或者重置       执行 AT+QMTOPEN 重建 MQTT 连接 
            2  发送 PINGREQ 包超时或者失败   首先反激活 PDP，然后再激活 PDP 并重建MQTT 连接。 
            3  发送 CONNECT 包超时或者失败   1.  查看输入的用户名和密码是否正确。 
                                            2.  确保客户端 ID 未被占用。 
                                            3.  重建 MQTT 连接，并尝试再次发送CONNECT 包到服务器。 
            4  接收 CONNACK 包超时或者失败   1.  查看输入的用户名和密码是否正确。 
                                            2.  确保客户端 ID 未被占用。 
                                            3.  重建 MQTT 连接，并尝试再次发送CONNECT 包到服务器。 
            5  客户端向服务器发送 DISCONNECT 包，但
               是服务器主动断开 MQTT 连接     正常流程。 
            6  因为发送数据包总是失败，客户端主动断开MQTT 连接      1.  确保数据正确。 
                                                                2.  可能因为网络拥堵或者其他错误，尝试重建 MQTT 连接。 
            7  链路不工作或者服务器不可用  确保当前链路或者服务器可用。 
            8  客户端主动断开 MQTT 连接  尝试重建连接。 
            9~255  留作将来使用   
            */
            //重新连接
            retVal = EC20x_DISCONNECT;
        }
        if(reply.indexOf("+QMTPUBEX:") != -1)
        {
            logln("rev the result of public message");
            /*
            <client_idx>  整型。MQTT 客户端标识符。范围：0~5。
            <msgid>  整型。数据包消息标识符。范围：0~65535。只有当<qos>=0 时，该参数值为 0。
            <result>  整型。命令执行结果。 
            0  数据包发送成功且接收到服务器的 ACK（当<qos>=0 时发布了数据，则无需 ACK） 
            1  数据包重传 
            2  数据包发送失败
            */
            //接收到发送返回
            retVal = EC20x_DISCONNECT;
        }
        
    }
    return retVal;
}
