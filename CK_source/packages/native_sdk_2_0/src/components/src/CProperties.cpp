// CProperties.cpp - "key = value" settings file.
#include <stdlib.h>
#include <strings.h>
#include "CProperties.h"
#include "CInputStream.h"
#include "CStringToKey.h"

                                                                           

/** Longest line chunk ReadLine assembles before appending (it flushes every character). */
static const int kReadLineBufferSize = 200;

CProperties::CProperties()
    : CClass(ClassId)
{
    m_overriddenKeys = NULL;
}

CProperties::~CProperties()
{
    Destroy();
}

void CProperties::Destroy()
{
    while (m_settings.Size() > 0) {
        int value;
        m_settings.Remove(0, value);
        delete (Setting*)value;
    }
    m_overriddenKeys = NULL;
}

void CProperties::Load(CInputStream& stream, unsigned char merge)
{
    if (!merge) {
        Destroy();
    }

    while (stream.Available()) {
        CStrChar line;
        CStrChar key;
        CStrChar value;
        ReadLine(stream, line);

        // skip empty lines and comments ('#' or '!')
        if (line.m_length == 0 || line.m_string[0] == '#' || line.m_string[0] == '!') {
            continue;
        }

        int separator = line.Find("=", 0);
        if (separator < 0) {
            separator = line.Find(":", 0);
        }
        if (separator >= 0) {
            key = line.GetSubString(0, separator - 1);
            value = line.GetSubString(separator + 1, line.m_length - 1);
        } else {
            key = line;
        }
        key = key.Trim();
        value = value.Trim();

        if (key.m_length < 1 || value.m_length < 1) {
            continue;
        }

        if (merge && Exists(key.m_string)) {
            if (Replace(key.m_string, value.m_string) && m_overriddenKeys != NULL) {
                int index = 0;
                for (; index < m_overriddenKeys->Size(); index++) {
                    if (*(CStrChar*)m_overriddenKeys->m_data[index] == key) {
                        break;
                    }
                }
                if (index >= m_overriddenKeys->Size()) {
                    CStrChar* overridden = new CStrChar(key.m_string);
                    if (m_overriddenKeys->Add((int)overridden) != 0) {
                        delete overridden;
                    }
                }
            }
        } else {
            Add(key.m_string, value.m_string);
        }
    }
}

void CProperties::ReadLine(CInputStream& stream, CStrChar& line)
{
    line = "";
    if (!stream.Available()) {
        return;
    }

    char buffer[kReadLineBufferSize];
    unsigned char endOfLine = 0;
    int length = 0;
    do {
        buffer[length] = stream.ReadInt8();
        if (buffer[length] == '\n') {
            buffer[length] = '\0';
            endOfLine = 1;
        } else {
            length++;
        }
        if (length > 0) {
            buffer[length] = '\0';
            line.Concatenate(buffer);
            length = 0;
            buffer[0] = '\0';
        }
    } while (stream.Available() && !endOfLine);
}

unsigned char CProperties::Exists(const char* key)
{
    return KeyToIndex(CStringToKey(key, 0)) >= 0;
}

unsigned char CProperties::Replace(const char* key, const char* value)
{
    if (key == NULL || key[0] == '\0' || value == NULL || value[0] == '\0') {
        return 0;
    }

    int index = KeyToIndex(CStringToKey(key, 0));
    if (index < 0) {
        return 0;
    }
    ((Setting*)m_settings.m_data[index])->m_value = value;
    return 1;
}

unsigned char CProperties::Add(const char* key, const char* value)
{
    if (key == NULL || key[0] == '\0' || value == NULL || value[0] == '\0') {
        return 0;
    }

    unsigned int keyHash = CStringToKey(key, 0);
    if (KeyToIndex(keyHash) > -1) {
        return 0;
    }

    Setting* added = new Setting();
    added->m_keyHash = keyHash;
    added->m_value = value;
    if (m_settings.Add((int)added) == 0) {
        return 1;
    }
    delete added;
    return 0;
}

int CProperties::KeyToIndex(unsigned int keyHash)
{
    for (int index = 0; index < m_settings.Size(); index++) {
        Setting* setting = (Setting*)m_settings.m_data[index];
        if (setting->m_keyHash == keyHash) {
            return index;
        }
    }
    return -1;
}

const char* CProperties::GetStr(const char* key)
{
    if (key == NULL) {
        return NULL;
    }
    int index = KeyToIndex(CStringToKey(key, 0));
    if (index < 0) {
        return NULL;
    }
    return ((Setting*)m_settings.m_data[index])->m_value.m_string;
}

int CProperties::GetInt(const char* key)
{
    if (key == NULL) {
        return 0;
    }
    int index = KeyToIndex(CStringToKey(key, 0));
    if (index < 0) {
        return 0;
    }
    return atoi(((Setting*)m_settings.m_data[index])->m_value.m_string);
}

unsigned char CProperties::GetBool(const char* key)
{
    if (key == NULL) {
        return 0;
    }
    int index = KeyToIndex(CStringToKey(key, 0));
    if (index < 0) {
        return 0;
    }
    return strcasecmp(((Setting*)m_settings.m_data[index])->m_value.m_string, "true") == 0;
}

void CProperties::CollectOverriddenKeysInto(CVector* keys)
{
    m_overriddenKeys = keys;
}

       
