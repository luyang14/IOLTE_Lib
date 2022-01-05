#ifndef EC20xlib_h
#define EC20xlib_h

#include <Arduino.h>

//#define DEBUG
#ifdef DEBUG
#define log(msg) Serial.print(msg)
#define logln(msg) Serial.println(msg)
#else
#define log(msg)
#define logln(msg)
#endif

#define USE_HTTP 1
#define USE_MQTT 1
#define USE_TCP  1

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define EC20x_OK          0
#define EC20x_NOTOK       1
#define EC20x_TIMEOUT     2
#define EC20x_FAILURE     3
#define EC20x_DISCONNECT  4

#define EC20x_CMD_TIMEOUT 2000

#define STSTE_SEND 1
#define STSTE_RE   0

extern "C" {
typedef byte (*recallbackFunction)(char* buf, int buflen,uint8_t connectID);
typedef byte (*subcallbackFunction)(char *topic,char* msg,int buflen,uint8_t qos1);
}
//#define EC20xconn Serial2

class EC20xlib {
public:
    EC20xlib(Stream &serial);
    ~EC20xlib();

    byte begin();
    byte blockUntilReady(long baudRate);
    
    void powerCycle(int pin);
    void powerOn(int pin);
    void powerOff(int pin);

    int getSignalStrength();
    String getRealTimeClock();
    
    int connect(char *ip, char *port,uint8_t connectID);
    int disconnect(uint8_t connectID);
    uint8_t connected();
    int  postdata(char* buf, int buflen,uint8_t connectID);
    byte connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password);
    byte SubTopic(char *topic,uint8_t qos1);
    byte PubTopic(char *topic,char* msg,uint8_t qos1);
    byte UnSubTopic(char *topic,uint8_t qos1);
    byte getMessage(char *topic,char* msg,uint8_t qos1);
    byte Read_CCID(char *DataBuf);
    byte Read_IMEI(char *DataBuf);
    void attachsubcallback(subcallbackFunction newFunction);
    void attachtcpcallback(recallbackFunction newFunction);
    void loop();
private:
    Stream* EC20xconn;
    uint8_t resend_state;
    char *ip;
    char *port;
    String read();
    byte EC20xcommand(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response);
    byte EC20xcommand(const char *command,uint16_t commandlen, const char *resp1, const char *resp2, int timeout, int repetitions, String *response);
    byte EC20xwaitFor(const char *resp1, const char *resp2, int timeout, String *response);
    long detectRate();
    char setRate(long baudRate);
    recallbackFunction getdatecallback = NULL;
    subcallbackFunction subdatecallback = NULL;
};
#endif
