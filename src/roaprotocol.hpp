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
#include <variant>
#include <string>
#include <functional>

#include "random_id.hpp"

using OnRemoteCallback=std::function<void(std::string)>;

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
    private:
        std::string offererSessionId;
        std::string answererSessionId;
        OnRemoteCallback onRemoteSDP=nullptr;
        uint32_t currentSeq;
        ROAPSessionState state;
        std::string remoteSdp;
        std::string localSdp;
        static RandomID randomIdGen;
    public:
        ROAPSession();
        ~ROAPSession()=default;
        void setRemoteSDPCallback(OnRemoteCallback&& callback){onRemoteSDP=callback;}
        std::string sendOffer(std::string sdp);
        bool processMessage(ROAPMessage &in,ROAPMessage &out);
        std::string& getRemoteSdp(){return remoteSdp;}
        void reset();
};


#endif /* __PROTOCOL_H */