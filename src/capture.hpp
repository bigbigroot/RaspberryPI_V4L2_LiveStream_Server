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

#include <linux/videodev2.h>    /* video for linux two header file */

#include <functional>
#include <string>
#include <queue>

#define VIDEO_DEBUG
#define ENUM_CTRL 1

#ifdef VIDEO_DEBUG
    #define V4L2_MESSAGE(...)   do{\
                                    printf("V4L2 info: ");\
                                    printf(__VA_ARGS__);\
                                    printf("\n");\
                                }while (0)
                                
#else
    #define V4L2_MESSAGE(...) do{}while (0)
#endif

enum class H264Profile: int
{
    Baseline = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE,
    Constrained_Baseline = V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE,
    Main = V4L2_MPEG_VIDEO_H264_PROFILE_MAIN,
    High = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH
};

enum class H264Level: int
{
    Level1 = V4L2_MPEG_VIDEO_H264_LEVEL_1_0,
    Level1b = V4L2_MPEG_VIDEO_H264_LEVEL_1B,
    Level1_1 = V4L2_MPEG_VIDEO_H264_LEVEL_1_1,
    Level1_2 = V4L2_MPEG_VIDEO_H264_LEVEL_1_2,
    Level1_3 = V4L2_MPEG_VIDEO_H264_LEVEL_1_3,
    Level2 = V4L2_MPEG_VIDEO_H264_LEVEL_2_0,
    Level2_1 = V4L2_MPEG_VIDEO_H264_LEVEL_2_1,
    Level2_2 = V4L2_MPEG_VIDEO_H264_LEVEL_2_2,
    Level3 = V4L2_MPEG_VIDEO_H264_LEVEL_3_0,
    Level3_1 = V4L2_MPEG_VIDEO_H264_LEVEL_3_1,
    Level3_2 = V4L2_MPEG_VIDEO_H264_LEVEL_3_2,
    Level4 = V4L2_MPEG_VIDEO_H264_LEVEL_4_0,
    Level4_1 = V4L2_MPEG_VIDEO_H264_LEVEL_4_1,
    Level4_2 = V4L2_MPEG_VIDEO_H264_LEVEL_4_2
};

class VideoCapture
{
    private:
        int fd;
        int imgSize;
        std::string deviceName;
        bool isOpened;
        struct{
            uint32_t height;
            uint32_t width;
        }windows;

    #if(ENUM_CTRL > 0)
        void enumerateMenu(uint32_t id, uint32_t min_i, uint32_t max_i);
    #endif
    
        static constexpr size_t videoBuffersNum = 5;
        struct buffer
        {
            void *start;
            size_t length;
        };

        std::deque<buffer> videoBuffer;
        
    public:
        enum class WindowsSize{
            pixel_720p,
            pixel_1080p,
            pixel_5MP,
        };
        
        VideoCapture()=delete;
        VideoCapture(std::string name);
        ~VideoCapture();
        
        std::function<void(void *, size_t)> onSample = nullptr;
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

        void setH264ProfileAndLevel();
        /**
         * @brief read a picture
         * 
         * @param buf 
         * @param len 
         * @return ssize_t 
         */
        int readVideoFrame(uint8_t *buf, size_t len);
        /**
         * @brief 
         * 
         */
        void initMmap(void);


        void handleLoop();
        /**
         * @brief Get the Image Size (only for read method)
         * 
         * @return size_t 
         */
        size_t getImageSize(){return imgSize;}

        /**
         * @brief Set the resolution of the Video
         * 
         * @param win resolution
         */
        void setWindow(WindowsSize win);

        void setH264ProfileAndLevel(H264Profile profile, H264Level level);
};

#endif /* __CAPTURE_H */