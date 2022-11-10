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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>    /* video for linux two header file */

static int fd;

static struct v4l2_capability video_cap;
static struct v4l2_queryctrl  cam_ctrl_info;
static struct v4l2_querymenu  cam_ctrl_menu_info;

static struct v4l2_format video_fmt;

#ifdef VIDEO_DEBUG
    #define VIDEO_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define VIDEO_DEBUG_PRINT(...) 
#endif
/**
 * @brief will be called when programm exit by Ctrl-C
 * 
 */
void programm_exit(){

    close(fd);
    fprintf(stderr, "error %d: %s\n", 
        errno, strerror(errno));
}

static void errno_exit(const char *s){
    fprintf(stderr, "%s error %d, %s\n", s, 
        errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static void enumerate_menu(__u32 id)
{
    VIDEO_DEBUG_PRINT("\tControl Menu items:\n");

    memset(&cam_ctrl_menu_info, 0, sizeof(cam_ctrl_menu_info));

    cam_ctrl_menu_info.id = id;

    for (cam_ctrl_menu_info.index = cam_ctrl_info.minimum;
         cam_ctrl_menu_info.index <= cam_ctrl_info.maximum;
         cam_ctrl_menu_info.index++) {
        if (0 == ioctl(fd, VIDIOC_QUERYMENU, &cam_ctrl_menu_info)) {
            VIDEO_DEBUG_PRINT("\t\t%s\n", cam_ctrl_menu_info.name);
        }
    }
}

/**
 * @brief check the device is a video capture device (camera) and then print the 
 * info of the driver 
 * 
 */
void check_dev_cap(){
    memset(&video_cap, 0, sizeof(video_cap));

    if(ioctl(fd, VIDIOC_QUERYCAP, &video_cap)==-1){
        fprintf(stderr, "video info: ! (%s (%d))\n", 
            strerror(errno), errno);
        errno_exit("video info");
    }else{
        if(!(video_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
            VIDEO_DEBUG_PRINT("video info: this device is not camera. \n");
        }
        
        if (!(video_cap.capabilities & V4L2_CAP_READWRITE)) {
            VIDEO_DEBUG_PRINT("video info: this device does not support write operation\n");
        }

        if (!(video_cap.capabilities & V4L2_CAP_STREAMING)) {
            VIDEO_DEBUG_PRINT("video info: this device does not support streaming i/o\n");
        }

        VIDEO_DEBUG_PRINT("Camera info:\tname:\t%s\n\tdriver:\t%s.\n", 
        video_cap.card, video_cap.driver);
    }
}


/**
 * @brief check which control will be supported  by device. 
 * 
 */
void check_all_contol(){
    memset(&cam_ctrl_info, 0, sizeof(cam_ctrl_info));
    
    cam_ctrl_info.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while(ioctl(fd, VIDIOC_QUERYCTRL, &cam_ctrl_info) == 0){
        if(!(cam_ctrl_info.flags & V4L2_CTRL_FLAG_DISABLED)){
            VIDEO_DEBUG_PRINT("control: %s\n", cam_ctrl_info.name);

            if(cam_ctrl_info.type == V4L2_CTRL_TYPE_MENU){
                enumerate_menu(cam_ctrl_info.id);
            }
        }

        cam_ctrl_info.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }
}

void check_video_format(){
    memset(&video_fmt, 0, sizeof(video_fmt));
}

int main(int argc, char *argv[]) {

    signal(SIGTERM, programm_exit);    

    fd = open("/dev/video0", O_RDWR);
    if(fd == -1){
        fprintf(stderr, "video info: cannot open device! (%s (%d))\n", 
            strerror(errno), errno);
        return EXIT_FAILURE;
    }
    
    check_dev_cap();

    check_all_contol();
    
    memset(&video_fmt, 0, sizeof(video_fmt));

    video_fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_H264;
    if(ioctl(fd, VIDIOC_S_FMT, &video_fmt)==-1){
        fprintf(stderr, "video info: ! (%s (%d))\n", 
            strerror(errno), errno);
        errno_exit("video info");
    }else{
        if(!(video_cap.capabilities == V4L2_CAP_VIDEO_CAPTURE)){
            fprintf(stderr, "video info: this video is not camera. \n");
        }else
        {
            printf("Camera info:\tname:\t%s\n\tdriver:\t%s.\n", 
            video_cap.card, video_cap.driver);
        }
    }

    close(fd);
    return 0;
}