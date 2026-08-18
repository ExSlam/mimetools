// This file is part of Notepad++ plugin MIME Tools project
// Copyright (C)2026 ExSlam contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.

#include "tdeflate.h"

#include <algorithm>
#include <cstdint>
#include <new>
#include <vector>

namespace {

constexpr std::size_t kWindowSize = 32768;
constexpr std::size_t kMaxMatch = 258;
constexpr std::size_t kMinMatch = 3;
constexpr std::size_t kHashBits = 15;
constexpr std::size_t kHashSize = static_cast<std::size_t>(1) << kHashBits;
constexpr int kMaxChain = 64;

constexpr unsigned short kLengthBase[] = {
    3, 4, 5, 6, 7, 8, 9, 10,
    11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115,
    131, 163, 195, 227, 258
};

constexpr unsigned char kLengthExtra[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4,
    5, 5, 5, 5, 0
};

constexpr unsigned short kDistanceBase[] = {
    1, 2, 3, 4, 5, 7, 9, 13,
    17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073,
    4097, 6145, 8193, 12289, 16385, 24577
};

constexpr unsigned char kDistanceExtra[] = {
    0, 0, 0, 0, 1, 1, 2, 2,
    3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10,
    11, 11, 12, 12, 13, 13
};

unsigned reverseBits(unsigned value, unsigned bitCount)
{
    unsigned reversed = 0;
    for (unsigned i = 0; i < bitCount; ++i)
    {
        reversed = (reversed << 1) | (value & 1U);
        value >>= 1;
    }
    return reversed;
}

class BitWriter
{
public:
    void reserve(std::size_t bytes)
    {
        _bytes.reserve(bytes);
    }

    void writeBits(unsigned value, unsigned bitCount)
    {
        _bitBuffer |= static_cast<std::uint64_t>(value) << _bitCount;
        _bitCount += bitCount;

        while (_bitCount >= 8)
        {
            _bytes.push_back(static_cast<unsigned char>(_bitBuffer & 0xffU));
            _bitBuffer >>= 8;
            _bitCount -= 8;
        }
    }

    void finish()
    {
        if (_bitCount != 0)
        {
            _bytes.push_back(static_cast<unsigned char>(_bitBuffer & 0xffU));
            _bitBuffer = 0;
            _bitCount = 0;
        }
    }

    std::vector<unsigned char>& bytes()
    {
        return _bytes;
    }

private:
    std::vector<unsigned char> _bytes;
    std::uint64_t _bitBuffer = 0;
    unsigned _bitCount = 0;
};

void writeFixedSymbol(BitWriter& writer, unsigned symbol)
{
    unsigned code = 0;
    unsigned bitCount = 0;

    if (symbol <= 143)
    {
        code = 0x30U + symbol;
        bitCount = 8;
    }
    else if (symbol <= 255)
    {
        code = 0x190U + (symbol - 144U);
        bitCount = 9;
    }
    else if (symbol <= 279)
    {
        code = symbol - 256U;
        bitCount = 7;
    }
    else
    {
        code = 0xC0U + (symbol - 280U);
        bitCount = 8;
    }

    writer.writeBits(reverseBits(code, bitCount), bitCount);
}

void writeLength(BitWriter& writer, std::size_t length)
{
    for (unsigned i = 0; i < 29; ++i)
    {
        const unsigned extraBits = kLengthExtra[i];
        const std::size_t maxLength = static_cast<std::size_t>(kLengthBase[i]) +
            (extraBits == 0 ? 0 : ((static_cast<std::size_t>(1) << extraBits) - 1));

        if (length <= maxLength)
        {
            writeFixedSymbol(writer, 257U + i);
            if (extraBits != 0)
            {
                const unsigned extraValue = static_cast<unsigned>(
                    length - static_cast<std::size_t>(kLengthBase[i]));
                writer.writeBits(extraValue, extraBits);
            }
            return;
        }
    }
}

void writeDistance(BitWriter& writer, std::size_t distance)
{
    for (unsigned i = 0; i < 30; ++i)
    {
        const unsigned extraBits = kDistanceExtra[i];
        const std::size_t maxDistance = static_cast<std::size_t>(kDistanceBase[i]) +
            (extraBits == 0 ? 0 : ((static_cast<std::size_t>(1) << extraBits) - 1));

        if (distance <= maxDistance)
        {
            // Fixed distance codes are 5-bit canonical codes. Huffman codes are
            // transmitted MSB-first, so reverse before feeding our LSB bit writer.
            writer.writeBits(reverseBits(i, 5), 5);
            if (extraBits != 0)
            {
                const unsigned extraValue = static_cast<unsigned>(
                    distance - static_cast<std::size_t>(kDistanceBase[i]));
                writer.writeBits(extraValue, extraBits);
            }
            return;
        }
    }
}

std::size_t hash3(const unsigned char* input, std::size_t position)
{
    const std::uint32_t value =
        (static_cast<std::uint32_t>(input[position]) << 16) |
        (static_cast<std::uint32_t>(input[position + 1]) << 8) |
        static_cast<std::uint32_t>(input[position + 2]);
    return static_cast<std::size_t>((value * 2654435761U) >> (32U - kHashBits));
}

void insertPosition(const unsigned char* input,
                    std::size_t inputLength,
                    std::size_t position,
                    std::vector<int>& head,
                    std::vector<int>& previous)
{
    if (position + kMinMatch > inputLength)
        return;

    const std::size_t hash = hash3(input, position);
    previous[position] = head[hash];
    head[hash] = static_cast<int>(position);
}

void findMatch(const unsigned char* input,
               std::size_t inputLength,
               std::size_t position,
               const std::vector<int>& head,
               const std::vector<int>& previous,
               std::size_t& bestLength,
               std::size_t& bestDistance)
{
    bestLength = 0;
    bestDistance = 0;

    if (position + kMinMatch > inputLength)
        return;

    const std::size_t maxLength = std::min(kMaxMatch, inputLength - position);
    int candidate = head[hash3(input, position)];
    int chain = 0;

    while (candidate >= 0 && chain < kMaxChain)
    {
        const std::size_t candidatePosition = static_cast<std::size_t>(candidate);
        const std::size_t distance = position - candidatePosition;
        if (distance == 0 || distance > kWindowSize)
            break;

        if (bestLength == 0 ||
            (bestLength < maxLength &&
             input[candidatePosition + bestLength] == input[position + bestLength]))
        {
            std::size_t length = 0;
            while (length < maxLength &&
                   input[candidatePosition + length] == input[position + length])
            {
                ++length;
            }

            if (length >= kMinMatch && length > bestLength)
            {
                bestLength = length;
                bestDistance = distance;
                if (length == maxLength)
                    break;
            }
        }

        candidate = previous[candidatePosition];
        ++chain;
    }
}

std::vector<unsigned char> compressFixed(const unsigned char* input, std::size_t inputLength)
{
    BitWriter writer;
    writer.reserve(inputLength + inputLength / 16 + 32);

    // BFINAL=1, BTYPE=01 (fixed Huffman codes).
    writer.writeBits(1, 1);
    writer.writeBits(1, 2);

    std::vector<int> head(kHashSize, -1);
    std::vector<int> previous(inputLength, -1);

    std::size_t position = 0;
    while (position < inputLength)
    {
        std::size_t matchLength = 0;
        std::size_t matchDistance = 0;
        findMatch(input, inputLength, position, head, previous, matchLength, matchDistance);

        // Three-byte matches with very large distances can cost more than literals.
        // Requiring four bytes in that case gives better worst-case output while
        // preserving useful short nearby matches.
        const bool useMatch = matchLength >= kMinMatch &&
            !(matchLength == 3 && matchDistance > 1024);

        if (useMatch)
        {
            writeLength(writer, matchLength);
            writeDistance(writer, matchDistance);

            for (std::size_t i = 0; i < matchLength; ++i)
                insertPosition(input, inputLength, position + i, head, previous);

            position += matchLength;
        }
        else
        {
            writeFixedSymbol(writer, input[position]);
            insertPosition(input, inputLength, position, head, previous);
            ++position;
        }
    }

    writeFixedSymbol(writer, 256); // End-of-block.
    writer.finish();
    return std::move(writer.bytes());
}

std::vector<unsigned char> compressStored(const unsigned char* input, std::size_t inputLength)
{
    std::vector<unsigned char> output;
    const std::size_t blockCount = inputLength == 0 ? 1 : (inputLength + 65534) / 65535;
    output.reserve(inputLength + blockCount * 5);

    std::size_t position = 0;
    do
    {
        const std::size_t remaining = inputLength - position;
        const unsigned short length = static_cast<unsigned short>(
            std::min<std::size_t>(remaining, 65535));
        const bool finalBlock = position + length == inputLength;

        // Stored blocks start on a byte boundary. The low three bits are
        // BFINAL followed by BTYPE=00; the remaining five padding bits are zero.
        output.push_back(static_cast<unsigned char>(finalBlock ? 0x01 : 0x00));
        output.push_back(static_cast<unsigned char>(length & 0xffU));
        output.push_back(static_cast<unsigned char>((length >> 8) & 0xffU));

        const unsigned short inverseLength = static_cast<unsigned short>(~length);
        output.push_back(static_cast<unsigned char>(inverseLength & 0xffU));
        output.push_back(static_cast<unsigned char>((inverseLength >> 8) & 0xffU));

        output.insert(output.end(), input + position, input + position + length);
        position += length;
    }
    while (position < inputLength);

    return output;
}

} // namespace

bool tdefl_compress_raw(std::vector<unsigned char>& output,
                        const unsigned char* input,
                        std::size_t inputLength)
{
    if (input == nullptr && inputLength != 0)
        return false;

    try
    {
        const unsigned char emptyInput = 0;
        const unsigned char* source = inputLength == 0 ? &emptyInput : input;
        std::vector<unsigned char> fixed = compressFixed(source, inputLength);
        std::vector<unsigned char> stored = compressStored(source, inputLength);

        output = fixed.size() <= stored.size() ? std::move(fixed) : std::move(stored);
        return true;
    }
    catch (const std::bad_alloc&)
    {
        output.clear();
        return false;
    }
}
