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

#include "uid.hpp"

#define number_of(array)

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
    Completed,
    WaitForShutdown,
    Closed
};

struct ROAPMessage
{
    ROAPMessageType messageType=ROAPMessageType::Invaild;
    ROAPMessageErrorType errorType=ROAPMessageErrorType::Invaild;
    std::string offererSessionId;
    std::string answererSessionId;
    uint32_t seq=0;
    std::string sdp;
    ROAPMessage()=default;
};

ROAPMessage ROAPMessageParser(std::string data);
/**
 * @brief convert to JSON formate string
 * 
 * @param m 
 * @return std::string 
 */
std::string ROAPMessageToString(ROAPMessage& m);


class ROAPSession
{
    private:
        std::string offererSessionId;
        std::string answererSessionId;
        uint32_t currentSeq;
        ROAPSessionState state;
        std::string sdp;
        static UniqueID uniqueIdGen;
    public:
        ROAPSession();
        ~ROAPSession()=default;
        // std::function<int(std::string)> sendMessage=nullptr;
        std::string createOffer(std::string sdp);
        std::string sendOffer(std::string sdp);
        void process(ROAPMessage &in,ROAPMessage &out);
        std::string& getRemoteSdp(){return sdp;}
};


#endif /* __PROTOCOL_H */