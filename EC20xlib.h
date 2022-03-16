#ifndef EC20xlib_h
#define EC20xlib_h

#include <Arduino.h>
#include <iolte_base.h>

#ifndef TOPIC_MAX_LEN
#define TOPIC_MAX_LEN   128
#endif

#ifndef PAYLOAD_MAX_LEN 
#define PAYLOAD_MAX_LEN 1024
#endif

#define USE_HTTP 1
#define USE_MQTT 1
#define USE_TCP  1

//#define EC20xconn Serial2

class EC20xlib : public IOLTE{
public:
    EC20xlib(Stream &serial);
    ~EC20xlib();
    byte begin();
    int connect(char *ip, char *port,uint8_t connectID);
    int disconnect(uint8_t connectID);
    boolean connected();
    byte ParseMsg(String data);
    int  postdata(char* buf, int buflen,uint8_t connectID);
    byte connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password,uint8_t keepalive,uint8_t will_qos,char *will,char *will_payload);
    byte connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password,uint8_t keepalive);
    byte connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password);
    byte connectMqttServer(char *clientID,char *ip,char *port);

    byte SubTopic(char *topic,uint8_t qos1);
    byte PubTopic(char *topic,char* msg,uint8_t qos1);
    byte UnSubTopic(char *topic,uint8_t qos1);
    byte getMessage(char *topic,char* msg,uint8_t qos1);
   
    void attachsubcallback(subcallbackFunction newFunction);
    void attachtcpcallback(recallbackFunction newFunction);
    void attacheventcallback(eventcallbackFunction newFunction);
    void loop();
    String Read_TimeClock();
    int  Read_Signal();
    byte Read_CCID(char *DataBuf);
    byte Read_IMEI(char *DataBuf);
private:
    Stream* EC20xconn;
    uint8_t resend_state;
    char *ip;
    char *port;
    bool connected_state = false;
    recallbackFunction getdatecallback = NULL;
    subcallbackFunction subdatecallback = NULL;
    eventcallbackFunction eventcallback = NULL;
};
#endif
