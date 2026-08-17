// This file is part of Notepad++ plugin MIME Tools project
// Copyright (C)2023 Don HO <don.h@free.fr>
// Enhance Base64 features, and rewrite Base64 encode/decode implementation
// Copyright 2019 by Paul Nankervis <paulnank@hotmail.com>
// Copyright 2024 by ExSlam <https://github.com/ExSlam>

#include "b64.h"

#include <climits>
#include <cstdint>

namespace {

constexpr char kBase64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr char kBase64UrlAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

constexpr int kIllegal = -1;
constexpr int kWhitespace = -2;
constexpr int kPadding = -3;

inline int checkedLength(std::size_t length)
{
    return length > static_cast<std::size_t>(INT_MAX) ? -1 : static_cast<int>(length);
}

inline int decodeValue(unsigned char c, bool urlSafe)
{
    // Preserve the legacy decoder's 7-bit lookup semantics for non-ASCII bytes.
    c &= 0x7f;
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+' || (urlSafe && c == '-')) return 62;
    if (c == '/' || (urlSafe && c == '_')) return 63;
    if (c == '=') return kPadding;
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return kWhitespace;
    return kIllegal;
}

inline std::size_t encodeBlock(char* out, const unsigned char* in, std::size_t length,
                               const char* alphabet, bool pad)
{
    std::size_t input = 0;
    std::size_t output = 0;

    // Hot path: three input bytes become four output bytes with no branches.
    while (input + 3 <= length)
    {
        const std::uint32_t bits =
            (static_cast<std::uint32_t>(in[input]) << 16) |
            (static_cast<std::uint32_t>(in[input + 1]) << 8) |
            static_cast<std::uint32_t>(in[input + 2]);
        input += 3;

        out[output++] = alphabet[(bits >> 18) & 0x3f];
        out[output++] = alphabet[(bits >> 12) & 0x3f];
        out[output++] = alphabet[(bits >> 6) & 0x3f];
        out[output++] = alphabet[bits & 0x3f];
    }

    const std::size_t remaining = length - input;
    if (remaining == 1)
    {
        const std::uint32_t bits = static_cast<std::uint32_t>(in[input]) << 16;
        out[output++] = alphabet[(bits >> 18) & 0x3f];
        out[output++] = alphabet[(bits >> 12) & 0x3f];
        if (pad)
        {
            out[output++] = '=';
            out[output++] = '=';
        }
    }
    else if (remaining == 2)
    {
        const std::uint32_t bits =
            (static_cast<std::uint32_t>(in[input]) << 16) |
            (static_cast<std::uint32_t>(in[input + 1]) << 8);
        out[output++] = alphabet[(bits >> 18) & 0x3f];
        out[output++] = alphabet[(bits >> 12) & 0x3f];
        out[output++] = alphabet[(bits >> 6) & 0x3f];
        if (pad)
            out[output++] = '=';
    }

    return output;
}

int encodeWrapped(char* resultString, const char* asciiString, std::size_t asciiStringLength,
                  std::size_t wrapLength, bool padFlag)
{
    const auto* input = reinterpret_cast<const unsigned char*>(asciiString);
    std::size_t inputIndex = 0;
    std::size_t resultLength = 0;
    std::size_t lineLength = 0;

    auto put = [&](char c) {
        if (wrapLength > 0 && lineLength >= wrapLength)
        {
            resultString[resultLength++] = '\n';
            lineLength = 0;
        }
        resultString[resultLength++] = c;
        ++lineLength;
    };

    while (inputIndex + 3 <= asciiStringLength)
    {
        const std::uint32_t bits =
            (static_cast<std::uint32_t>(input[inputIndex]) << 16) |
            (static_cast<std::uint32_t>(input[inputIndex + 1]) << 8) |
            static_cast<std::uint32_t>(input[inputIndex + 2]);
        inputIndex += 3;

        put(kBase64Alphabet[(bits >> 18) & 0x3f]);
        put(kBase64Alphabet[(bits >> 12) & 0x3f]);
        put(kBase64Alphabet[(bits >> 6) & 0x3f]);
        put(kBase64Alphabet[bits & 0x3f]);
    }

    const std::size_t remaining = asciiStringLength - inputIndex;
    if (remaining == 1)
    {
        const std::uint32_t bits = static_cast<std::uint32_t>(input[inputIndex]) << 16;
        put(kBase64Alphabet[(bits >> 18) & 0x3f]);
        put(kBase64Alphabet[(bits >> 12) & 0x3f]);
        if (padFlag)
        {
            put('=');
            put('=');
        }
    }
    else if (remaining == 2)
    {
        const std::uint32_t bits =
            (static_cast<std::uint32_t>(input[inputIndex]) << 16) |
            (static_cast<std::uint32_t>(input[inputIndex + 1]) << 8);
        put(kBase64Alphabet[(bits >> 18) & 0x3f]);
        put(kBase64Alphabet[(bits >> 12) & 0x3f]);
        put(kBase64Alphabet[(bits >> 6) & 0x3f]);
        if (padFlag)
            put('=');
    }

    return checkedLength(resultLength);
}

int encodeByLine(char* resultString, const char* asciiString, std::size_t asciiStringLength,
                 bool padFlag)
{
    std::size_t inputIndex = 0;
    std::size_t outputIndex = 0;

    while (inputIndex < asciiStringLength)
    {
        const std::size_t lineStart = inputIndex;
        while (inputIndex < asciiStringLength &&
               asciiString[inputIndex] != '\r' && asciiString[inputIndex] != '\n')
        {
            ++inputIndex;
        }

        outputIndex += encodeBlock(
            resultString + outputIndex,
            reinterpret_cast<const unsigned char*>(asciiString + lineStart),
            inputIndex - lineStart,
            kBase64Alphabet,
            padFlag);

        // Preserve the exact EOL byte sequence, including CRLF and consecutive blank lines.
        while (inputIndex < asciiStringLength &&
               (asciiString[inputIndex] == '\r' || asciiString[inputIndex] == '\n'))
        {
            resultString[outputIndex++] = asciiString[inputIndex++];
        }
    }

    return checkedLength(outputIndex);
}

int decodeImpl(char* resultString, const char* encodedString, std::size_t encodedStringLength,
               bool strictFlag, bool whitespaceReset, bool urlSafe)
{
    std::size_t index = 0;
    std::size_t resultLength = 0;
    int padLength = 0;

    while (index < encodedStringLength)
    {
        std::uint32_t bitField = 0;
        int bitOffset = 18;
        int charValue = 0;
        int charIndex = 0;

        while (bitOffset >= 0 && index < encodedStringLength)
        {
            charValue = static_cast<unsigned char>(encodedString[index++]);
            charIndex = decodeValue(static_cast<unsigned char>(charValue), urlSafe);

            if (charIndex >= 0)
            {
                if (padLength > 0 && strictFlag)
                    return -1; // Data after pad character.
                bitField |= static_cast<std::uint32_t>(charIndex) << bitOffset;
                bitOffset -= 6;
            }
            else if (charIndex == kPadding)
            {
                ++padLength;
                if (strictFlag && bitOffset > 6)
                    return -2; // Pad character in wrong place.
            }
            else if (charIndex == kIllegal || whitespaceReset)
            {
                charIndex = kIllegal;
                break;
            }
            // Whitespace is otherwise ignored.
        }

        if (strictFlag && bitOffset == 12)
            return -3; // Single-symbol block is invalid.

        const int endOffset = bitOffset + 3;
        for (int outputOffset = 16; outputOffset > endOffset; outputOffset -= 8)
            resultString[resultLength++] = static_cast<char>((bitField >> outputOffset) & 0xff);

        if (charIndex == kIllegal)
        {
            if (strictFlag)
                return -4;
            resultString[resultLength++] = static_cast<char>(charValue);
        }
    }

    return checkedLength(resultLength);
}

} // namespace

int base64Encode(char* resultString, const char* asciiString, std::size_t asciiStringLength,
                 std::size_t wrapLength, bool padFlag, bool byLineFlag)
{
    if (byLineFlag)
        return encodeByLine(resultString, asciiString, asciiStringLength, false);

    if (wrapLength == 0)
    {
        const std::size_t resultLength = encodeBlock(
            resultString,
            reinterpret_cast<const unsigned char*>(asciiString),
            asciiStringLength,
            kBase64Alphabet,
            padFlag);
        return checkedLength(resultLength);
    }

    return encodeWrapped(resultString, asciiString, asciiStringLength, wrapLength, padFlag);
}

int base64Decode(char* resultString, const char* encodedString, std::size_t encodedStringLength,
                 bool strictFlag, bool whitespaceReset)
{
    return decodeImpl(resultString, encodedString, encodedStringLength,
                      strictFlag, whitespaceReset, false);
}

int base64EncodeWithPaddingByLine(std::string& resultString, const char* asciiString,
                                  std::size_t asciiStringLength)
{
    // Worst case is 4/3 expansion plus the original line endings.
    resultString.clear();
    resultString.reserve(((asciiStringLength + 2) / 3) * 4 + asciiStringLength / 32 + 4);

    std::size_t inputIndex = 0;
    char encoded[4];

    while (inputIndex < asciiStringLength)
    {
        const std::size_t lineStart = inputIndex;
        while (inputIndex < asciiStringLength &&
               asciiString[inputIndex] != '\r' && asciiString[inputIndex] != '\n')
        {
            ++inputIndex;
        }

        std::size_t lineIndex = lineStart;
        const std::size_t lineEnd = inputIndex;
        while (lineIndex < lineEnd)
        {
            const std::size_t chunk = (lineEnd - lineIndex >= 3) ? 3 : (lineEnd - lineIndex);
            const std::size_t produced = encodeBlock(
                encoded,
                reinterpret_cast<const unsigned char*>(asciiString + lineIndex),
                chunk,
                kBase64Alphabet,
                true);
            resultString.append(encoded, produced);
            lineIndex += chunk;
        }

        while (inputIndex < asciiStringLength &&
               (asciiString[inputIndex] == '\r' || asciiString[inputIndex] == '\n'))
        {
            resultString.push_back(asciiString[inputIndex++]);
        }
    }

    return checkedLength(resultString.size());
}

int base64UrlEncode(char* resultString, const char* asciiString, std::size_t asciiStringLength)
{
    const std::size_t resultLength = encodeBlock(
        resultString,
        reinterpret_cast<const unsigned char*>(asciiString),
        asciiStringLength,
        kBase64UrlAlphabet,
        false);
    return checkedLength(resultLength);
}

int base64UrlDecode(char* resultString, const char* encodedString, std::size_t encodedStringLength,
                    bool strictFlag, bool whitespaceReset)
{
    return decodeImpl(resultString, encodedString, encodedStringLength,
                      strictFlag, whitespaceReset, true);
}
