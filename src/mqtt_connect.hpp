/**
 * @file signalingchannnel.h
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __SIGNALINGCHANNEL_H
#define __SIGNALINGCHANNEL_H

#include <string>
#include <functional>
#include <map>

#include <MQTTClient.h>

#define MQTT_INFO

#ifdef MQTT_INFO
    #define MQTT_LOG(...)   do{\
                                printf("MQTT info: ");\
                                printf(__VA_ARGS__);\
                                printf("\n");\
                            }while (0)
                                
#else
    #define V4L2_MESSAGE(...) do{}while (0)
#endif

using onMessageCallback = std::function<void(std::string topic, std::string msg)>;

class MqttConnect
{
    private:
        MQTTClient client;
        MQTTClient_connectOptions connOpt;
        onMessageCallback handler;
    public:
        MqttConnect()=delete;
        MqttConnect(const std::string& url, const std::string& clientId, 
                    const std::string& username, const std::string& password);
        ~MqttConnect();
        onMessageCallback onMessage;
        int publishMessage(std::string topic, std::string message) noexcept;
        int subscribeTopic(std::string topic) noexcept;
        int unsubscribeTopic(std::string topic) noexcept;
};

class NotifyMessage
{
    private:
    public:
        NotifyMessage()=default;
        ~NotifyMessage()=default;
};

#endif /* __SIGNALINGCHANNEL_H */

