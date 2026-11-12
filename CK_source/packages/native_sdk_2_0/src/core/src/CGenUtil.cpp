// CGenUtil.cpp - miscellaneous helpers of the Glu platform core.
#include "CGenUtil.h"

                                                                     

unsigned int CGenUtil::HexAToI(const char* text)
{
    unsigned int value = 0;

    if (text == NULL) {
        return value;
    }

    while (*text != '\0') {
        char character = *text;
        unsigned char digit;

        if (character >= '0' && character <= '9') {
            digit = character - '0';
        } else if (character == 'A' || character == 'a') {
            digit = 10;
        } else if (character == 'B' || character == 'b') {
            digit = 11;
        } else if (character == 'C' || character == 'c') {
            digit = 12;
        } else if (character == 'D' || character == 'd') {
            digit = 13;
        } else if (character == 'E' || character == 'e') {
            digit = 14;
        } else if (character == 'F' || character == 'f') {
            digit = 15;
        } else {
            // not a hex digit: stop parsing
            return value;
        }

        value = (value << 4) | digit;
        text++;
    }

    return value;
}

       

void CGenUtil::ClipRegionToRegion(short clipStart, short clipLength, short& start, short& length)
{
    short originalLength = length;
    if (clipStart + clipLength > start && clipStart < start + length) {
        if (start + length > clipStart + clipLength) {
            length = clipStart + clipLength - start;
        }
        if (start < clipStart) {
            length = length + (start - clipStart);
            start = clipStart;
        }
        if ((originalLength > 0 && length < 0) || (originalLength < 0 && length > 0)) {
            length = 0;
        }
    } else {
        length = 0;
    }
}
