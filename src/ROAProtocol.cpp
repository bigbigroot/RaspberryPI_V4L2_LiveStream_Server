/**
 * @file protocol.cpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-12-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "protocol.hpp"

#include <stdexcept>
#include <array>
#include <nlohmann/json.hpp>

const std::string messageTypeString[] =
{
    "OFFER",
    "ANSWER",
    "OK",
    "ERROR",
    "SHUTDOWN"
};
constexpr size_t ROAPMessageTypeNum = 
sizeof(messageTypeString)/sizeof(std::string);

const std::string messageErrorTypeString[] =
{
    "NOMATCH",
    "TIMEOUT",
    "REFUSED",
    "CONFLICT",
    "DOUBLECONFLICT",
    "FAILED"
};
constexpr size_t ROAPMessageErrorTypeNum = 
sizeof(messageErrorTypeString)/sizeof(std::string);

ROAPMessage ROAPMessageParser(std::string data)
{
    ROAPMessage ret;
    if(nlohmann::json::accept(data))
    {
        std::string value;
        auto rootJosn = nlohmann::json::parse(data);
        if(rootJosn.contains("messageType"))
        {
            value = rootJosn["messageType"].get<std::string>();
            for(size_t i=0; i < ROAPMessageTypeNum; i++)
            {
                if(value.compare(messageTypeString[i]) == 0)
                {
                    ret.messageType = ROAPMessageType(i);
                    break;
                }
            }
        }
        if(rootJosn.contains("errorType"))
        {
            value = rootJosn["errorType"].get<std::string>();
            for(size_t i=0; i < ROAPMessageErrorTypeNum; i++)
            {
                if(value.compare(messageErrorTypeString[i]) == 0)
                {
                    ret.errorType = ROAPMessageErrorType(i);
                    break;
                }
            }
        }
        if(rootJosn.contains("offererSessionId"))
        {
            ret.offererSessionId = rootJosn["offererSessionId"].get<std::string>();
        }
        if(rootJosn.contains("answererSessionId"))
        {
            ret.answererSessionId = rootJosn["answererSessionId"].get<std::string>();
        }
        if(rootJosn.contains("seq"))
        {
            ret.seq = rootJosn["seq"].get<unsigned>();
        }
        if(rootJosn.contains("sdp"))
        {
            ret.sdp = rootJosn["sdp"].get<std::string>();
        }

    }else
    {
        throw std::runtime_error("Invaild Json formate.");
    }
    return ret;
}

std::string ROAPMessageToString(ROAPMessage& m)
{
    nlohmann::ordered_json json;

    switch(m.messageType)
    {
        case ROAPMessageType::Invaild: break;
        case ROAPMessageType::Offer:
        case ROAPMessageType::Answer:
        case ROAPMessageType::Ok:
        case ROAPMessageType::Error:
        case ROAPMessageType::Shutdown:
        {
            json["messageType"] = messageTypeString[uint8_t(m.messageType)];
            break;
        }
    }
    switch(m.errorType)
    {
        case ROAPMessageErrorType::Invaild: break;
        case ROAPMessageErrorType::NoMatch:
        case ROAPMessageErrorType::Timeout:
        case ROAPMessageErrorType::Refused:
        case ROAPMessageErrorType::Conflict:
        case ROAPMessageErrorType::DoubleConflict:
        case ROAPMessageErrorType::Failed:
        {
            json["errorType"] = messageErrorTypeString[uint8_t(m.errorType)];
            break;
        } 
    }
    if(!m.offererSessionId.empty())
    {
        json["offererSessionId"] = m.offererSessionId;
    }
    if(!m.answererSessionId.empty())
    {
        json["answererSessionId"] = m.answererSessionId;
    }
    if(m.seq > 0)
    {
        json["seq"] = m.seq;
    }
    if(!m.sdp.empty())
    {
        json["sdp"] = m.sdp;
    }
    return std::forward<std::string>(json.dump(2));
}