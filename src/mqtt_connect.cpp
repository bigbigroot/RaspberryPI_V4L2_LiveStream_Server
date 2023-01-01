/**
 * @file signalingChannel.cpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "mqtt_connect.hpp"

#include <stdexcept>

#define CLIENT_ID   "camera"

void connectLostHandle(void *context, char *cause)
{
    MQTT_LOG("Connect Lost: %s", cause);
}

int messageArrivedHandle(void *context, char *topicName, int topicLen, 
MQTTClient_message *msg)
{
    std::string topic;
    if(topicLen == 0)
    {
        topic = std::string(topicName);
    }else
    {
        topic = std::string(topicName, topicLen); 
    }
    

    std::string message((char *)msg->payload, msg->payloadlen);
    
    MQTT_LOG("message come in. topic: %s(%d)", topicName, topicLen);
    MqttConnect *conn = (MqttConnect *)context;
    conn->handleMessage(topic, message);

    MQTTClient_freeMessage(&msg);
    MQTTClient_free(topicName);
    return 1;
}

// void deliveredHandle(void *context, MQTTClient_deliveryToken dt)
// {

// }

MqttConnect::MqttConnect(const std::string& url, const std::string& clientId, 
                         const std::string& username, const std::string& password)
{
    int rc;
    MQTTClient_create(&client, url.c_str(), clientId.c_str(),
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    connOpt = MQTTClient_connectOptions_initializer;
    connOpt.username = username.c_str();
    connOpt.password = password.c_str();

    MQTTClient_setCallbacks(client, this, connectLostHandle,
                            messageArrivedHandle, nullptr);

    if ((rc = MQTTClient_connect(client, &connOpt)) != MQTTCLIENT_SUCCESS)
    {
        throw std::runtime_error("Failed to connect, return code "+ std::to_string(rc));
    }
}

MqttConnect::~MqttConnect()
{
    MQTT_LOG("mqtt close.");
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}

void MqttConnect::registeTopicHandle(std::string topic, onMessageCallback callback)
{
    allTopicHandles.insert_or_assign(topic, callback);
}

int MqttConnect::publishMessage(std::string topic, std::string message) noexcept
{
    MQTTClient_message mqttMsg(MQTTClient_message_initializer);
    mqttMsg.payload = (void *)message.c_str();
    mqttMsg.payloadlen = message.length();
    return MQTTClient_publishMessage(client, topic.c_str(), &mqttMsg, nullptr);
}

int MqttConnect::subscribeTopic(std::string topic) noexcept
{
    return MQTTClient_subscribe(client, topic.c_str(), 0);
}

int MqttConnect::unsubscribeTopic(std::string topic) noexcept
{
    return MQTTClient_unsubscribe(client, topic.c_str());
}

void MqttConnect::handleMessage(std::string topic, std::string message)
{
    auto it = allTopicHandles.find(topic);
    if(it != allTopicHandles.end())
    {
        it->second(topic, message);
    }
}