// CTextParser.cpp - breaks a text into lines that fit a width with a given font.
#include "CTextParser.h"
#include "CFont.h"

                                                                         

CTextParser::CTextParser()
    : CClass(0x0275a643)
{
    m_classId = ClassId;
    m_font = NULL;
    m_text = NULL;
    m_width = 0;
    m_numLines = 0;
    m_lines.Clear();
}

CTextParser::~CTextParser()
{
}

int CTextParser::GetNumLines() const
{
    return m_numLines;
}

int CTextParser::GetIndexForLine(int line) const
{
    int index = 0;
    if (m_numLines > line) {
        // high half of the line entry
        m_lines.Get(line, index);
        index = (unsigned int)index >> 16;
    }
    return index;
}

int CTextParser::GetNumCharsForLine(int line) const
{
    int numChars = 0;
    if (m_numLines > line) {
        // low half of the line entry
        m_lines.Get(line, numChars);
        numChars = (unsigned short)numChars;
    }
    return numChars;
}

int CTextParser::Parse()
{
    m_numLines = 0;
    m_lines.Clear();
    if (m_font == NULL || m_text == NULL || m_width < 1) {
        return 0;
    }

    int fitCount = 0;
    int length = wcslen(m_text);
    int index = 0;
    for (;;) {
        int lineStart = index;
        unsigned int entry = (unsigned int)index << 16;
        int emptyLines = 0;
        for (;;) {
            if (lineStart >= length) {
                return 0;
            }
            int tokenType = m_font->GetTokenType(m_text[lineStart]);
            if (tokenType == CFont::TokenType_End) {
                return 0;
            }
            if (tokenType != CFont::TokenType_NewLine) {
                break;
            }
            lineStart++;
            entry += 0x10000;
            emptyLines++;
        }

        while (emptyLines > 0) {
            int result = m_lines.Add(entry & 0xff0000);
            if (result != 0) {
                return result;
            }
            m_numLines++;
            emptyLines--;
        }
        int width = m_font->MeasureTextWidth(m_text + lineStart, length - lineStart, m_width, &fitCount);
        fitCount = ParseLine(lineStart, fitCount, width);
        if (fitCount < 1) {
            return 0;
        }
        int result = m_lines.Add(entry | (fitCount & 0xffff));
        if (result != 0) {
            return result;
        }
        m_numLines++;
        index = lineStart + fitCount;
        int tokenType = m_font->GetTokenType(m_text[index]);
        if (tokenType == CFont::TokenType_Space || tokenType == CFont::TokenType_NewLine) {
            index++;
        }
    }
}

int CTextParser::ParseLine(int start, int count, int measuredWidth)
{
    int lineLength = count;
    unsigned char searching = 1;
    for (int position = start + count; position > start; position--) {
        int tokenType = m_font->GetTokenType(m_text[position]);
        switch (tokenType) {
        case CFont::TokenType_End:
        case CFont::TokenType_NewLine:
            lineLength = position - start;
            searching = 0;
            break;
        case CFont::TokenType_Space:
            // break before the space
            if (searching) {
                lineLength = position - start;
            }
            searching = 0;
            break;
        case CFont::TokenType_Break:
            // break after the break character when it still fits
            if (searching) {
                int charWidth = m_font->GetWidth(m_text[position]);
                int spacing = m_font->GetMinCharSpacing();
                if (position - start + 1 <= lineLength || m_width - measuredWidth >= charWidth + spacing) {
                    lineLength = position - start + 1;
                    searching = 0;
                }
            }
            break;
        }
    }
    return lineLength;
}

       
