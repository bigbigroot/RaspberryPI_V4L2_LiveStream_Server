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
#include <string.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "capture.h"
#include "utility.h"

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

VideoCapture::VideoCapture(const char *name){
    deviceName = name;
    isOpen = false;
    imgSize = 0;
    windows = {0, 0};
}

VideoCapture::~VideoCapture(){
    if(isOpen){
        closeDevice();
    }    
}
/**
 * @brief open device by its name
 * 
 * @param dev_name device name for exmaple /dev/videoX
 * @return int if 0 sucess
 */
int VideoCapture::openDevice(){
    struct stat st;

    if (-1 == stat(deviceName, &st)) {
        ERROR_MESSAGE("Cannot identify '%s': %d, %s\n",
                        deviceName, errno, strerror(errno));
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
        ERROR_MESSAGE("%s is no devicen", deviceName);
        return -1;
    }
    fd = open(deviceName, O_RDWR);
    if(fd == -1){
        ERROR_MESSAGE("video info: cannot open device! (%s (%d))\n", 
            strerror(errno), errno);
        return -1;
    }
    isOpen = true;
    return 0;
}
/**
 * @brief close devide
 * 
 */
void VideoCapture::closeDevice(){
    close(fd);
    isOpen = false;
    fd= -1;
}
/**
 * @brief check the device is a video capture device (camera) and then print the 
 * info of the driver 
 * 
 */
int VideoCapture::checkDevCap(){
    struct v4l2_capability video_cap;
    if(!isOpen){
        ERROR_MESSAGE("device(%s) have not opened.",
            deviceName);
        return -1;
    }

    memset(&video_cap, 0, sizeof(video_cap));

    if(ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1){
        ERROR_MESSAGE("VIDIOC_QUERYCAP (%s(%d)).",
            strerror(errno), errno);
        return -1;
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

        return 0;
    }
}


/**
 * @brief check which control will be supported  by device. 
 * 
 */
void VideoCapture::checkAllContol(){    
    struct v4l2_queryctrl  cam_ctrl_info;
    
    if(!isOpen){
        ERROR_MESSAGE("device(%s) have not opened.",
            deviceName);
        return;
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
                enumerate_menu(cam_ctrl_info.id, cam_ctrl_info.minimum, 
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
int VideoCapture::checkVideoFormat(){
    struct v4l2_fmtdesc video_fmtdesc;
    
    if(!isOpen){
        ERROR_MESSAGE("device(%s) have not opened.",
            deviceName);
        return -1;
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
        return -1;
    }
    return 0;
}

/**
 * @brief set H.264's video stream format 
 * 
 */
int VideoCapture::setVideoFormat(){
    struct v4l2_format video_fmt;
    
    if(!isOpen){
        ERROR_MESSAGE("device(%s) have not opened.",
            deviceName);
        return -1;
    }

    memset(&video_fmt, 0, sizeof(video_fmt));

    video_fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_G_FMT, &video_fmt) == -1){
        ERROR_MESSAGE("VIDIOC_G_FMT (%s(%d)).",
            strerror(errno), errno);
        return -1;
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
        return -1;
    }
    if (video_fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_H264) {
        ERROR_MESSAGE("driver didn't accept H.264 format. Can't proceed.\n");
        return -1;
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
    return 0;
}

int VideoCapture::readVideoFrame(uint8_t *buf, size_t len){
    if(!isOpen){
        ERROR_MESSAGE("device(%s) have not opened.",
            deviceName);
        return -1;
    }
    int ret = read(fd, buf, len);
    if(ret == -1){
        ERROR_MESSAGE("read \'%s\' failed(%s(%d)).", deviceName,
            strerror(errno), errno);
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