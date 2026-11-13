// ResMap.h - resource name map read from res_map.dat ("<resource id>=<file name>" lines).
#ifndef RESMAP_H
#define RESMAP_H

#include <stddef.h>
#include "xString.h"
#include "HashTable.h"
#include "CStrChar.h"

/** Maps resource ids to file names and file names back to resource ids (0x18 bytes). */
class ResMap {
public:
    /** Map file name in the application bundle. */
    static const wchar_t* RESMAP_FILENAME;
    /** Separator of the two names of a line. */
    static const char* SEPARATOR;
    /** End of a record in the original map file. */
    static const char* RESMAP_NEWLINE_SEQ;

    ResMap();
    ~ResMap();
    /** Read and parse RESMAP_FILENAME. */
    void init();
    /** Parse `size` bytes of map text (lines end with \n). */
    void parseMapData(const char* data, unsigned int size);
    /** Parse one "<id>=<file>" line of `length` characters. */
    void parseMapStr(const char* line, unsigned int length);
    void addMapRecord(const char* resourceId, const char* fileName);
    /** `path` with '/' replaced by '\'. */
    static XString collapsePathToString(const XString& path);
    /** File name of the resource (empty when not mapped). */
    XString getFilename(const char* name) const;
    /** Resource id of file `fileName` (NULL when unknown or empty). */
    const char* getResourceID(const XString& fileName) const;
    /** Whole text resource as a new zero terminated buffer (caller delete[]s it). */
    char* loadText(const XString& name) const;

    HashTable<XString, XString> m_fileNames;                                    // 0x00 resource id -> file name
    HashTable<XString, CStrChar> m_resourceIds; // 0x0c file name -> resource id
};

#endif
