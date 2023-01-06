/**
 * @file session.hpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-12-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __SESSION_H
#define __SESSION_H

#include <rtc/rtc.hpp>
#include <memory>
#include <map>

#include "roaprotocol.hpp"
#include "streamer.hpp"
#include "random_id.hpp"


class RTCPeerSessionManager;

class RTCPeerSession
{
    private:
        rtc::PeerConnection pc;
        std::shared_ptr<H264VideoTrack> videoTrack;
    public:
        RTCPeerSession(std::string id, const rtc::Configuration &config,
                       const std::shared_ptr<MqttConnect>& conn,
                       const std::shared_ptr<H264VideoTrack>& vt,
                       RTCPeerSessionManager &mg);
        ~RTCPeerSession();
        OfferSession offer;
        RTCPeerSessionManager &manager;
        std::string getLocalSdp();
        std::string getId();
        void setRemoteSdp(std::string sdp);
        void open();
};

class RTCPeerSessionManager
{
    private:
        RandomIdGenerator uidg;
        rtc::Configuration config;
        std::shared_ptr<MqttConnect> mqttConn;
        std::shared_ptr<H264VideoStream> stream;
        std::map<std::string, std::unique_ptr<RTCPeerSession>> peerSessions;
    public:
        RTCPeerSessionManager(rtc::Configuration&& config,
                              const std::shared_ptr<MqttConnect>& conn,
                              const std::shared_ptr<H264VideoStream>& s);
        ~RTCPeerSessionManager()=default;
        void createRTCPeerSession();
        void processMessage(std::string message);
        void deleteRTCPeerSession(const std::string& id);
};

#endif /* __SESSION_H */