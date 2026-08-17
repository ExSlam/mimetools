#pragma once

#include <cstddef>
#include <string>

struct Rfc2047DecodeResult
{
    std::size_t decodedWords;
    std::size_t skippedWords;
};

// Decode RFC 2047 encoded-words in a selected header fragment.
// targetCodePage is Scintilla's current document code page (0 means system ANSI).
Rfc2047DecodeResult decodeRfc2047Header(const char* input, std::size_t inputLength,
                                        unsigned int targetCodePage, std::string& output);
