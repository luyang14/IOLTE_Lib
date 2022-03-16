#ifndef IOLTE_BASE_h
#define IOLTE_BASE_h

#include <Arduino.h>

#define DEBUG
#ifdef DEBUG
#define log(msg) Serial.print(msg)
#define logln(msg) Serial.println(msg)
#else
#define log(msg)
#define logln(msg)
#endif

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define IOLTE_NOTOK       0
#define IOLTE_TIMEOUT     1
#define IOLTE_FAILURE     2
#define IOLTE_DISCONNECT  3
//保持OK是最大值且不超过120
#define IOLTE_OK          4

#define IOLTE_CMD_TIMEOUT 2000

#define STSTE_SEND 1
#define STSTE_RE   0

enum EVENT
{
    MQTT_CONNECTED,
    MQTT_DISCONNECT,
    MQTT_CONNECT_TIMEOUT,
    TCP_CONNECTED,
    TCP_DISCONNECT,
    TCP_CONNECT_TIMEOUT,
    HTTP_CONNECT_TIMEOUT
};

typedef struct
{
    char ip[20];          //服务器Ip地址
    char port[10];        //端口号
} httpType;

//extern "C" {
typedef byte (*redatacheckFunction)(String msg);
typedef byte (*callbackFunction)(char *topic,char* msg,int buflen,uint8_t qos1);
typedef byte (*recallbackFunction)(char* buf, int buflen,uint8_t connectID);
typedef byte (*subcallbackFunction)(char *topic,char* msg,int buflen,uint8_t qos1);
typedef byte (*eventcallbackFunction)(uint8_t event);
//}
//#define EC20xconn Serial2

class IOLTE {
public:
    IOLTE(Stream &serial);
    ~IOLTE();
    virtual byte ParseMsg(String data) = 0;
    byte initcom();
    byte blockUntilReady(long baudRate);
    
    void powerCycle(int pin);
    void powerOn(int pin);
    void powerOff(int pin);

    byte IOLtecommand(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response);
    byte IOLtecommand(const char *command,uint16_t commandlen, const char *resp1, const char *resp2, int timeout, int repetitions, String *response);
    byte IOLtewaitFor(const char *resp1,const char *resp2,const char *resp3,const char *resp4, const char *resp5, int timeout, String *response);
    byte IOLtewaitFor(const char *resp1, const char *resp2, int timeout, String *response);
    int connect(char *ip, char *port,uint8_t connectID);
    int disconnect(uint8_t connectID);
    uint8_t connected();

    void attachredatacallback(redatacheckFunction newFunction);
    void attachcallback(callbackFunction newFunction);
    void loop();
private:
    Stream* iolte_conn;
    uint8_t resend_state;
    char *ip;
    char *port;
    String read();
    
    long detectRate();
    char setRate(long baudRate);
    callbackFunction datecallback = NULL;
    redatacheckFunction datecallback2 = NULL; 
};
#endif
