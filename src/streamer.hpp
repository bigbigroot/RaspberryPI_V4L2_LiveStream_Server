/**
 * @file streamer.hpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-12-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __STREAMER_H
#define __STREAMER_H

#include <vector>
#include <memory>
#include <mutex>
#include <ctime>

#include <rtc/rtc.hpp>

#include "capture.hpp"

using NALUnit = std::vector<std::byte>;


class H264VideoTrack
{
    private:
        std::shared_ptr<rtc::Track> track;
        std::shared_ptr<rtc::RtcpSrReporter> srReporter;
        std::function<void()> startHandler;
        const double frameDuration_s;
    public:
        H264VideoTrack(double frameDuration);
        ~H264VideoTrack()=default;
    
        void addVideo(rtc::PeerConnection& pc);
        void onStart(std::function<void()> callback);
        void sendKeyframe(rtc::binary initalNALUs);
        void send(NALUnit data, uint64_t time);
        void start();
};

class H264VideoStream
{
    private:
        unsigned int framesPerSecond = 30;
        uint64_t startTime=0;
        
        uint64_t sampleDuration_us;
        uint64_t sampleTime_us = 0;
        std::map<std::string, std::weak_ptr<H264VideoTrack>> tracks;
        std::mutex lock;
        std::optional<NALUnit> previousUnitType5 = std::nullopt;
        std::optional<NALUnit> previousUnitType7 = std::nullopt;
        std::optional<NALUnit> previousUnitType8 = std::nullopt;
    public:
        H264VideoStream()=default;
        ~H264VideoStream()=default;
        H264VideoStream(unsigned int fps);
        void setStreamFps(unsigned int fps);
        unsigned int getStreamFps();
        uint64_t getDuration_us();
        void addTrack(std::string id, const std::shared_ptr<H264VideoTrack>& track);
        void deleteById(std::string id);
        void onDataHandle(std::byte *data, size_t len);
        bool hasTrack();
        NALUnit getInitialNALUS();
        static std::string getProfileLevelId(H264Profile profile,
                                             H264Level level);
};

#endif /* __STREAMER_H */