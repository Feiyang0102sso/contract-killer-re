// CHTTPNative.h - thin C++ wrapper around the NSURLConnection based HTTPHandler (MRC).
#ifndef CHTTPNATIVE_H
#define CHTTPNATIVE_H

class CHTTPNative {
public:
    CHTTPNative();
    ~CHTTPNative();

    /** Cancels the running connection (if any). */
    void Cancel();
    /** Drops the last response string. */
    void ClearResponse();
    /** Starts an asynchronous request; ignored while a connection is running. */
    void Send(unsigned long timeoutMs, const char* url, const char* body, unsigned long bodyLength,
              const char* userAgent, const char* contentType, const char* method);
    bool IsBusy() const;
    /** NSError code of the failed connection, 0 when none. */
    int GetError();
    /** UTF-8 response text, NULL when nothing was received. */
    const char* GetResponsePtr();
    unsigned int GetResponseLen();

    void* m_handler;  // 0x00 HTTPHandler*
};

#endif
