// This file is part of Notepad++ plugin MIME Tools project
// Copyright (C)2023 Don HO <don.h@free.fr>
// Optimization pass prepared against ExSlam/mimetools master (2e20af5), 2026.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qp.h"

#include <cstring>

char* QuotedPrintable::encode(const char* str)
{
    return encode(str, std::strlen(str));
}

char* QuotedPrintable::encode(const char* str, std::size_t len)
{
    initVar();

    _bufLen = len * 3 + 1;
    const std::size_t nbEOL = (len * 3) / QP_ENCODED_LINE_LEN_MAX;
    _bufLen += nbEOL * 3;

    _buffer = new char[_bufLen];

    for (std::size_t i = 0; i < len; ++i)
    {
        getQPChar(str[i]);
        putQPChar();
    }
    _buffer[_i] = '\0';
    return _buffer;
}

void QuotedPrintable::getQPChar(char c)
{
    bool crlf = false;
    const auto uc = static_cast<unsigned char>(c);

    if ((c != '=' && c > 32 && c < 127) || c == ' ' || c == '\t' || uc == 0x0D)
    {
        _chars[0] = c;
        _nbChar = 1;
    }
    else if (uc == 0x0A)
    {
        _chars[0] = c;
        _nbChar = 1;
        crlf = true;
    }
    else
    {
        _chars[0] = '=';
        _chars[1] = toChar(uc >> 4);
        _chars[2] = toChar(uc & 15);
        _nbChar = 3;
    }

    if (crlf)
        _nbCharInLine = _nbChar;
    else
        _nbCharInLine += _nbChar;

    if (_nbCharInLine >= QP_ENCODED_LINE_LEN_MAX)
    {
        if (_i + 3 >= _bufLen)
        {
            const std::size_t oldLen = _bufLen;
            _bufLen = _bufLen * 2 + 4;
            char* newBuf = new char[_bufLen];
            std::memcpy(newBuf, _buffer, oldLen);
            delete [] _buffer;
            _buffer = newBuf;
        }
        _buffer[_i++] = '=';
        _buffer[_i++] = '\r';
        _buffer[_i++] = '\n';
        _nbCharInLine = _nbChar;
    }
}

void QuotedPrintable::putQPChar()
{
    if (_i + static_cast<std::size_t>(_nbChar) >= _bufLen)
    {
        const std::size_t oldLen = _bufLen;
        _bufLen = _bufLen * 2 + static_cast<std::size_t>(_nbChar) + 1;
        char* newBuf = new char[_bufLen];
        std::memcpy(newBuf, _buffer, oldLen);
        delete [] _buffer;
        _buffer = newBuf;
    }

    for (int i = 0; i < _nbChar; ++i)
        _buffer[_i++] = _chars[i];
}

char* QuotedPrintable::decode(const char* str)
{
    return decode(str, std::strlen(str));
}

char* QuotedPrintable::decode(const char* str, std::size_t len)
{
    initVar();
    _bufLen = len + 1;
    _buffer = new char[_bufLen];

    // Single linear pass. The previous implementation repeatedly called strlen()
    // on the remaining suffix and copied each line into a full-size temporary buffer.
    for (std::size_t i = 0; i < len; )
    {
        const char c = str[i];

        if (c == '=')
        {
            // Soft line break: remove =CRLF entirely.
            if (i + 2 < len && str[i + 1] == '\r' && str[i + 2] == '\n')
            {
                i += 3;
                continue;
            }

            if (i + 2 >= len)
                return nullptr;

            unsigned char restored = 0;
            if (!makeChar(str[i + 1], str[i + 2], restored) || restored == 0)
                return nullptr; // Preserve the legacy decoder's rejection of =00.

            _buffer[_i++] = static_cast<char>(restored);
            i += 3;
            continue;
        }

        if (c == '\r')
        {
            if (i + 1 >= len || str[i + 1] != '\n')
                return nullptr;
            _buffer[_i++] = '\r';
            _buffer[_i++] = '\n';
            i += 2;
            continue;
        }

        // Preserve the existing decoder's requirement that physical newlines are CRLF.
        if (c == '\n')
            return nullptr;

        _buffer[_i++] = c;
        ++i;
    }

    _buffer[_i] = '\0';
    return _buffer;
}
