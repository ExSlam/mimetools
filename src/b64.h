// This file is part of Notepad++ plugin MIME Tools project
// Copyright (C)2023 Don HO <don.h@free.fr>

#pragma once

#include <cstddef>
#include <string>

int base64Encode(char *resultString, const char *asciiString, std::size_t asciiStringLength,
                 std::size_t wrapLength, bool padFlag, bool byLineFlag);
int base64Decode(char *resultString, const char *encodedString, std::size_t encodedStringLength,
                 bool strictFlag, bool whitespaceReset);
int base64EncodeWithPaddingByLine(std::string& resultString, const char* asciiString,
                                  std::size_t asciiStringLength);

// RFC 4648 base64url helpers. Encoding is unpadded and uses '-'/'_' directly.
// Decoding accepts both URL-safe and standard Base64 alphabet symbols, matching
// the previous MIME Tools Base64URL wrapper's permissive behavior.
int base64UrlEncode(char *resultString, const char *asciiString, std::size_t asciiStringLength);
int base64UrlDecode(char *resultString, const char *encodedString, std::size_t encodedStringLength,
                    bool strictFlag, bool whitespaceReset);
