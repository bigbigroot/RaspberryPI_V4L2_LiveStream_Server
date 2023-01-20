/**
 * @file capture.cpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdexcept>
#include <system_error>
#include <string>

#include <string.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "capture.hpp"
#include "utility.h"

std::string StrOfProfile(int p)
{
    switch (static_cast<H264Profile>(p))
    {
        case H264Profile::Baseline:
        {
            return "Baseline";
        }
        case H264Profile::Constrained_Baseline:
        {
            return "Constrained Baseline";
        }
        case H264Profile::Main:
        {
            return "Main";
        }
        case H264Profile::High:
        {
            return "High";
        }
    }
    return "";
}

std::string StrOfLevel(int p)
{
    switch (static_cast<H264Level>(p))
    {
        case H264Level::Level1:
        {
            return "1.0";
        }
        case H264Level::Level1b:
        {
            return "1b";
        }
        case H264Level::Level1_1:
        {
            return "1.1";
        }
        case H264Level::Level1_2:
        {
            return "1.2";
        }
        case H264Level::Level1_3:
        {
            return "1.3";
        }
        case H264Level::Level2:
        {
            return "2.0";
        }
        case H264Level::Level2_1:
        {
            return "2.1";
        }
        case H264Level::Level2_2:
        {
            return "2.2";
        }
        case H264Level::Level3:
        {
            return "3.0";
        }
        case H264Level::Level3_1:
        {
            return "3.1";
        }
        case H264Level::Level3_2:
        {
            return "3.2";
        }
        case H264Level::Level4:
        {
            return "4.0";
        }
        case H264Level::Level4_1:
        {
            return "4.1";
        }
        case H264Level::Level4_2:
        {
            return "4.2";
        }
    }
    return "";
}

#if(ENUM_CTRL > 0)
void VideoCapture::enumerateMenu(__u32 id, __u32 min_i, __u32 max_i){
    struct v4l2_querymenu  cam_ctrl_menu_info;

    V4L2_MESSAGE("Menu items: ");

    memset(&cam_ctrl_menu_info, 0, sizeof(cam_ctrl_menu_info));

    cam_ctrl_menu_info.id = id;

    for (cam_ctrl_menu_info.index = min_i;
         cam_ctrl_menu_info.index <= max_i;
         cam_ctrl_menu_info.index++) {
        if (0 == ioctl(fd, VIDIOC_QUERYMENU, &cam_ctrl_menu_info)) {
            V4L2_MESSAGE("\t%s", cam_ctrl_menu_info.name);
        }
    }
}
#endif

VideoCapture::VideoCapture(std::string name)
:fd(-1), imgSize(0), deviceName{name},
isOpened(false), windows{0,0}
{
}

VideoCapture::~VideoCapture(){
    if(isOpened){
        closeDevice();
    }    
}
/**
 * @brief open device by its name
 * 
 * @param dev_name device name for exmaple /dev/videoX
 * @return int if 0 sucess
 */
void VideoCapture::openDevice(void){
    struct stat st;

    if (-1 == stat(deviceName.c_str(), &st)) {
        isOpened = false;
        ERROR_MESSAGE("Cannot identify '%s': %d, %s\n",
                        deviceName.c_str(), errno, strerror(errno));
        throw std::system_error(errno, std::generic_category(), 
            "Cannot identify" + deviceName);
    }

    if (!S_ISCHR(st.st_mode)) {
        isOpened = false;
        ERROR_MESSAGE("%s is no devicen", deviceName.c_str());
        throw std::system_error(errno, std::generic_category(), 
            deviceName+" is no devicen");
    }
    fd = open(deviceName.c_str(), O_RDWR);
    if(fd == -1){
        isOpened = false;
        ERROR_MESSAGE("video info: cannot open device! (%s (%d))\n", 
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            "cannot open device!"+ deviceName);
    }
    isOpened = true;
}
/**
 * @brief close devide
 * 
 */
void VideoCapture::closeDevice(void){
    if(!isOpened) return;
    
    for(auto i: videoBuffer)
    {
        if(munmap(i.start, i.length) == -1)
        {
            ERROR_MESSAGE("munmap (%s(%d)).",
                strerror(errno), errno);
            throw std::system_error(errno, std::generic_category(), 
                "munmap");
        }
    }

    if(close(fd)==-1){
        ERROR_MESSAGE("close device error! (%s (%d))\n", 
            strerror(errno), errno);
    }
    isOpened = false;
    fd= -1;
}
/**
 * @brief check the device is a video capture device (camera) and then print the 
 * info of the driver 
 * 
 */
void VideoCapture::checkDevCap(void){
    struct v4l2_capability video_cap;
    if(!isOpened){
        ERROR_MESSAGE("device(%s) has not been opened.",
            deviceName.c_str());
        throw std::runtime_error("device has not been opened.");
    }

    memset(&video_cap, 0, sizeof(video_cap));

    if(ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1){
        ERROR_MESSAGE("VIDIOC_QUERYCAP (%s(%d)).",
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            "VIDIOC_QUERYCAP");
    }else{
        if(!(video_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
            V4L2_MESSAGE("video info: this device is not camera.");
        }
        
        if (!(video_cap.capabilities & V4L2_CAP_READWRITE)) {
            V4L2_MESSAGE("video info: this device does not support write operation.");
        }

        if (!(video_cap.capabilities & V4L2_CAP_STREAMING)) {
            V4L2_MESSAGE("video info: this device does not support streaming i/o.");
        }

        V4L2_MESSAGE("Capture device:\t%s", video_cap.card);
        V4L2_MESSAGE("Capture deriver:\t%s.", video_cap.driver);
    }
}


/**
 * @brief check which control will be supported  by device. 
 * 
 */
void VideoCapture::checkAllContol(){    
    struct v4l2_queryctrl  cam_ctrl_info;
    
    if(!isOpened){
        ERROR_MESSAGE("device(%s) has not opened.",
            deviceName.c_str());
        throw std::runtime_error("device has not been opened.");
    }

    memset(&cam_ctrl_info, 0, sizeof(cam_ctrl_info));
    
    cam_ctrl_info.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while(ioctl(fd, VIDIOC_QUERYCTRL, &cam_ctrl_info) == 0){
        if(!(cam_ctrl_info.flags & V4L2_CTRL_FLAG_DISABLED)){
            if(cam_ctrl_info.type == V4L2_CTRL_TYPE_CTRL_CLASS){
                V4L2_MESSAGE("======================================");
                V4L2_MESSAGE("control class (%s)", cam_ctrl_info.name);
                V4L2_MESSAGE("======================================");
            }
#if(ENUM_CTRL > 0)
            else if(cam_ctrl_info.type == V4L2_CTRL_TYPE_MENU){
                V4L2_MESSAGE("control (%s)", cam_ctrl_info.name);
                enumerateMenu(cam_ctrl_info.id, cam_ctrl_info.minimum, 
                    cam_ctrl_info.maximum);
            }
#endif
            else{
                V4L2_MESSAGE("control (%s)", cam_ctrl_info.name);
            }
        }

        cam_ctrl_info.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }
}
/**
 * @brief check which image format will be supported  by device. 
 * 
 */
void VideoCapture::checkVideoFormat(void){
    struct v4l2_fmtdesc video_fmtdesc;
    
    if(!isOpened){
        ERROR_MESSAGE("device(%s) has not opened.",
            deviceName.c_str());
        throw std::runtime_error("device has not been opened.");
    }
    
    memset(&video_fmtdesc, 0, sizeof(video_fmtdesc));
    
    video_fmtdesc.index=0;
    video_fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while(ioctl(fd, VIDIOC_ENUM_FMT, &video_fmtdesc) == 0){
        V4L2_MESSAGE("format[%d]: %s", video_fmtdesc.index, 
            video_fmtdesc.description);
        video_fmtdesc.index++;
    }

    if(video_fmtdesc.index == 0){
        ERROR_MESSAGE("video info: VIDIOC_ENUM_FMT (%s (%d)).", 
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            "VIDIOC_ENUM_FMT");
    }
}

/**
 * @brief set H.264's video stream format 
 * 
 */
void VideoCapture::setVideoFormat(void){
    struct v4l2_format video_fmt;
    
    if(!isOpened){
        ERROR_MESSAGE("device(%s) has not opened.",
            deviceName.c_str());
        throw std::runtime_error("device has not been opened.");
    }

    memset(&video_fmt, 0, sizeof(video_fmt));

    video_fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_G_FMT, &video_fmt) == -1){
        ERROR_MESSAGE("VIDIOC_G_FMT (%s(%d)).",
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            "VIDIOC_G_FMT");
    }
    
    if(windows.height != 0 || windows.width != 0){
        video_fmt.fmt.pix.height = windows.height;
        video_fmt.fmt.pix.width = windows.width;
        V4L2_MESSAGE("set windows...");
    }
    video_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    if(ioctl(fd, VIDIOC_S_FMT, &video_fmt) == -1){
        ERROR_MESSAGE("VIDIOC_S_FMT (%s(%d)).",
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            "VIDIOC_S_FMT");
    }
    if (video_fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_H264) {
        ERROR_MESSAGE("driver didn't accept H.264 format. Can't proceed.\n");
        throw std::system_error(errno, std::generic_category(), 
            "VIDIOC_QUERYCAP");
    }
    if ((video_fmt.fmt.pix.width != windows.width) ||
        (video_fmt.fmt.pix.height != windows.height)){
        windows.height = video_fmt.fmt.pix.height;
        windows.width = video_fmt.fmt.pix.width;

        V4L2_MESSAGE("Warning: driver is sending image at %dx%d\n",
                windows.width, windows.height);
    }
    
    imgSize = video_fmt.fmt.pix.sizeimage;
    V4L2_MESSAGE("image size: %d.", imgSize);
}

void VideoCapture::setH264ProfileAndLevel(H264Profile profile,
                                          H264Level level)
{
    struct v4l2_control control;

    if(!isOpened){
        ERROR_MESSAGE("device(%s) has not opened.",
            deviceName.c_str());
        throw std::runtime_error("device has not been opened.");
    }
    
    memset(&control, 0, sizeof(control));
    control.id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;
    control.value = (int32_t)profile;

    if(ioctl(fd, VIDIOC_S_CTRL, &control) == -1)
    {
        ERROR_MESSAGE("VIDIOC_S_CTRL (%s(%d)).",
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            "VIDIOC_S_CTRL");
    }else
    {
        V4L2_MESSAGE("set h.264 profil: %s", StrOfProfile(control.value).c_str());        
    }    
    
    memset(&control, 0, sizeof(control));
    control.id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
    control.value = (int32_t)level;

    if(ioctl(fd, VIDIOC_S_CTRL, &control) == -1)
    {
        ERROR_MESSAGE("VIDIOC_S_CTRL (%s(%d)).",
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            "VIDIOC_S_CTRL");
    }else
    {
        V4L2_MESSAGE("set h.264 level: %s", StrOfLevel(control.value).c_str());        
    }
}

int VideoCapture::readVideoFrame(uint8_t *buf, size_t len){
    if(!isOpened){
        ERROR_MESSAGE("device(%s) has not opened.",
            deviceName.c_str());
        throw std::runtime_error("device has not been opened.");
    }
    int ret = read(fd, buf, len);
    if(ret == -1){
        ERROR_MESSAGE("read \'%s\' failed(%s(%d)).", deviceName.c_str(),
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            "read device failed");
    }else if(ret == 0){
        V4L2_MESSAGE("EOF Read");
    }
    return ret;
}

void VideoCapture::setWindow(WindowsSize win){
    switch (win)
    {
        case WindowsSize::pixel_720p:
        {
            windows.height = 720;
            windows.width = 1280;
            break;
        }
        case WindowsSize::pixel_1080p:
        {
            windows.height = 1920;
            windows.width = 1080;
            break;
        }
        case WindowsSize::pixel_5MP:
        {
            windows.height = 2592;
            windows.width = 1944;
            break;
        }    
    }

}

void VideoCapture::initMmap()
{
    struct v4l2_requestbuffers reqbufs;

    if(!isOpened){
        ERROR_MESSAGE("device(%s) has not opened.",
            deviceName.c_str());
        throw std::runtime_error("device has not been opened.");
    }

    memset(&reqbufs, 0, sizeof(reqbufs));
    
    reqbufs.count = videoBuffersNum;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    
    if(ioctl(fd, VIDIOC_REQBUFS, &reqbufs) ==  -1)
    {        
        ERROR_MESSAGE("VIDIOC_REQBUFS (%s(%d)).",
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            "VIDIOC_REQBUFS");
    }

    if (reqbufs.count < 1)
    {        
        ERROR_MESSAGE("%s have no enough buffer. (%s(%d)).", deviceName.c_str(),
            strerror(errno), errno);
        throw std::system_error(errno, std::generic_category(), 
            deviceName + " have no enough buffer.");
    }


    for(size_t i = 0; i < reqbufs.count; i++)
    {
        struct v4l2_buffer buf;
        size_t len;
        void *start;

        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if(ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            ERROR_MESSAGE("VIDIOC_QUERYBUF (%s(%d)).",
                strerror(errno), errno);
            throw std::system_error(errno, std::generic_category(), 
                "VIDIOC_QUERYBUF");
        }

        len = buf.length;
        start = mmap(NULL, len, PROT_READ|PROT_WRITE,
                     MAP_SHARED, fd, buf.m.offset);

        if(start == MAP_FAILED)
        {
            ERROR_MESSAGE("mmap (%s(%d)).",
                strerror(errno), errno);
            throw std::system_error(errno, std::generic_category(), 
                "mmap");
        }

        videoBuffer.push_back({.start = start, .length = len});
    }
}

void VideoCapture::handleLoop()
{
    if(videoBuffer.empty())
    {
        initMmap();
    }

    for(; !videoBuffer.empty(); videoBuffer.pop_front())
    {
        auto& i = videoBuffer.front();
        onSample(i.start, i.length);
        if(munmap(i.start, i.length) == -1)
        {
            ERROR_MESSAGE("munmap (%s(%d)).",
                strerror(errno), errno);
            throw std::system_error(errno, std::generic_category(), 
                "munmap");
        }
    }

}

