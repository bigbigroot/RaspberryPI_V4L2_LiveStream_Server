/**
 * @file capture.h
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __CAPTURE_H
#define __CAPTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define VIDEO_DEBUG
#define ENUM_CTRL 0

#ifdef VIDEO_DEBUG
    #define V4L2_MESSAGE(...)   do{\
                                    printf("V4L2 info: ");\
                                    printf(__VA_ARGS__);\
                                    printf("\n");\
                                }while (0)
                                
#else
    #define V4L2_MESSAGE(...) do{}while (0)
#endif

enum class WindowsSize{
    pixel_720p,
    pixel_1080p,
    pixel_5MP,
};

class VideoCapture
{
private:
    int fd;
    int imgSize;
    std::string deviceName;
    bool isOpen;
    struct{
        uint32_t height=0;
        uint32_t width=0;
    }windows;

#if(ENUM_CTRL > 0)
    void enumerateMenu(__u32 id, __u32 min_i, __u32 max_i);
#endif
public:
    VideoCapture()=delete;
    VideoCapture(std::string name);
    ~VideoCapture();
        
    /**
     * @brief open device by its name
     * 
     * @param dev_name device name for exmaple /dev/videoX
     * @return int if 0 sucess
     */
    void openDevice(void);
    /**
     * @brief close devide
     * 
     */
    void closeDevice(void);
    /**
     * @brief check the device is a video capture device (camera) and then print the 
     * info of the driver 
     * 
     */
    void checkDevCap(void);

    /**
     * @brief check which control will be supported  by device. 
     * 
     */
    void checkAllContol(void);
    /**
     * @brief check which image format will be supported  by device. 
     * 
     */
    void checkVideoFormat(void);

    /**
     * @brief set H.264's video stream format 
     * 
     */
    void setVideoFormat(void);
    /**
     * @brief 
     * 
     * @param buf 
     * @param len 
     * @return ssize_t 
     */
    int readVideoFrame(uint8_t *buf, size_t len);

    size_t getImageSize(){return imgSize;}
    
    void setWindow(WindowsSize win);
};

#endif /* __CAPTURE_H */