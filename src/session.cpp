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

RTCPeerSession::RTCPeerSession(std::string id, const rtc::Configuration &config,
                               const std::shared_ptr<MqttConnect>& conn,
                               const std::shared_ptr<H264VideoTrack>& vt,
                               RTCPeerSessionManager &mg):
pc(config), videoTrack(vt), offer(id, conn), manager(mg)
{}

RTCPeerSession::~RTCPeerSession()
{
}

std::string RTCPeerSession::getLocalSdp()
{
    return std::string(pc.localDescription().value());
}

void RTCPeerSession::setRemoteSdp(std::string sdp)
{
    pc.setRemoteDescription(rtc::Description(sdp, "answer"));
}

void RTCPeerSession::open()
{
    pc.onGatheringStateChange(
        [this](rtc::PeerConnection::GatheringState state)
        {
            if(state == rtc::PeerConnection::GatheringState::Complete)
            {
                auto localSdp = this->getLocalSdp();
                this->offer.sendOffer(localSdp);
            }
        }
    );

    pc.onStateChange(
        [this](rtc::PeerConnection::State state)
        {
            if(state == rtc::PeerConnection::State::Closed
                || state == rtc::PeerConnection::State::Failed
                || state == rtc::PeerConnection::State::Disconnected)
            {
                this->offer.close();
            }
        }
    );

    offer.onRemoteSDP = [this](std::string sdp)
    {
        this->setRemoteSdp(sdp);
    };
    
    offer.onClose = [this](){
        auto id = this->offer.getId();
        this->manager.deleteRTCPeerSession(id);
    };

    videoTrack->addVideo(pc);

    pc.setLocalDescription(rtc::Description::Type::Offer);
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
    for(;peerSessions.find(id) == peerSessions.end();        
        id = uidg.allocateAUniqueId());    

    std::shared_ptr<H264VideoTrack> track = std::make_shared<H264VideoTrack>();

    auto session = 
        std::make_unique<RTCPeerSession>(id, config, mqttConn, track, *this);
    session->open();
    stream->addTrack(id, track);
    peerSessions.insert({id, std::move(session)});
}

void RTCPeerSessionManager::processMessage(std::string message)
{
    ROAPMessage in;
    in.parser(message);
    auto it  = peerSessions.find(in.offererSessionId);

    if(it == peerSessions.end())
    {
        ROAPMessage out;
        out.messageType = ROAPMessageType::Error;
        out.errorType = ROAPMessageErrorType::NoMatch;
        out.offererSessionId = in.offererSessionId;
        out.answererSessionId = in.answererSessionId;
        out.seq = in.seq;

        mqttConn->publishMessage(SEND_TOPIC, out.toString());
    }else
    {
        it->second->offer.processMessage(in);
    }
    
}

void RTCPeerSessionManager::deleteRTCPeerSession(const std::string& id)
{
    peerSessions.erase(id);
    stream->deleteById(id);
}