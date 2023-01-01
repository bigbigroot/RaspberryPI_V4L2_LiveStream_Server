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

// H264VideoTrack::H264VideoTrack()
// {
// }

H264VideoTrack::~H264VideoTrack()
{
}

void H264VideoTrack::addVideo(rtc::PeerConnection& pc)
{
    const rtc::SSRC ssrc{1};
    //payload type 98: Single NALU mode
    const int payloadType{98};
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


void H264VideoTrack::send(std::byte *data, size_t len)
{
    track->send(data, len);
}

void H264VideoStream::addTrack(std::string id, const std::shared_ptr<H264VideoTrack>& track)
{
    tracks.insert({id, track});
}

void H264VideoStream::deleteById(std::string id)
{
    auto it = tracks.find(id);

    if(it != tracks.end())
    {
        tracks.erase(it);
    }
}

void H264VideoStream::onDataHandle(std::byte *data, size_t len)
{
    for(auto i: tracks)
    {
        i.second->send(data, len);
    }
}