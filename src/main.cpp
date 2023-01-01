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

#include "capture.hpp"
#include "utility.h"
#include "session.hpp"
#include "mqtt_connect.hpp"

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
    rtc::Configuration rtcConfig;
    std::shared_ptr<MqttConnect> mqttConn;
    std::shared_ptr<VideoCapture> camera;
    std::shared_ptr<H264VideoStream> videoStream;
    std::unique_ptr<RTCPeerSessionManager> peers;

    signal(SIGINT, signal_handler);
    
    rtcConfig.iceServers.emplace_back(rtc::IceServer("stun:192.168.5.10:3478"));

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
        rtc::Preload();

        auto configJson = nlohmann::json::parse(configFile);
        auto iceServerUrls = configJson["iceServer"]["urls"];
        std::string&& mqttURL = configJson["mqtt"]["url"].get<std::string>();
        std::string&& mqttClientId = configJson["mqtt"]["clientid"].get<std::string>();
        std::string&& mqttUsername = configJson["mqtt"]["username"].get<std::string>();
        std::string&& mqttPassword = configJson["mqtt"]["password"].get<std::string>();

        for(auto item : iceServerUrls)
        {
            std::string&& url = item.get<std::string>();
            rtcConfig.iceServers.emplace_back(rtc::IceServer(url));
        }
        
        camera = std::make_shared<VideoCapture>("/dev/video0");

        camera->setWindow(VideoCapture::WindowsSize::pixel_720p);
        camera->openDevice();
        camera->setVideoFormat();
        camera->onSample = [&videoStream](void *data, size_t len)
        {
            videoStream->onDataHandle(reinterpret_cast<std::byte *>(data), len);
        };

        mqttConn = std::make_shared<MqttConnect>(
            mqttURL, mqttClientId,mqttUsername, mqttPassword);
        videoStream = std::make_shared<H264VideoStream>();
        peers = std::make_unique<RTCPeerSessionManager>(std::move(rtcConfig), mqttConn, videoStream);
        
        mqttConn->subscribeTopic("webrtc/notify/camera");
        mqttConn->subscribeTopic("webrtc/roap/camera");

        mqttConn->registeTopicHandle("webrtc/notify/camera", 
        [&peers](std::string topic, std::string message)
        {
            peers->createRTCPeerSession();
        });

        mqttConn->registeTopicHandle("webrtc/roap/camera", 
        [&peers](std::string topic, std::string message)
        {
            peers->processMessage(message);
        });

        while(!awaitExit)
        {
            camera->handleLoop();
        }
        //... finally ...
        rtc::Cleanup();
    }catch(const std::exception& e)
    {
        printf("%s\n", e.what());
        return EXIT_FAILURE;
    }

    camera->closeDevice();

    return 0;
}