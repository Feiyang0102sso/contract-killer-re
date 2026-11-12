// CNGSJSONParser.h - minimal JSON reader/writer for gServe messages.
// Decoding builds a CObjectMapValue tree (objects, arrays, strings, 64-bit integers, booleans, null);
// encoding writes such a tree back. All members are static.
#ifndef CNGSJSONPARSER_H
#define CNGSJSONPARSER_H

#include "CStrWChar.h"
#include "TCVector.h"
#include "CObjectMap.h"

class CWStringBuffer;

class CNGSJSONParser {
public:
    /** Resolve the backslash escapes of a JSON string body. */
    static CStrWChar decodeString(const CStrWChar& text);
    /** Escape the characters JSON strings cannot hold literally. */
    static CStrWChar encodeString(const CStrWChar& text);
    /** {"payload":[{"version":v, "sequenceId":id, "message":{...}}, ...]} */
    static CStrWChar encodeMessages(const TCVector<CObjectMap>& messages);
    static void encodeInteger(CWStringBuffer& buffer, long long value);
    static void encodeValue(CWStringBuffer& buffer, const CObjectMapValue* value);
    static CStrWChar encodeValue(const CObjectMapValue* value);
    /** Append the messages of a payload document to `messages`; returns how many were added. */
    static int decodeMessages(const CStrWChar& json,
                              TCVector<CObjectMap>& messages);
    /** Parse a JSON object or array (the caller owns the result); NULL on a syntax error. */
    static CObjectMapValue* decodeValue(const CStrWChar& json);
    /** Split `json` into tokens; 0 when the brackets do not match. */
    static int tokenize(TCVector<CStrWChar>& tokens,
                        CStrWChar json);
    static long long decodeInteger(const CStrWChar& text);
};

#endif
