// This file is part of Notepad++ plugin MIME Tools project
// Copyright (C)2023 Don HO <don.h@free.fr>

#pragma once

#include <cstddef>
#include <cstdint>

constexpr auto QP_ENCODED_LINE_LEN_MAX = 76;

class QuotedPrintable {
public:
    QuotedPrintable() : _buffer(nullptr) {}
    ~QuotedPrintable() { delete [] _buffer; }

    char* encode(const char* str);
    char* encode(const char* str, std::size_t len);
    char* decode(const char* str);
    char* decode(const char* str, std::size_t len);
    std::size_t length() const { return _i; }

private:
    char* _buffer = nullptr;
    std::size_t _bufLen = 0;
    std::size_t _i = 0;
    int _nbCharInLine = 0;
    int _nbChar = 0;
    char _chars[4] = {};

    void putQPChar();
    void getQPChar(char c);

    int32_t charToDigit(char c) const {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        return -1;
    }

    bool makeChar(char hiChar, char loChar, unsigned char& value) const {
        const auto hi = charToDigit(hiChar);
        const auto lo = charToDigit(loChar);
        if (hi < 0 || lo < 0) return false;
        value = static_cast<unsigned char>((hi << 4) | lo);
        return true;
    }

    void initVar() {
        delete [] _buffer;
        _buffer = nullptr;
        _bufLen = 0;
        _i = 0;
        _nbChar = 0;
        _nbCharInLine = 0;
    }

    static char toChar(int i) {
        return i < 10 ? static_cast<char>('0' + i) : static_cast<char>('A' + i - 10);
    }
};
