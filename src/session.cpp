/**
 * @file session.cpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-12-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "session.hpp"
#include "utility.h"

RTCPeerSession::RTCPeerSession(std::string id, const rtc::Configuration &config,
                               const std::shared_ptr<MqttConnect>& conn,
                               RTCPeerSessionManager &mg):
isWilldestroyed(false), sessionId(id), pc(config),
offerer(id, conn), manager(mg)
{
    double duration_s = double(manager.stream->getDuration_us()) / 1000*1000;
    videoTrack = std::make_shared<H264VideoTrack>(duration_s);
    videoTrack->onStart(
        [this]()
        {
            this->addToStream();
        }
    );
}

RTCPeerSession::~RTCPeerSession()
{
    APP_MESSAGE("session (id: %s) will be destoryed!", getId().c_str());
    isWilldestroyed = true;
    pc.close();
}

std::string RTCPeerSession::getLocalSdp()
{
    return std::string(pc.localDescription().value());
}

void RTCPeerSession::setRemoteSdp(std::string sdp)
{
    pc.setRemoteDescription(rtc::Description(sdp, "answer"));
}

std::string RTCPeerSession::getId()
{
    return sessionId;
}

void RTCPeerSession::open()
{
    pc.onGatheringStateChange(
        [this](rtc::PeerConnection::GatheringState state)
        {
            if(state == rtc::PeerConnection::GatheringState::Complete)
            {
                auto localSdp = this->getLocalSdp();
                this->offerer.sendOffer(localSdp);
            }
        }
    );

    pc.onStateChange(
        [this](rtc::PeerConnection::State state)
        {
            if(state == rtc::PeerConnection::State::Connected)
            {
                auto id = this->getId();
                
                APP_MESSAGE("Session (id: %s) have been connected to the answer.", id.c_str());
            }
            else if(state == rtc::PeerConnection::State::Closed
                || state == rtc::PeerConnection::State::Failed
                || state == rtc::PeerConnection::State::Disconnected)
            {
                this->close();
            }
        }
    );

    offerer.onRemoteSDP = [this](std::string sdp)
    {
        this->setRemoteSdp(sdp);
    };
    
    offerer.onClose = [this](){
        pc.close();
    };

    videoTrack->addVideo(pc);

    pc.setLocalDescription(rtc::Description::Type::Offer);
}

void RTCPeerSession::close()
{
    if(!isWilldestroyed)
    {
        auto id = this->getId();
        this->manager.deleteRTCPeerSession(id);
        APP_MESSAGE("connect (id: %s) will be destoryed...", id.c_str());
    }
}

void RTCPeerSession::addToStream()
{
    videoTrack->sendKeyframe(manager.stream->getInitialNALUS());
    manager.stream->addTrack(sessionId, videoTrack);
}

RTCPeerSessionManager::RTCPeerSessionManager(
    rtc::Configuration&& config,
    const std::shared_ptr<MqttConnect>& conn,
    const std::shared_ptr<H264VideoStream>& s):
config(config), mqttConn(conn), stream(s)
{}

void RTCPeerSessionManager::createRTCPeerSession()
{
    auto id = uidg.allocateAUniqueId();
    APP_MESSAGE("allocated a id: %s.", id.c_str());
    while(peerSessions.find(id) != peerSessions.end())
    {
        ERROR_MESSAGE("id: %s have already existed!", id.c_str());
        id = uidg.allocateAUniqueId();
    } 

    auto session = 
        std::make_unique<RTCPeerSession>(id, config, mqttConn, *this);
    session->open();
    peerSessions.insert({id, std::move(session)});
}

void RTCPeerSessionManager::processMessage(std::string message)
{
    ROAPMessage in;
    in.parser(message);
    auto it  = peerSessions.find(in.offererSessionId);

    if(it == peerSessions.end())
    {
        if(in.messageType != ROAPMessageType::Error)
        {
            ROAPMessage out;
            out.messageType = ROAPMessageType::Error;
            out.errorType = ROAPMessageErrorType::NoMatch;
            out.offererSessionId = in.offererSessionId;
            out.answererSessionId = in.answererSessionId;
            out.seq = in.seq;

            mqttConn->publishMessage("webrtc/roap/app", out.toString());
        }

    }else
    {
        it->second->offerer.processMessage(in);
    }
    
}

void RTCPeerSessionManager::deleteRTCPeerSession(const std::string& id)
{
    stream->deleteById(id);
    lock.lock();
    closedSessions.push_back(id);
    lock.unlock();
}

void RTCPeerSessionManager::loopHandler()
{
    lock.lock();
    for(auto id: closedSessions)
    {
        peerSessions.erase(id);
    }
    lock.unlock();
}