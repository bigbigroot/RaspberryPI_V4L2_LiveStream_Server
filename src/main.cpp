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

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <chrono>

#include <rtc/rtc.hpp>
#include <json11.hpp>
#include <ixwebsocket/IXWebSocket.h>

#include "utility.h"

void signal_handler(int sig){
    rtc::Cleanup();
    exit(EXIT_SUCCESS);
}

/**
 * @brief will be called when programm exit by Ctrl-C
 * 
 */

int main(int argc, char *argv[]) {
    std::string offerSdp("v=0 \n \
o=- 1946435221213382501 2 IN IP4 127.0.0.1\n\
s=-\n\
t=0 0\n\
a=extmap-allow-mixed\n\
a=msid-semantic: WMS");
    rtc::Configuration rtcConfig;
    rtc::Description offerDescription(offerSdp,
    "offer");
    std::unique_ptr<rtc::PeerConnection> pc;

    signal(SIGINT, signal_handler);

    rtc::InitLogger(rtc::LogLevel::Verbose, 
    [](rtc::LogLevel logLevel,std::string msg){
        printf("LibDataChannel Debug: %s\n", msg.c_str());
    });
    
    
    try
    {
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

    // // Our websocket object
    // ix::WebSocket webSocket;

    // std::string url("ws://localhost:8080/");
    // webSocket.setUrl(url);

    // // Optional heart beat, sent every 45 seconds when there is not any traffic
    // // to make sure that load balancers do not kill an idle connection.
    // webSocket.setPingInterval(45);

    // // Per message deflate connection is enabled by default. You can tweak its parameters or disable it
    // webSocket.disablePerMessageDeflate();

    // // Setup a callback to be fired when a message or an event (open, close, error) is received
    // webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg)
    //     {
    //         if (msg->type == ix::WebSocketMessageType::Message)
    //         {
    //             std::cout << msg->str << std::endl;
    //         }
    //     }
    // );

    // // Now that our callback is setup, we can start our background thread and receive messages
    // webSocket.start();

    // // ... finally ...
    for(;;);


    // // Stop the connection
    // webSocket.stop();

    return 0;
}