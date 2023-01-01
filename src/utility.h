/**
 * @file utility.h
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __UTILITY_H
#define __UTILITY_H

#include <stdio.h>

#define ERROR_MESSAGE(...)   do{\
                                fprintf(stderr, "error: ");\
                                fprintf(stderr, __VA_ARGS__);\
                                fprintf(stderr, "\n");\
                            }while (0)


#endif /* __UTILITY_H */