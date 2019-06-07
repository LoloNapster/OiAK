#ifndef FLOAT80_H
#define FLOAT80_H
#include <stdint.h>


class Float80
{
    public:
        Float80();

        uint16_t signAndExponent;
        uint64_t mantissa;
};

#endif // FLOAT80_H

