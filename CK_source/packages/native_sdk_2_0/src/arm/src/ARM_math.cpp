// ARM_math.cpp - fixed-point helpers.
#include "ARM_math.h"

                                                                    

int smult16(int a, int b)
{
    // The ARM VFP conversion truncates toward zero.
    return (int)((double)a * (double)b * (1.0 / 65536.0));
}

       
