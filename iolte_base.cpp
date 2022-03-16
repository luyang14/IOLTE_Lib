#include <Arduino.h>
#include "iolte_base.h"

#define min _min
#define max _max

/////////////////////////////////////////////
// Public methods.
//

IOLTE::IOLTE(Stream &serial) {
#ifndef ESP32
#ifdef ESP8266
    iolte_conn = new SoftwareSerial(receivePin, transmitPin, false, 1024);
#else
    iolte_conn = new SoftwareSerial(receivePin, transmitPin, false);

#endif
    iolte_conn->setTimeout(100);
#else
    iolte_conn = &serial;
    
    iolte_conn->setTimeout(50);//阻塞时间
#endif
}


IOLTE::~IOLTE() {

    //delete iolte_conn;
}


// Block until the module is ready.
byte IOLTE::blockUntilReady(long baudRate) {

    byte response = IOLTE_NOTOK;
    while (IOLTE_OK != response) {
        response = initcom();
        // This means the modem has failed to initialize and we need to reboot
        // it.
        if (IOLTE_FAILURE == response) {
            return IOLTE_FAILURE;
        }
        delay(1000);
        logln("Waiting for module to be ready...");
    }
    return IOLTE_OK;
}


// Initialize the software serial connection and change the baud rate from the
// default (autodetected) to the desired speed.
byte IOLTE::initcom() {

    iolte_conn->flush();

    // Factory reset.
    if (IOLtecommand("AT", "OK", "yy", IOLTE_CMD_TIMEOUT, 20, NULL) <= IOLTE_OK) return IOLTE_FAILURE;

    // Echo off.
    if (IOLtecommand("ATE0", "OK", "yy", IOLTE_CMD_TIMEOUT, 2, NULL) <= IOLTE_OK) return IOLTE_FAILURE;

    // Echo off.
    if (IOLtecommand("ATI", "OK", "yy", IOLTE_CMD_TIMEOUT, 2, NULL) <= IOLTE_OK) return IOLTE_FAILURE;

    // 检查SIM卡是否在位
    if (IOLtecommand("AT+CPIN?", "+CPIN: READY", "yy", IOLTE_CMD_TIMEOUT, 2, NULL) <= IOLTE_OK) return IOLTE_FAILURE;

    // 检查CSQ
    if (IOLtecommand("AT+CSQ", "OK", "yy", IOLTE_CMD_TIMEOUT, 2, NULL) <= IOLTE_OK) return IOLTE_FAILURE;

    // 查看是否注册GSM网络
    if (IOLtecommand("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", IOLTE_CMD_TIMEOUT, 20, NULL) <= IOLTE_OK) return IOLTE_FAILURE;

    // 查看是否注册GPRS网络
    if (IOLtecommand("AT+CGREG?", "+CGREG: 0,1", "+CGREG: 0,5", IOLTE_CMD_TIMEOUT, 20, NULL) <= IOLTE_OK) return IOLTE_FAILURE;

    return IOLTE_OK;
}


// Reboot the module by setting the specified pin HIGH, then LOW. The pin should
// be connected to a P-MOSFET, not the EC20x's POWER pin.
void IOLTE::powerCycle(int pin) {
    logln("Power-cycling module...");

    powerOff(pin);

    delay(2000);

    powerOn(pin);

    // Give the module some time to settle.
    logln("Done, waiting for the module to initialize...");
    delay(20000);
    logln("Done.");

    iolte_conn->flush();
}


// Turn the modem power completely off.
void IOLTE::powerOff(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}


// Turn the modem power on.
void IOLTE::powerOn(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

/////////////////////////////////////////////
// Private methods.
//

// Read some data from the EC20x in a non-blocking manner.
String IOLTE::read() {
    String reply = "";
    if (iolte_conn->available()) {
        reply = iolte_conn->readString();
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
byte IOLTE::IOLtecommand(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response) {
    int returnValue = IOLTE_NOTOK;
    byte count = 0;

    // Get rid of any buffered output.
    iolte_conn->flush();

    //loop();
    while ((count < repetitions) && (returnValue < IOLTE_OK)) {
        log("Issuing command: ");
        logln(command);

        iolte_conn->write(command);
        iolte_conn->write('\r');
        returnValue = IOLtewaitFor(resp1, resp2, timeout, response);
        count++;
    }

    return returnValue;
}
/****************************************
* 函数名：EC20_SendCommand
* 描述  ：向GPRS模块发送命令
*         CommandStr 命令字符串
*         Check 检测的命令回复
*         retry 重试次数
*         每次等待命令回复的时间 单位100mS
* 输入  ：无
* 输出  ：无
****************************************/
byte IOLTE::IOLtecommand(const char *command,uint16_t commandlen, const char *resp1, const char *resp2, int timeout, int repetitions, String *response)
{
    byte returnValue = IOLTE_NOTOK;
    byte count = 0;
    resend_state = STSTE_SEND;
    // Get rid of any buffered output.
    iolte_conn->flush();

    while (count < repetitions && returnValue < IOLTE_OK) {
        log("Issuing command: ");
        logln(command);

        iolte_conn->write(command,commandlen);
        iolte_conn->write('\r');
        returnValue = IOLtewaitFor(resp1, resp2, timeout, response);
        count++;
    }
    resend_state = STSTE_RE;
    return returnValue;
}


// Wait for responses.
byte IOLTE::IOLtewaitFor(const char *resp1,const char *resp2,const char *resp3,const char *resp4, const char *resp5, int timeout, String *response) {
    unsigned long entry = millis();
    String reply = "";
    byte retVal = 0;

    do {
        reply += read();
        yield();
#ifdef ESP8266
        yield();
#endif
      if(resp1 && (reply.indexOf(resp1)>=0)){//接收到需要的数据
        retVal = IOLTE_OK+1;
        break;
      }
      else if(resp2 && (reply.indexOf(resp2)>=0)){//接收到需要的数据
        retVal = IOLTE_OK+2;
        break;
      }
      else if(resp3 && (reply.indexOf(resp3)>=0)){//接收到需要的数据
        retVal = IOLTE_OK+3;
        break;
      }
      else if(resp4 && (reply.indexOf(resp4)>=0)){//接收到需要的数据
        retVal = IOLTE_OK+4;
        break;
      }
      else if(resp5 && (reply.indexOf(resp5)>=0)){//接收到需要的数据
        retVal = IOLTE_OK+5;
        break;
      } 
      delay(1);
    } while ((millis() - entry) < timeout);
    
    if (reply != "") {
        log("Reply in ");
        log((millis() - entry));
        log(" ms: ");
        logln(reply);
        if (response != NULL) {
            *response = reply;
        }
        this->ParseMsg(reply);
    }
    if (retVal == 0) retVal = IOLTE_TIMEOUT;       

    /* reply2 = reply;
    if(reply2.indexOf("+QIURC: \"recv\",") != -1)//接收到数据
    {
        logln("recv data");
        uint8_t connectID = 0;
        uint16_t datalen = 0;
        char sendbuf[1000] = "";
        int commaPosition = 0;
        commaPosition = reply2.indexOf("+QIURC: \"recv\",");
        reply2 = reply2.substring(commaPosition,reply2.length());//截取有效数据

        for(uint8_t i =0;i<3;i++){
            commaPosition = reply2.indexOf(",");
            if(commaPosition != -1){//检测到“，”
                if(i == 1){
                    connectID = atoi(reply2.substring(0,commaPosition).c_str());
                    log("connectID:");
                    logln(connectID);
                    datalen = atoi(reply2.substring(commaPosition+1,reply2.indexOf("\r")).c_str());
                    logln("datalen---------------------------------");
                    logln(datalen);
                    break;
                }
                reply2 = reply2.substring(commaPosition+1,reply2.length());
            }
        }
        commaPosition = reply2.indexOf("HTTP/1.1");
        if(commaPosition != -1){//检测到“HTTP/1.1”
            reply2 = reply2.substring(commaPosition,reply2.length());
            reply2.toCharArray(sendbuf,reply2.length()+1,0);
            getdatecallback(sendbuf,datalen,connectID);
        }
    } */
    return retVal;
}

byte IOLTE::IOLtewaitFor(const char *resp1,const char *resp2, int timeout, String *response) {
    return IOLtewaitFor(resp1,resp2,NULL,NULL,NULL, timeout, response);
}
void IOLTE::attachcallback(callbackFunction newFunction){
    datecallback = newFunction;
}

// void IOLTE::loop()
// {  
//     //    uint16_t i=0;
//     uint32_t entry = millis();
//     int commaPosition = 0;//存储还没有分离出来的字符串
//     String reply = "";

//     if(resend_state == STSTE_RE){//发送完成，可以接收
//         reply = read();
// #ifdef ESP32
//         yield();
// #endif
//         if(reply != "") {
//             log("Reply in ");
//             log((millis() - entry));
//             log(" ms: ");
//             logln(reply);
//             /*示例
//             +QIURC: "recv",0,256
//             HTTP/1.1 200 
//             Content-Type: application/json;charset=UTF-8
//             Transfer-Encoding: chunked
//             Vary: Accept-Encoding
//             Date: Wed, 05 Jan 2022 02:27:40 GMT

//             63
//             {"pulseAll":4997520,"dateStr":"2022-01-05 10:27:40","pulseMoney":4997520,"state":"1","pulseFree":0}
//             */
//             if(reply.indexOf("+QIURC: \"recv\",") != -1)//接收到数据
//             {
//                 logln("recv data");
//                 uint8_t connectID = 0;
//                 uint16_t datalen = 0;
//                 char sendbuf[1000] = "";
//                 commaPosition = reply.indexOf("+QIURC: \"recv\",");
//                 reply = reply.substring(commaPosition,reply.length());//截取有效数据

//                 for(uint8_t i =0;i<3;i++){
//                     commaPosition = reply.indexOf(",");
//                     if(commaPosition != -1){//检测到“，”
//                         if(i == 1){
//                             connectID = atoi(reply.substring(0,commaPosition).c_str());
//                             log("connectID:");
//                             logln(connectID);
//                             datalen = atoi(reply.substring(commaPosition+1,reply.indexOf("\r")).c_str());
//                             logln("datalen---------------------------------");
//                             logln(datalen);
//                             break;
//                         }
//                         reply = reply.substring(commaPosition+1,reply.length());
//                     }
//                 }
//                 commaPosition = reply.indexOf("HTTP/1.1");
//                 if(commaPosition != -1){//检测到“HTTP/1.1”
//                     reply = reply.substring(commaPosition,reply.length());
//                     reply.toCharArray(sendbuf,reply.length()+1,0);
//                     //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1datecallback(sendbuf,datalen,connectID);
//                 }
//             }
//             if(reply.indexOf("+QIURC: \"closed\",") != -1)//断开连接
//             {
//                 logln("断开连接");
//             }
//             if(reply.indexOf("+QMTRECV:") != -1)
//             {
//             }    

//         }
//     }
// }