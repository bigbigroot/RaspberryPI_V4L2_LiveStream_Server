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

#include <memory>
#include <functional>

#include <rtc/rtc.hpp>

#include "capture.h"
#include "server.h"
#include "utility.h"

/**
 * @brief will be called when programm exit by Ctrl-C
 * 
 */

int main(int argc, char *argv[]) {
    rtc::Configuration config;

    // rtc::Description desc("", rtc::Description::Type::Answer);
    // desc.addMedia()
    
    rtc::InitLogger(rtc::LogLevel::Debug, 
    [](rtc::LogLevel logLevel,std::string msg){
        printf("LibDataChannel Debug: %s\n", msg.c_str());
    });
    
    config.iceServers.emplace_back(rtc::IceServer("localhost", 3478));

    rtc::PeerConnection pc(config);

    // pc.setLocalDescription();

    // pc.addTrack(

    return 0;
}