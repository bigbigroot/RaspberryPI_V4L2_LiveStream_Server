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

#include "server.h"

static int fd;


struct v4l2_format video_fmt;
struct v4l2_requestbuffers video_reqbuf;

struct video_buffer_typ
{
    void *start;
    size_t len;
} *video_buffer;

#define VIDEO_DEBUG

#ifdef VIDEO_DEBUG
    #define VIDEO_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define VIDEO_DEBUG_PRINT(...) 
#endif
/**
 * @brief will be called when programm exit by Ctrl-C
 * 
 */
void signal_handler(){

    close(fd);
    fprintf(stderr, "error %d: %s\n", 
        errno, strerror(errno));
}

static void errno_exit(const char *s){
    close(fd);
    fprintf(stderr, "%s error %d, %s\n", s, 
        errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static void enumerate_menu(__u32 id, int min_i, int max_i){
    struct v4l2_querymenu  cam_ctrl_menu_info;

    VIDEO_DEBUG_PRINT("\tControl Menu items:\n");

    memset(&cam_ctrl_menu_info, 0, sizeof(cam_ctrl_menu_info));

    cam_ctrl_menu_info.id = id;

    for (cam_ctrl_menu_info.index = min_i;
         cam_ctrl_menu_info.index <= max_i;
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
    struct v4l2_capability video_cap;

    memset(&video_cap, 0, sizeof(video_cap));

    if(ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1){
        errno_exit("VIDIOC_QUERYCAP");
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
    struct v4l2_queryctrl  cam_ctrl_info;

    memset(&cam_ctrl_info, 0, sizeof(cam_ctrl_info));
    
    cam_ctrl_info.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while(ioctl(fd, VIDIOC_QUERYCTRL, &cam_ctrl_info) == 0){
        if(!(cam_ctrl_info.flags & V4L2_CTRL_FLAG_DISABLED)){
            VIDEO_DEBUG_PRINT("control: %s\n", cam_ctrl_info.name);

            if(cam_ctrl_info.type == V4L2_CTRL_TYPE_MENU){
                enumerate_menu(cam_ctrl_info.id, cam_ctrl_info.minimum, 
                    cam_ctrl_info.maximum);
            }
        }

        cam_ctrl_info.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }
}
/**
 * @brief check which image format will be supported  by device. 
 * 
 */
void check_video_format(){
    struct v4l2_fmtdesc video_fmtdesc;
    
    memset(&video_fmtdesc, 0, sizeof(video_fmtdesc));
    
    video_fmtdesc.index=0;
    video_fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while(ioctl(fd, VIDIOC_ENUM_FMT, &video_fmtdesc) == 0){
            VIDEO_DEBUG_PRINT("format: [%d] %s\n", video_fmtdesc.index, 
                video_fmtdesc.description);
            video_fmtdesc.index++;
    }

    if(video_fmtdesc.index == 0){
        fprintf(stderr, "video info: VIDIOC_ENUM_FMT (%s (%d))\n", 
            strerror(errno), errno);
    }
    
}

/**
 * @brief check whether driver support mmap 
 * 
 */
void check_io_buffer(){

}

int main(int argc, char *argv[]) {
    unsigned int i;

    int client_fd;
    tcp_server *server;

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);    

    fd = open("/dev/video0", O_RDWR);
    if(fd == -1){
        fprintf(stderr, "video info: cannot open device! (%s (%d))\n", 
            strerror(errno), errno);
        return EXIT_FAILURE;
    }

    // FILE *output_file = fopen("output.jpg", "w");
    // if(output_file == NULL){
    //     fprintf(stderr, "video info: cannot open file! (%s (%d))\n", 
    //         strerror(errno), errno);
    //     return EXIT_FAILURE;
    // }
    
    check_dev_cap();

    check_all_contol();
    
    check_video_format();

    memset(&video_fmt, 0, sizeof(video_fmt));

    video_fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_G_FMT, &video_fmt) == -1){
        errno_exit("VIDIOC_G_FMT");
    }

    video_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    if(ioctl(fd, VIDIOC_S_FMT, &video_fmt) == -1){
        errno_exit("VIDIOC_S_FMT");
    }

    video_buffer = calloc(1, sizeof(*video_buffer));
    if(video_buffer == NULL){
        errno_exit("cannot allocate enough buffer memory.\n");
    }

    video_buffer->len = video_fmt.fmt.pix.sizeimage;
    video_buffer->start = malloc(video_buffer->len);

    if(video_buffer->start == NULL){
        errno_exit("cannot allocate enough buffer memory.\n");
    }

    server = new_tcp_server();

    server->port=8000;
    if(open_tcp_server(server) == -1){
        return 1;
    }

    client_fd = tcp_server_accept(server);
    if(client_fd == -1){
        return 1;
    }
    for(i=0; i<100000; i++){
        if(read(fd, video_buffer->start, video_buffer->len) == -1){
            errno_exit("cannot reading\n");
        }
        
        if(tcp_send(client_fd, video_buffer->start, video_buffer->len) == -1) break;
    }

    close(client_fd);
    close_free_tcp_server(server);
    free(video_buffer->start);
    free(video_buffer);
    // fclose(output_file);
    // memset(&video_reqbuf, 0, sizeof(video_reqbuf));

    // video_reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // video_reqbuf.memory = V4L2_MEMORY_MMAP;
    // video_reqbuf.count = 100;
     
    // if(ioctl(fd, VIDIOC_REQBUFS, &video_reqbuf)){
    //     errno_exit("VIDIOC_REQBUFS");
    // }

    // if(video_reqbuf.count<20){
    //     fprintf(stderr, "size %d\n", video_reqbuf.count);
    //     errno_exit("video info: not enough buffer memory \n");
    // }

    // buffers = calloc(video_reqbuf.count, sizeof(*buffers));
    // assert(buffers != NULL);

    // for (i = 0; i < video_reqbuf.count; i++){
    //     struct v4l2_buffer buffer;

    //     memset(&buffer, 0, sizeof(buffer));
    //     buffer.type = video_reqbuf.type;
    //     buffer.memory = V4L2_MEMORY_MMAP;
    //     buffer.index = 1;

    //     if(ioctl(fd, VIDIOC_QUERYBUF, &buffer)){
    //         errno_exit("VIDIOC_QUERYBUF");
    //     }

    //     buffers[i].len = buffer.length;

    //     buffers[i].start = mmap(NULL, buffer.length, 
    //         PROT_READ | PROT_WRITE,
    //         MAP_SHARED, fd, buffer.m.offset);

    //     if(buffers[i].start == MAP_FAILED){
    //         errno_exit("mmap");
    //     }
    // }
    
    // for (i = 0; i < video_reqbuf.count; i++){
    //     fwrite(buffers[i].start, buffers[i].len, sizeof(char), output_file);
    // }
    
    // for (i = 0; i < video_reqbuf.count; i++){
    //     munmap(buffers[i].start, buffers[i].len);
    // }

    close(fd);
    return 0;
}