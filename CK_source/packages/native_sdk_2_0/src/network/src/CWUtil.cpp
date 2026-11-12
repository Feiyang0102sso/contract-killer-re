// CWUtil.cpp - URL parsing.
#include "CWUtil.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

                                                                        

// Overload that discards the "secure" flag.
int CWUtil::ParseUrl(const char* url, CStrChar& host, unsigned short& port, CStrChar& path)
{
    unsigned char secure;
    return ParseUrl(url, host, port, path, secure);
}

int CWUtil::ParseUrl(const char* url, CStrChar& host, unsigned short& port,
                     CStrChar& path, unsigned char& secure)
{
    host = (const char*)NULL;
    port = 80;
    path = (const char*)NULL;
    secure = 0;

    if (url == NULL) {
        return 1;
    }

    // host starts after "scheme://" (or at the beginning when there is no scheme)
    const char* hostStart = strstr(url, "://");
    if (hostStart != NULL) {
        hostStart = hostStart + 3;
    } else {
        hostStart = url;
    }

    const char* colon = strchr(hostStart, ':');
    const char* slash = strchr(hostStart, '/');
    // a ':' inside the path is not a port separator
    if (colon != NULL && slash != NULL && colon > slash) {
        colon = NULL;
    }

    host = hostStart;
    if (colon != NULL) {
        host = host.GetSubString(0, colon - hostStart - 1);
    } else if (slash != NULL) {
        host = host.GetSubString(0, slash - hostStart - 1);
    }

    if (colon != NULL) {
        // explicit port between ':' and '/'
        const char* portStart = colon + 1;
        CStrChar portText;
        portText = portStart;
        portText = portText.GetSubString(0, slash - portStart - 1);
        port = atoi(portText.GetCharPtr());
    } else {
        port = 80;
        if (hostStart > url + 3) {
            // a scheme is present: "https" switches to the secure default port
            const char* schemeEnd = hostStart - 3;
            CStrChar scheme(url);
            scheme = scheme.GetSubString(0, schemeEnd - url - 1);
            if (strcasecmp(scheme.GetCharPtr(), "https") == 0) {
                port = 443;
                secure = 1;
            }
        }
    }

    path = slash;
    return 0;
}

       
