#ifndef EC20xlib_h
#define EC20xlib_h

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

#define EC20x_OK          0
#define EC20x_NOTOK       1
#define EC20x_TIMEOUT     2
#define EC20x_FAILURE     3
#define EC20x_DISCONNECT  4

#define EC20x_CMD_TIMEOUT 2000

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
    
    byte connectMqttServer(char *clientID,char *ip,char *port,char *username,char *password);
    byte SubTopic(char *topic,uint8_t qos1);
    byte PubTopic(char *topic,char* msg,uint8_t qos1);
    byte UnSubTopic(char *topic,uint8_t qos1);
    byte getMessage(char *topic,char* msg,uint8_t qos1);
    byte Read_CCID(char *DataBuf);
    byte Read_IMEI(char *DataBuf);
private:
    Stream* EC20xconn;
    String read();
    byte EC20xcommand(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response);
    byte EC20xwaitFor(const char *resp1, const char *resp2, int timeout, String *response);
    long detectRate();
    char setRate(long baudRate);
};
#endif
