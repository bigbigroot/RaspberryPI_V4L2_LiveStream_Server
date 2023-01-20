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

#include <rtc/rtc.hpp>

#include "capture.hpp"

using NALUnit = std::vector<std::byte>;


class H264VideoTrack
{
    private:
        std::shared_ptr<rtc::Track> track;
        std::shared_ptr<rtc::RtcpSrReporter> srReporter;

    public:
        H264VideoTrack()=default;
        ~H264VideoTrack()=default;
        void addVideo(rtc::PeerConnection& pc);
        // void sendKeyframe();
        void send(std::byte *data, size_t len, time_t time);
};

class H264VideoStream
{
    private:
        std::map<std::string, std::shared_ptr<H264VideoTrack>> tracks;
        std::mutex lock;
    public:
        H264VideoStream()=default;
        ~H264VideoStream()=default;
        void addTrack(std::string id, const std::shared_ptr<H264VideoTrack>& track);
        void deleteById(std::string id);
        void onDataHandle(std::byte *data, size_t len,  time_t time);
        static std::string getProfileLevelId(H264Profile profile,
                                             H264Level level);
};

#endif /* __STREAMER_H */