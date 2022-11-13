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

#include "capture.h"
#include "server.h"
#include "utility.h"


struct v4l2_requestbuffers video_reqbuf;

struct video_buffer_typ
{
    uint8_t *start;
    size_t len;
} *video_buffer;

/**
 * @brief will be called when programm exit by Ctrl-C
 * 
 */

int main(int argc, char *argv[]) {
    unsigned int i;

    // signal(SIGTERM, signal_handler);
    // signal(SIGINT, signal_handler);    

    std::unique_ptr<VideoCapture> camera = std::make_unique<VideoCapture>("/dev/video0");

    camera->setWindow(WindowsSize::pixel_720p);

    if(camera->openDevice() == -1) return EXIT_FAILURE;

    if(camera->checkDevCap() == -1) return EXIT_FAILURE;

    camera->checkAllContol();
    
    if(camera->checkVideoFormat() == -1) return EXIT_FAILURE;

    if(camera->setVideoFormat() == -1) return EXIT_FAILURE;

    video_buffer = (video_buffer_typ *)calloc(1, sizeof(*video_buffer));
    if(video_buffer == NULL){
        IO_ERROR_MESSAGE("cannot allocate enough buffer memory.");
    }

    video_buffer->len = camera->getImageSize();
    video_buffer->start = (uint8_t *)malloc(video_buffer->len);

    if(video_buffer->start == NULL){
        IO_ERROR_MESSAGE("cannot allocate enough buffer memory.");
    }

    
    FILE *output_file = fopen("output.264", "w");
    if(output_file == NULL){
        IO_ERROR_MESSAGE("cannot open file!");
        return EXIT_FAILURE;
    }
    // test read h.264 video stream and the write to file.
    for(i=0; i<1000; i++){
        if(camera->readVideoFrame(video_buffer->start, video_buffer->len) == -1){
            break;
        }
        
        fwrite(video_buffer->start, 1,video_buffer->len, output_file);
    }

    free(video_buffer->start);
    free(video_buffer);


    fclose(output_file);
    camera->closeDevice();
    return 0;
}