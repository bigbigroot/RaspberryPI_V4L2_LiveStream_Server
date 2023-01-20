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

#include "roaprotocol.hpp"
#include "utility.h"

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

void ROAPMessage::parser(std::string data)
{
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
                    messageType = ROAPMessageType(i);
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
                    errorType = ROAPMessageErrorType(i);
                    break;
                }
            }
        }
        if(rootJosn.contains("offererSessionId"))
        {
            offererSessionId = rootJosn["offererSessionId"].get<std::string>();
        }
        if(rootJosn.contains("answererSessionId"))
        {
            answererSessionId = rootJosn["answererSessionId"].get<std::string>();
        }
        if(rootJosn.contains("seq"))
        {
            seq = rootJosn["seq"].get<unsigned>();
        }
        if(rootJosn.contains("sdp"))
        {
            sdp = rootJosn["sdp"].get<std::string>();
        }

    }else
    {
        throw std::runtime_error("Invaild Json formate.");
    }
}

std::string ROAPMessage::toString()
{
    nlohmann::ordered_json json;

    switch(messageType)
    {
        case ROAPMessageType::Invaild: break;
        case ROAPMessageType::Offer:
        case ROAPMessageType::Answer:
        case ROAPMessageType::Ok:
        case ROAPMessageType::Error:
        case ROAPMessageType::Shutdown:
        {
            json["messageType"] = messageTypeString[uint8_t(messageType)];
            break;
        }
    }
    switch(errorType)
    {
        case ROAPMessageErrorType::Invaild: break;
        case ROAPMessageErrorType::NoMatch:
        case ROAPMessageErrorType::Timeout:
        case ROAPMessageErrorType::Refused:
        case ROAPMessageErrorType::Conflict:
        case ROAPMessageErrorType::DoubleConflict:
        case ROAPMessageErrorType::Failed:
        {
            json["errorType"] = messageErrorTypeString[uint8_t(errorType)];
            break;
        } 
    }
    if(!offererSessionId.empty())
    {
        json["offererSessionId"] = offererSessionId;
    }
    if(!answererSessionId.empty())
    {
        json["answererSessionId"] = answererSessionId;
    }
    if(seq > 0)
    {
        json["seq"] = seq;
    }
    if(!sdp.empty())
    {
        json["sdp"] = sdp;
    }
    return std::forward<std::string>(json.dump(4));
}

ROAPSession::ROAPSession(std::string id):
myId(id), currentSeq(1),
state(ROAPSessionState::Start)
{}

ROAPSession::ROAPSession(ROAPSession&& session):
myId(std::move(session.myId)),
yourId(std::move(session.yourId)),
currentSeq(session.currentSeq), state(session.state),
remoteSdp(std::move(session.remoteSdp)),
localSdp(std::move(session.localSdp))
{}

OfferSession::OfferSession(
    std::string id, std::shared_ptr<MqttConnect> conn):
ROAPSession(id), mqttConn(conn)
{}

OfferSession::OfferSession(OfferSession&& session):
ROAPSession(std::move(session)), mqttConn(std::move(session.mqttConn))
{}

void OfferSession::sendOffer(std::string sdp)
{
    ROAPMessage packet;
    if(state == ROAPSessionState::Closed)
    {
        ERROR_MESSAGE("the offerer(%s) is closed", myId.c_str());
    }
    packet.messageType = ROAPMessageType::Offer;
    packet.offererSessionId = myId;
    packet.seq=currentSeq;
    packet.sdp = sdp;
    state = ROAPSessionState::WaitAnswer;
    mqttConn->publishMessage("webrtc/roap/app", packet.toString());
}

void OfferSession::processMessage(ROAPMessage &in)
{
    ROAPMessage out;

    if(in.seq != currentSeq)
    {
        out.messageType = ROAPMessageType::Error;
        out.errorType = ROAPMessageErrorType::Failed;
        out.offererSessionId = in.offererSessionId;
        out.answererSessionId = in.answererSessionId;
        out.seq = in.seq;
        
        mqttConn->publishMessage("webrtc/roap/app", out.toString());
        state = ROAPSessionState::Closed;
        onClose();
    }
    else if(!(yourId.empty() ||
        yourId.compare(in.answererSessionId) == 0))
    {
        out.messageType = ROAPMessageType::Error;
        out.errorType = ROAPMessageErrorType::NoMatch;
        out.offererSessionId = in.offererSessionId;
        out.answererSessionId = in.answererSessionId;
        out.seq = currentSeq;

        mqttConn->publishMessage("webrtc/roap/app", out.toString());  
        state = ROAPSessionState::Closed;
        onClose();         
    }else
    {
        switch (state)
        {
            case ROAPSessionState::Start:
            {
                if(in.messageType == ROAPMessageType::Shutdown)
                {
                    state=ROAPSessionState::Closed;
                    out.messageType = ROAPMessageType::Ok;
                    out.offererSessionId = in.offererSessionId;
                    out.answererSessionId = in.answererSessionId;
                    out.seq = in.seq;
                    mqttConn->publishMessage("webrtc/roap/app", out.toString());
                    onClose();
                }
                break;
            }
            case ROAPSessionState::WaitAnswer:
            {
                if(in.messageType == ROAPMessageType::Answer)
                {
                    if(yourId.empty())
                    {
                        yourId=in.answererSessionId;
                    }
        
                    remoteSdp = in.sdp;
                    onRemoteSDP(in.sdp);
                    state = ROAPSessionState::Completed;
                    out.messageType = ROAPMessageType::Ok;
                    out.offererSessionId = in.offererSessionId;
                    out.answererSessionId = out.answererSessionId;
                    out.seq = currentSeq;
                    currentSeq++;
                    mqttConn->publishMessage("webrtc/roap/app", out.toString());
                }else if(in.messageType == ROAPMessageType::Error)
                {
                    ERROR_MESSAGE("ROAP Error: %x", uint8_t(in.errorType));
                    state = ROAPSessionState::Closed;     
                    
                }else if(in.messageType == ROAPMessageType::Shutdown)
                {
                    state=ROAPSessionState::Closed;
                    out.messageType = ROAPMessageType::Ok;
                    out.offererSessionId = in.offererSessionId;
                    out.answererSessionId = in.answererSessionId;
                    out.seq = in.seq;
                    mqttConn->publishMessage("webrtc/roap/app", out.toString());
                }
                
                break;
            }
            case ROAPSessionState::WaitCompleted:
            {
                if(in.messageType == ROAPMessageType::Ok)
                {
                    currentSeq++;
                    state=ROAPSessionState::Completed;
                }else if(in.messageType == ROAPMessageType::Shutdown)
                {
                    state=ROAPSessionState::Closed;
                    out.messageType = ROAPMessageType::Ok;
                    out.offererSessionId = in.offererSessionId;
                    out.answererSessionId = in.answererSessionId;
                    out.seq = in.seq;
                    mqttConn->publishMessage("webrtc/roap/app", out.toString());
                    onClose();
                }
                
                break;
            }
            case ROAPSessionState::Completed:
            {
                break;
            }
            case ROAPSessionState::WaitForShutdown:
            {
                if(in.messageType == ROAPMessageType::Ok) 
                {
                    state=ROAPSessionState::Closed;
                    onClose();
                }else if(in.messageType == ROAPMessageType::Shutdown)
                {
                    state=ROAPSessionState::Closed;
                    out.messageType = ROAPMessageType::Ok;
                    out.offererSessionId = in.offererSessionId;
                    out.answererSessionId = in.answererSessionId;
                    out.seq = in.seq;
                    mqttConn->publishMessage("webrtc/roap/app", out.toString());
                    
                    onClose();                
                }else
                {
                    out.messageType = ROAPMessageType::Shutdown;
                    out.offererSessionId = in.offererSessionId;
                    out.answererSessionId = in.answererSessionId;
                    out.seq = in.seq;
                    mqttConn->publishMessage("webrtc/roap/app", out.toString());
                }
                
                break;
            } 
            case ROAPSessionState::Closed:
            {
                out.messageType = ROAPMessageType::Error;
                out.errorType = ROAPMessageErrorType::NoMatch;
                out.offererSessionId = in.offererSessionId;
                out.answererSessionId = out.answererSessionId;
                out.seq = currentSeq;  
                mqttConn->publishMessage("webrtc/roap/app", out.toString());
                
                break;
            }   
        }
    }
    
}


void OfferSession::close()
{
    if(state == ROAPSessionState::Closed)
        return;

    state = ROAPSessionState::WaitForShutdown;
    
    ROAPMessage packet;
    packet.messageType = ROAPMessageType::Shutdown;
    packet.offererSessionId = myId;
    packet.answererSessionId = yourId;
    packet.seq = currentSeq;
    
    mqttConn->publishMessage("webrtc/roap/app", packet.toString());
}