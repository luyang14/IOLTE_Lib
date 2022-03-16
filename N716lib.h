#ifndef N716lib_h
#define N716lib_h

#include <Arduino.h>
#include <iolte_base.h>

#ifndef TOPIC_MAX_LEN
#define TOPIC_MAX_LEN   128
#endif

#ifndef PAYLOAD_MAX_LEN 
#define PAYLOAD_MAX_LEN 1024
#endif

#define GET        0 
#define HEAD       1
#define POST       2 
#define OPEN_MODE  99

#define http_x_www_form_urlencoded 0
#define http_text  1
#define http_json  2
#define http_xml   3
#define http_html  4

class N716lib : public IOLTE{
public:
    N716lib(Stream &serial);
    ~N716lib();
    byte ParseMsg(String data);
    byte begin();
    byte connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password,uint8_t keepalive,uint8_t will_qos,char *will,char *will_payload);
    byte connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password,uint8_t keepalive);
    byte connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password);
    byte connectMqttServer(char *clientID,char *ip,char *port);
    byte SubTopic(char *topic,uint8_t qos1);
    byte PubTopic(char *topic,char* msg,uint8_t qos1);
    byte UnSubTopic(char *topic,uint8_t qos1);
    void attachsubcallback(subcallbackFunction newFunction);
    void attachtcpcallback(recallbackFunction newFunction);
    void attacheventcallback(eventcallbackFunction newFunction);
    bool connected();
    void loop();
    byte http_send(String url,String port,String api,byte mode,String data,uint32_t datalen,byte data_type,uint32_t timeout, int repetitions, String *response,uint32_t offsize,uint32_t size);
    byte http_post(String url,String port,String api,String data,uint32_t datalen,byte data_type,String *response);
    byte http_post(String url,String port,uint32_t datalen,byte data_type);

    String Read_TimeClock();
    int  Read_Signal();
    byte Read_CCID(char *DataBuf);
    byte Read_IMEI(char *DataBuf);
private:
    recallbackFunction getdatecallback = NULL;
    subcallbackFunction subdatecallback = NULL;
    eventcallbackFunction eventcallback = NULL;
    bool connected_state = false;
};
#endif
