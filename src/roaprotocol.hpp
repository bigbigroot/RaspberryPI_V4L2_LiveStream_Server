/**
 * @file protocol.hpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-12-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>
#include <map>
#include <memory>
#include <string>
#include <functional>

#include "mqtt_connect.hpp"
#include "random_id.hpp"



enum class ROAPMessageType : uint8_t
{
    Invaild=0xff,
    Offer=0,
    Answer,
    Ok,
    Error,
    Shutdown
};

enum class ROAPMessageErrorType : uint8_t
{
    Invaild=0xff,
    NoMatch=0,
    Timeout,
    Refused,
    Conflict,
    DoubleConflict,
    Failed
};

enum class ROAPSessionState : uint8_t
{
    Start,
    WaitAnswer,
    WaitCompleted,
    Completed,
    WaitForShutdown,
    Closed
};

class ROAPMessage
{
    public:
        ROAPMessageType messageType=ROAPMessageType::Invaild;
        ROAPMessageErrorType errorType=ROAPMessageErrorType::Invaild;
        std::string offererSessionId;
        std::string answererSessionId;
        uint32_t seq=0;
        std::string sdp;
        ROAPMessage()=default;
        void parser(std::string data);
        std::string toString();
};

class ROAPSession
{
    protected:
        std::string myId;
        std::string yourId;
        uint32_t currentSeq;
        ROAPSessionState state;
        std::string remoteSdp;
        std::string localSdp;

    public:
        ROAPSession(const std::string& id);
        ROAPSession(ROAPSession&& session);
        ~ROAPSession()=default;
};


class OfferSession: public ROAPSession
{
    private:
        std::shared_ptr<MqttConnect> mqttConn;
    public:
        OfferSession(const std::string& id, const std::shared_ptr<MqttConnect>& conn);
        OfferSession(OfferSession&& session);
        ~OfferSession()=default;
        std::function<void(std::string sdp)> onRemoteSDP=nullptr;
        std::function<void()> onClose=nullptr;
        bool isclosed()  noexcept {return state == ROAPSessionState::Closed;}
        std::string getId()  noexcept {return myId;}
        void sendOffer(std::string sdp);
        void processMessage(ROAPMessage &in);
        std::string& getRemoteSdp(){return remoteSdp;}
        void close();
        // void reset();
};


#endif /* __PROTOCOL_H */