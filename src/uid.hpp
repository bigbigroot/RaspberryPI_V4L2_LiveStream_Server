/**
 * @file uid.hpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-12-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __UID_H
#define __UID_H

#include <string>
#include <random>

class UniqueID
{
private:
    std::random_device rd;
    std::mt19937 rng;
public:
    UniqueID();
    ~UniqueID()=default;
    std::string uniqueIdgenerator();
};


#endif /* __UID_H */