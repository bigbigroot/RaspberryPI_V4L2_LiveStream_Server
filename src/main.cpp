/**
 * @file main.c
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <chrono>

#include <rtc/rtc.hpp>
#include <nlohmann/json.hpp>

#include "utility.h"
#include "ROAProtocol.hpp"
#include "MessageHandler.hpp"

bool awaitExit = false;

void signal_handler(int sig){
    printf("programm will exit...\n");
    awaitExit = true;
}

/**
 * @brief will be called when programm exit by Ctrl-C
 * 
 */
int main(int argc, char *argv[]) {
    FILE *configFile;
    ROAPSession session;
    rtc::Configuration rtcConfig;
    std::unique_ptr<rtc::PeerConnection> pc;
    std::unique_ptr<MqttConnect> mqttConn;

    signal(SIGINT, signal_handler);

    configFile = fopen("config.json", "r");
    if(configFile == nullptr)
    {
        ERROR_MESSAGE("cannot open file!");
        return EXIT_FAILURE;
    }

    rtc::InitLogger(rtc::LogLevel::Verbose, 
    [](rtc::LogLevel logLevel,std::string msg){
        printf("LibDataChannel Debug: %s\n", msg.c_str());
    });
    
    
    try
    {
        auto configJson = nlohmann::json::parse(configFile);
        std::string mqttURL = configJson["mqtt"]["url"].get<std::string>();
        std::string mqttClientId = configJson["mqtt"]["clientid"].get<std::string>();
        std::string mqttUsername = configJson["mqtt"]["username"].get<std::string>();
        std::string mqttPassword = configJson["mqtt"]["password"].get<std::string>();

        mqttConn = std::make_unique<MqttConnect>(mqttURL, mqttClientId, mqttUsername, mqttPassword);
        mqttConn->subscribeTopic("webrtc/notify/camera");
        mqttConn->subscribeTopic("webrtc/roap/camera");
        mqttConn->registeTopicHandle("webrtc/notify/camera", 
        [&mqttConn, &pc, &session](std::string topic, std::string message)
        {
            auto offerSdp = pc->localDescription().value().generateSdp();
            mqttConn->publishMessage("webrtc/roap/app", session.createOffer(offerSdp));

        });

        mqttConn->registeTopicHandle("webrtc/roap/camera", 
        [&mqttConn, &pc, &session](std::string topic, std::string message)
        {
            ROAPMessage out;
            auto in = ROAPMessageParser(message);
            session.process(in, out);
            mqttConn->publishMessage("webrtc/roap/app", ROAPMessageToString(out));
        });

        rtcConfig.iceServers.emplace_back(rtc::IceServer("stun:192.168.5.10:3478"));

        rtc::Preload();

        pc = std::make_unique<rtc::PeerConnection>(rtcConfig);

        pc->onLocalDescription([](rtc::Description sdp)
        {
            // Send the SDP to the remote peer
            printf(std::string(sdp).c_str());
        });

        pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state)
        {
            switch (state)
            {
                case rtc::PeerConnection::GatheringState::New:
                {
                    printf("gather new\n");
                    break;
                }
                case rtc::PeerConnection::GatheringState::InProgress:
                {
                    printf("gather inprogress\n");
                    break;
                }
                case rtc::PeerConnection::GatheringState::Complete:
                {
                    printf("gather complete\n");
                    break;
                }                
            }
        });

        pc->onLocalCandidate([](rtc::Candidate candidate)
        {
            printf("loacl candidate: %s.\n", candidate.candidate().c_str());
        });

        pc->onStateChange([](rtc::PeerConnection::State state) 
        {
            switch (state)
            {
                case rtc::PeerConnection::State::New:
                {
                    printf("rtc new\n");
                    break;
                }
                case rtc::PeerConnection::State::Connecting:
                {
                    printf("rtc Connecting\n");
                    break;
                }
                case rtc::PeerConnection::State::Connected:
                {
                    printf("rtc connected\n");
                    break;
                }   
                case rtc::PeerConnection::State::Disconnected:
                {
                    printf("rtc disconnected\n");
                    break;
                }   
                case rtc::PeerConnection::State::Failed:
                {
                    printf("rtc failed\n");
                    break;
                }  
                case rtc::PeerConnection::State::Closed:
                {
                    printf("rtc closed\n");
                    break;
                }               
            }
        });
		const rtc::SSRC ssrc = 42;
		rtc::Description::Video media("video", rtc::Description::Direction::SendOnly);
		media.addH264Codec(96); // Must match the payload type of the external h264 RTP stream
		media.addSSRC(ssrc, "video-send");
		auto track = pc->addTrack(media);
        
        pc->setLocalDescription();
    }catch(const std::exception& e)
    {
        printf("%s\n", e.what());
        return EXIT_FAILURE;
    }

    while(!awaitExit);
    //... finally ...
    rtc::Cleanup();


    return 0;
}