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

#include "streamer.hpp"
#include "utility.h"

// H264VideoTrack::H264VideoTrack()
// {
// }

// H264VideoTrack::~H264VideoTrack()
// {
// }

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
    auto srReporter = std::make_shared<rtc::RtcpSrReporter>(rtpConfig);
    h264Handler->addToChain(srReporter);
    // add RTCP NACK handler
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    h264Handler->addToChain(nackResponder);
    // set handler
    track->setMediaHandler(h264Handler);
}


void H264VideoTrack::send(std::byte *data, size_t len, time_t time)
{
    // sample time is in us, we need to convert it to seconds
    auto elapsedSeconds = double(time) / (1000 * 1000);
    
    auto rtpConfig = srReporter->rtpConfig;

    // get elapsed time in clock rate
    uint32_t elapsedTimestamp = rtpConfig->secondsToTimestamp(elapsedSeconds);
    // set new timestamp
    rtpConfig->timestamp = rtpConfig->startTimestamp + elapsedTimestamp;

    // get elapsed time in clock rate from last RTCP sender report
    auto reportElapsedTimestamp = rtpConfig->timestamp - srReporter->lastReportedTimestamp();
    // check if last report was at least 1 second ago
    if (rtpConfig->timestampToSeconds(reportElapsedTimestamp) > 1) {
        srReporter->setNeedsToReport();
    }

    try {
        // send sample
        track->send(data, len);
    } catch (const std::exception &e) {
        ERROR_MESSAGE("Unable to send %s", e.what());
    }
}

void H264VideoStream::addTrack(std::string id, const std::shared_ptr<H264VideoTrack>& track)
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

void H264VideoStream::onDataHandle(std::byte *data, size_t len, time_t time)
{
    lock.lock();
    for(auto i: tracks)
    {
        lock.unlock();
        i.second->send(data, len, time);
        lock.lock();
    }
    lock.unlock();
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