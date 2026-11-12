// CFont.cpp - common base of the fonts: character classification for the line breaker.
#include "CFont.h"

CFont::CFont()
    : m_classId(ClassId)
{
}

CFont::~CFont()
{
}
                                                                         

int CFont::GetTokenType(wchar_t character)
{
    int tokenType = TokenType_Normal;
    switch (character) {
    case L'-':
    case L'@':
        tokenType = TokenType_Break;
        break;
    case L'\n':
        tokenType = TokenType_NewLine;
        break;
    case L' ':
        tokenType = TokenType_Space;
        break;
    case L'\t':
        tokenType = TokenType_Tab;
        break;
    case L'\0':
        tokenType = TokenType_End;
        break;
    }
    return tokenType;
}

       
