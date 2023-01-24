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

#include <csignal>

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
int count=0;

void signal_handler(int sig){
    APP_MESSAGE("programm will exit...");
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
    

    configFile = fopen("config.json", "r");
    if(configFile == nullptr)
    {
        ERROR_MESSAGE("cannot open configure file!");
        return EXIT_FAILURE;
    }
    
    try
    {
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

        mqttConn = std::make_shared<MqttConnect>(
            mqttURL, mqttClientId,mqttUsername, mqttPassword);
        peers = std::make_unique<RTCPeerSessionManager>(std::move(rtcConfig), mqttConn, videoStream);

        rtc::InitLogger(rtc::LogLevel::Verbose, 
            [](rtc::LogLevel logLevel, std::string msg){
                switch (logLevel)
                {
                    case rtc::LogLevel::Fatal:
                    {
                        std::cout<<"LibDataChannel Fatal: "<<msg<<std::endl;
                    }
                    case rtc::LogLevel::Error:
                    {
                        std::cout<<"LibDataChannel Error: "<<msg<<std::endl;
                    }
                    case rtc::LogLevel::Warning:
                    {
                        std::cout<<"LibDataChannel Warn: "<<msg<<std::endl;
                    }
                    case rtc::LogLevel::Info:
                    {
                        std::cout<<"LibDataChannel Info: "<<msg<<std::endl;
                    }
                    case rtc::LogLevel::Debug:
                    {
                        std::cout<<"LibDataChannel Debug: "<<msg<<std::endl;
                    }
                    default:
                    {
                        // std::cout<<"LibDataChannel output: "<<msg<<std::endl;
                        break;
                    }
                }
            }
        );

        rtc::Preload();

        mqttConn->onMessage =
        [&peers](std::string topic, std::string message)
        {
            peers->loopHandler(); 
            if(topic.compare("webrtc/notify/camera") == 0)
            {
                peers->createRTCPeerSession();
            }else if(topic.compare("webrtc/roap/camera") == 0)
            {
                peers->processMessage(message);
            }       
        };

        mqttConn->subscribeTopic("webrtc/notify/camera");
        mqttConn->subscribeTopic("webrtc/roap/camera");

        camera = std::make_shared<VideoCapture>("/dev/video0");

        camera->setWindow(VideoCapture::WindowsSize::pixel_720p);
        camera->openDevice();
        camera->checkDevCap();
        camera->checkAllContol();
        camera->checkVideoFormat();
        camera->setVideoFormat();
        camera->setH264ProfileAndLevel(H264Profile::Constrained_Baseline,
                                       H264Level::Level3_1);
        auto fps = camera->getVideoStreamFps();
        videoStream = std::make_shared<H264VideoStream>(fps);

        camera->onSample = [&videoStream](void *data, size_t len)
        {
            videoStream->onDataHandle(reinterpret_cast<std::byte *>(data), len);
        };
        camera->initMmap();
        camera->start();

        while(!awaitExit)
        {
            // if(videoStream->hasTrack())
            {
                camera->handleLoop();
            }
        }

        //... finally ...
        camera->stop();
        camera->uninitMmap();
        camera->closeDevice();

        rtc::Cleanup();

    }catch(const std::exception& e)
    {
        std::cout<<e.what()<<std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}