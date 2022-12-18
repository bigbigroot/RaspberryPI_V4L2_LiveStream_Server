/**
 * @file uid.cpp
 * @author Weigen Huang (weigen.huang.k7e@fh-zwickau.de)
 * @brief 
 * @version 0.1
 * @date 2022-12-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "random_id.hpp"
#include <stdint.h>
#include <array>

constexpr uint32_t bit6mask = 0x0000003fUL;
constexpr uint32_t bit4mask = 0x0000000fUL;
constexpr uint32_t bit2mask = 0x00000003UL;

std::array<char, 64> constexpr base64Table
{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'G', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z', '-', '_'
};


RandomID::RandomID()
{
    rng.seed(rd());
}

std::string RandomID::uniqueIdgenerator()
{
    constexpr int idSize = 16;
    char id[idSize];
    uint32_t r = rng();
    uint8_t reserve=0;
    for(int i=0; i<16; i++)
    {
        if(i==6)
        {
            reserve = r;
            r = rng();
            id[(idSize-1)-i] = base64Table[((r&bit4mask)<<2) + reserve];
            r >>= 4;
        }else if(i==11)
        {
            reserve = r;
            r = rng();
            id[(idSize-1)-i] = base64Table[((r&bit2mask)<<4) + reserve];
            r >>= 2;
        }else{
            id[(idSize-1)-i] = base64Table[r&bit6mask];
            r >>= 6;
        }
    }
    return std::string(id, idSize);
}
