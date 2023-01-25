/**
 * @file streamer.cpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-12-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <string.h>

#include "streamer.hpp"
#include "utility.h"


const uint8_t start_code[] = {0x00, 0x00, 0x00, 0x01};

constexpr uint8_t h264_baseline_profile = 66;
constexpr uint8_t h264_main_profile = 77;
constexpr uint8_t h264_High_profile = 100;

constexpr uint8_t h264_level_1b = 11;

constexpr uint8_t constraint_set0_flag = 0x80;
constexpr uint8_t constraint_set1_flag = 0x40;
constexpr uint8_t constraint_set2_flag = 0x20;
constexpr uint8_t constraint_set3_flag = 0x10;
constexpr uint8_t constraint_set4_flag = 0x08;
constexpr uint8_t constraint_set5_flag = 0x04;


constexpr uint8_t constraint_clear0_flag = 0x7f;
constexpr uint8_t constraint_clear1_flag = 0xbf;
constexpr uint8_t constraint_clear2_flag = 0xd0;
constexpr uint8_t constraint_clear3_flag = 0xe0;
constexpr uint8_t constraint_clear4_flag = 0xf7;
constexpr uint8_t constraint_clear5_flag = 0xfb;

H264VideoTrack::H264VideoTrack(double frameDuration):
frameDuration_s(frameDuration)
{
}

// H264VideoTrack::~H264VideoTrack()
// {
// }

void H264VideoTrack::addVideo(rtc::PeerConnection& pc)
{
    const rtc::SSRC ssrc{1};
    
    const int payloadType{static_cast<int>(100)};
    const std::string cname("camera");

    rtc::Description::Video media(cname, rtc::Description::Direction::SendOnly);
    media.addH264Codec(payloadType);
    media.addSSRC(ssrc, cname);
    track = pc.addTrack(media);
    
    // create RTP configuration
    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
        ssrc, cname, payloadType,
        rtc::H264RtpPacketizer::defaultClockRate);

    // create packetizer
    auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(
        rtc::H264RtpPacketizer::Separator::StartSequence, rtpConfig);
    // create H264 handler
    auto h264Handler = std::make_shared<rtc::H264PacketizationHandler>(packetizer);
    // add RTCP SR handler
    srReporter = std::make_shared<rtc::RtcpSrReporter>(rtpConfig);
    h264Handler->addToChain(srReporter);
    // add RTCP NACK handler
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    h264Handler->addToChain(nackResponder);
    // set handler
    track->setMediaHandler(h264Handler);

    track->onOpen([this]()
        {
            this->start();
        }
    );
}

void H264VideoTrack::start()
{
    startHandler();
}

void H264VideoTrack::onStart(std::function<void()> callback)
{
    startHandler = callback;

}

void H264VideoTrack::send(NALUnit data, uint64_t time)
{
    
    auto rtpConfig = srReporter->rtpConfig;
     // sample time is in us, we need to convert it to seconds
    auto elapsedSeconds = double(time) / (1000 * 1000);
    // get elapsed time in clock rate
    uint32_t elapsedTimestamp = rtpConfig->secondsToTimestamp(elapsedSeconds);
    // set new timestamp
    rtpConfig->timestamp = rtpConfig->startTimestamp + elapsedTimestamp;

    // get elapsed time in clock rate from last RTCP sender report
    auto reportElapsedTimestamp = rtpConfig->timestamp - srReporter->lastReportedTimestamp();
    // check if last report was at least 1 second ago
    if (rtpConfig->timestampToSeconds(reportElapsedTimestamp) > 1)
    {
        srReporter->setNeedsToReport();
    }

    try {
        // send sample
        track->send(data);
    } catch (const std::exception &e) {
        ERROR_MESSAGE("Unable to send %s", e.what());
    }
}

void H264VideoTrack::sendKeyframe(rtc::binary initalNALUs)
{
    if(!initalNALUs.empty())
    {
        const double frameDuration_s = double() / (1000 * 1000);
        const uint32_t frameTimestampDuration = srReporter->rtpConfig->secondsToTimestamp(frameDuration_s);
        srReporter->rtpConfig->timestamp = srReporter->rtpConfig->startTimestamp - frameTimestampDuration * 2;
        track->send(initalNALUs);
        srReporter->rtpConfig->timestamp += frameTimestampDuration;
        // Send initial NAL units again to start stream in firefox browser
        track->send(initalNALUs);
    }
}

H264VideoStream::H264VideoStream(unsigned int fps):
framesPerSecond(fps), startTime(0), sampleTime_us(0)
{
    sampleDuration_us = (1000000UL)/fps;
}

void H264VideoStream::setStreamFps(unsigned int fps)
{
    framesPerSecond = fps;
    sampleDuration_us = (1000000UL)/fps;
}

unsigned int H264VideoStream::getStreamFps()
{
    return framesPerSecond;
}

uint64_t H264VideoStream::getDuration_us()
{
    return sampleDuration_us;
}
void H264VideoStream::start()
{
    sampleTime_us = std::numeric_limits<uint64_t>::max()
                    - sampleDuration_us + 1;
}

void H264VideoStream::stop()
{
    sampleTime_us = 0;
}
void H264VideoStream::addTrack(std::string id,
                               const std::shared_ptr<H264VideoTrack>& track)
{
    lock.lock();
    tracks.insert({id, track});
    lock.unlock();
}

void H264VideoStream::deleteById(std::string id)
{
    lock.lock();
    auto it = tracks.find(id);

    if(it != tracks.end())
    {
        tracks.erase(it);
    }
    lock.unlock();
}

bool H264VideoStream::hasTrack()
{
    bool ret;
    lock.lock();
    ret = tracks.empty();
    lock.unlock();
    return !ret;
}

NALUnit H264VideoStream::getInitialNALUS()
{
    NALUnit units{};
    if (previousUnitType7.has_value()) {
        auto nalu = previousUnitType7.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    if (previousUnitType8.has_value()) {
        auto nalu = previousUnitType8.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    if (previousUnitType5.has_value()) {
        auto nalu = previousUnitType5.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    return units;
}

void H264VideoStream::onDataHandle(std::byte *data, size_t len)
{
    if(memcmp(data, start_code, sizeof(start_code)) == 0)
    {
        sampleTime_us += sampleDuration_us;
        if(len > sizeof(start_code))
        {
            auto nalu = NALUnit(data, data+len);
            char type = uint8_t(nalu[sizeof(start_code)]) & 0x1f;
            switch (type)
            {
                case 7:
                    previousUnitType7 = nalu;
                    break;
                case 8:
                    previousUnitType8 = nalu;
                    break;
                case 5:
                    previousUnitType5 = nalu;
                    break;
            }

            lock.lock();
            for(auto i: tracks)
            {
                lock.unlock();
                auto wkt = i.second;
                if(wkt.expired()){
                    deleteById(i.first);
                }else
                {
                    wkt.lock()->send(nalu, sampleTime_us);
                }
                
                lock.lock();
            }
            lock.unlock();
        }else
        {
            ERROR_MESSAGE("this sample is too small.");
        }
    }else{
        ERROR_MESSAGE("this sample is no with the start code at the start!");
    }
}

std::string H264VideoStream::getProfileLevelId(
    H264Profile profile, H264Level level)
{
    char profile_level_id[9];
    uint8_t profile_idc = 0;
    uint8_t profile_iop = 0;
    uint8_t level_idc = 0;
    switch(profile)
    {
        case H264Profile::Constrained_Baseline:
        {
            profile_iop |= constraint_set1_flag;
        }
        case H264Profile::Baseline:
        {
            profile_idc=h264_baseline_profile;
            break;
        }
        case H264Profile::Main:
        {
            profile_iop |= constraint_set1_flag;
            profile_idc = h264_main_profile;
            break;
        }
        case H264Profile::High:
        {
            profile_iop = 0;
            profile_idc = h264_High_profile;
            break;
        }
    
    }

    switch (level)
    {
        case H264Level::Level1:
        {
            level_idc = 10;
            break;
        }
        case H264Level::Level1b:
        {
            if(profile == H264Profile::High){
                level_idc = 9;
            }
            else{
                level_idc = 10;
                profile_iop |= constraint_clear3_flag;
            }
            break;
        }        
        case H264Level::Level1_1:
        {
            level_idc = 11;
            break;
        }
        case H264Level::Level1_2:
        {
            level_idc = 12;
            break;
        }
        case H264Level::Level1_3:
        {
            level_idc = 13;
            break;
        }
        case H264Level::Level2:
        {
            level_idc = 20;
            break;
        }
        case H264Level::Level2_1:
        {
            level_idc = 21;
            break;
        }
        case H264Level::Level2_2:
        {
            level_idc = 22;
            break;
        }
        case H264Level::Level3:
        {
            level_idc = 30;
            break;
        }
        case H264Level::Level3_1:
        {
            level_idc = 31;
            break;
        }
        case H264Level::Level3_2:
        {
            level_idc = 32;
            break;
        }
        case H264Level::Level4:
        {
            level_idc = 40;
            break;
        }
        case H264Level::Level4_1:
        {
            level_idc = 41;
            break;
        }
        case H264Level::Level4_2:
        {
            level_idc = 42;
            break;
        }
    }

    sprintf(profile_level_id, "0x%2x%2x%2x", profile_idc, profile_iop, level_idc);
    return profile_level_id;
}