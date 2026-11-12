// CWUtil.h - web helpers.
#ifndef CWUTIL_H
#define CWUTIL_H

#include "CStrChar.h"

                                                                        

class CWUtil {
public:
    /**
     * Split "scheme://host[:port]/path" into host, port and path. Without a port the default
     * is 80, or 443 with `secure` = 1 for "https". Returns 0 on success, 1 for a NULL url.
     */
    static int ParseUrl(const char* url, CStrChar& host, unsigned short& port,
                        CStrChar& path);
    static int ParseUrl(const char* url, CStrChar& host, unsigned short& port,
                        CStrChar& path, unsigned char& secure);
};

       

#endif
