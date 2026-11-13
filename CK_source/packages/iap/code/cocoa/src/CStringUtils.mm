// CStringUtils.mm - NSString <-> CStrWChar conversion (UTF-32 little endian = wchar_t on iOS).
#include "CStringUtils.h"

#include <wchar.h>
#include "CStrWCharBuffer.h"

CStrWChar CStringUtils::NSStringToCStrWChar(NSString* string)
{
    const wchar_t* utf32 = reinterpret_cast<const wchar_t*>(
        [string cStringUsingEncoding:NSUTF32LittleEndianStringEncoding]);
    if (utf32 == NULL) {
        return CStrWChar();
    }
    // NSString's length counts UTF-16 units. Measure the converted UTF-32 string.
    unsigned long length = wcslen(utf32);
    CStrWCharBuffer buffer(length);
    buffer.Append(utf32, 0, length);
    return CStrWChar(buffer.GetWCharPtr());
}

NSString* CStringUtils::CStrWCharToNSString(const CStrWChar& string)
{
    return [[NSString alloc] initWithBytes:string.GetWCharPtr()
                                    length:string.GetLength() * sizeof(wchar_t)
                                  encoding:NSUTF32LittleEndianStringEncoding];
}
