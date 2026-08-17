// RFC 2047 encoded-word decoder for MIME Tools.
// Designed for a single linear pass over the selected text and reuses the
// plugin's optimized Base64 implementation for "B" encoded-words.

#include "rfc2047.h"
#include "b64.h"

#include <windows.h>
#include <mlang.h>
#include <oleauto.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")

#include <climits>
#include <cstring>
#include <string>
#include <vector>

namespace
{

inline char asciiLower(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

bool asciiEquals(const char* text, std::size_t length, const char* literal)
{
    std::size_t i = 0;
    for (; i < length && literal[i] != '\0'; ++i)
    {
        if (asciiLower(text[i]) != asciiLower(literal[i]))
            return false;
    }
    return i == length && literal[i] == '\0';
}

bool isLinearWhitespace(const char* text, std::size_t length)
{
    for (std::size_t i = 0; i < length; ++i)
    {
        const char c = text[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            return false;
    }
    return true;
}

int hexValue(unsigned char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

bool decodeQWord(const char* encoded, std::size_t length, std::string& decoded)
{
    decoded.clear();
    decoded.reserve(length);

    for (std::size_t i = 0; i < length; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(encoded[i]);
        if (c == '_')
        {
            decoded.push_back(' ');
        }
        else if (c == '=')
        {
            if (i + 2 >= length)
                return false;
            const int hi = hexValue(static_cast<unsigned char>(encoded[i + 1]));
            const int lo = hexValue(static_cast<unsigned char>(encoded[i + 2]));
            if (hi < 0 || lo < 0)
                return false;
            decoded.push_back(static_cast<char>((hi << 4) | lo));
            i += 2;
        }
        else
        {
            // RFC 2047 encoded-text cannot contain SPACE, TAB, CR, LF or '?'.
            // Rejecting these here keeps malformed words from being rewritten.
            if (c <= 0x20 || c == '?' || c == 0x7f)
                return false;
            decoded.push_back(static_cast<char>(c));
        }
    }

    return true;
}

bool decodeBWord(const char* encoded, std::size_t length, std::string& decoded)
{
    if (length == 0 || length > static_cast<std::size_t>(INT_MAX))
        return false;

    // RFC 2047 B encoded-text cannot contain whitespace or arbitrary bytes.
    // Validate before entering the permissive legacy Base64 lookup table.
    for (std::size_t i = 0; i < length; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(encoded[i]);
        const bool valid = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                           (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=';
        if (!valid)
            return false;
    }

    decoded.assign(length, '\0');
    const int decodedLength = base64Decode(&decoded[0], encoded, length, true, false);
    if (decodedLength < 0)
    {
        decoded.clear();
        return false;
    }

    decoded.resize(static_cast<std::size_t>(decodedLength));
    return true;
}

class CharsetResolver
{
public:
    CharsetResolver() : _multiLanguage(nullptr), _comInitialized(false), _triedCom(false) {}

    ~CharsetResolver()
    {
        if (_multiLanguage != nullptr)
            _multiLanguage->Release();
        if (_comInitialized)
            ::CoUninitialize();
    }

    UINT resolve(const char* charset, std::size_t length)
    {
        // Fast paths for the overwhelmingly common MIME charsets. These avoid
        // COM/MIME-database setup for the normal UTF-8 case.
        if (asciiEquals(charset, length, "utf-8") || asciiEquals(charset, length, "utf8"))
            return CP_UTF8;
        if (asciiEquals(charset, length, "us-ascii") || asciiEquals(charset, length, "ascii"))
            return 20127;
        if (asciiEquals(charset, length, "iso-8859-1") ||
            asciiEquals(charset, length, "latin1") || asciiEquals(charset, length, "latin-1"))
            return 28591;
        if (asciiEquals(charset, length, "windows-1252") || asciiEquals(charset, length, "cp1252"))
            return 1252;

        // Fall back to Windows' MIME charset database for less common aliases
        // and legacy encodings rather than carrying a large hand-maintained map.
        if (!ensureMultiLanguage() || length > static_cast<std::size_t>(UINT_MAX))
            return 0;

        BSTR charsetName = ::SysAllocStringLen(nullptr, static_cast<UINT>(length));
        if (charsetName == nullptr)
            return 0;

        for (std::size_t i = 0; i < length; ++i)
            charsetName[i] = static_cast<unsigned char>(charset[i]);

        MIMECSETINFO info = {};
        const HRESULT hr = _multiLanguage->GetCharsetInfo(charsetName, &info);
        ::SysFreeString(charsetName);
        return SUCCEEDED(hr) ? (info.uiInternetEncoding != 0 ? info.uiInternetEncoding : info.uiCodePage) : 0;
    }

private:
    bool ensureMultiLanguage()
    {
        if (_triedCom)
            return _multiLanguage != nullptr;

        _triedCom = true;
        const HRESULT init = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (init == S_OK || init == S_FALSE)
            _comInitialized = true;
        else if (init != RPC_E_CHANGED_MODE)
            return false;

        const HRESULT hr = ::CoCreateInstance(
            CLSID_CMultiLanguage, nullptr, CLSCTX_INPROC_SERVER,
            IID_IMultiLanguage2, reinterpret_cast<void**>(&_multiLanguage));
        return SUCCEEDED(hr) && _multiLanguage != nullptr;
    }

    IMultiLanguage2* _multiLanguage;
    bool _comInitialized;
    bool _triedCom;
};

bool appendConverted(std::string& output, const std::string& decodedBytes,
                     UINT sourceCodePage, UINT targetCodePage)
{
    if (decodedBytes.empty())
        return true;

    if (targetCodePage == 0)
        targetCodePage = ::GetACP();

    // No transcode needed. This is the hot path for UTF-8 mail in a UTF-8 tab.
    if (sourceCodePage == targetCodePage)
    {
        output.append(decodedBytes);
        return true;
    }

    if (decodedBytes.size() > static_cast<std::size_t>(INT_MAX))
        return false;

    const DWORD inFlags = sourceCodePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
    const int wideLength = ::MultiByteToWideChar(
        sourceCodePage, inFlags, decodedBytes.data(),
        static_cast<int>(decodedBytes.size()), nullptr, 0);
    if (wideLength <= 0)
        return false;

    std::vector<wchar_t> wide(static_cast<std::size_t>(wideLength));
    if (::MultiByteToWideChar(sourceCodePage, inFlags, decodedBytes.data(),
                              static_cast<int>(decodedBytes.size()), &wide[0], wideLength) != wideLength)
        return false;

    if (targetCodePage == CP_UTF8 || targetCodePage == 54936)
    {
        const int outLength = ::WideCharToMultiByte(
            targetCodePage, 0, &wide[0], wideLength, nullptr, 0, nullptr, nullptr);
        if (outLength <= 0)
            return false;

        const std::size_t oldSize = output.size();
        output.resize(oldSize + static_cast<std::size_t>(outLength));
        if (::WideCharToMultiByte(targetCodePage, 0, &wide[0], wideLength,
                                  &output[oldSize], outLength, nullptr, nullptr) != outLength)
        {
            output.resize(oldSize);
            return false;
        }
        return true;
    }

    BOOL usedDefault = FALSE;
    const int outLength = ::WideCharToMultiByte(
        targetCodePage, WC_NO_BEST_FIT_CHARS, &wide[0], wideLength,
        nullptr, 0, nullptr, &usedDefault);
    if (outLength <= 0 || usedDefault)
        return false;

    const std::size_t oldSize = output.size();
    output.resize(oldSize + static_cast<std::size_t>(outLength));
    usedDefault = FALSE;
    if (::WideCharToMultiByte(targetCodePage, WC_NO_BEST_FIT_CHARS, &wide[0], wideLength,
                              &output[oldSize], outLength, nullptr, &usedDefault) != outLength || usedDefault)
    {
        output.resize(oldSize);
        return false;
    }
    return true;
}

std::size_t findEncodedWordStart(const char* input, std::size_t from, std::size_t length)
{
    while (from + 1 < length)
    {
        const void* found = std::memchr(input + from, '=', length - from - 1);
        if (found == nullptr)
            return length;

        const std::size_t pos = static_cast<const char*>(found) - input;
        if (input[pos + 1] == '?')
            return pos;
        from = pos + 1;
    }
    return length;
}

std::size_t findQuestion(const char* input, std::size_t from, std::size_t length)
{
    const void* found = std::memchr(input + from, '?', length - from);
    return found == nullptr ? length : static_cast<const char*>(found) - input;
}

bool tryDecodeWord(const char* input, std::size_t length, std::size_t start,
                   UINT targetCodePage, CharsetResolver& resolver,
                   std::string& converted, std::size_t& wordEnd)
{
    const std::size_t charsetEnd = findQuestion(input, start + 2, length);
    if (charsetEnd == length || charsetEnd == start + 2)
        return false;

    const std::size_t encodingEnd = findQuestion(input, charsetEnd + 1, length);
    if (encodingEnd == length || encodingEnd != charsetEnd + 2)
        return false;

    std::size_t textEnd = encodingEnd + 1;
    while (textEnd + 1 < length && !(input[textEnd] == '?' && input[textEnd + 1] == '='))
        ++textEnd;
    if (textEnd + 1 >= length || textEnd == encodingEnd + 1)
        return false;

    const char encoding = asciiLower(input[charsetEnd + 1]);
    if (encoding != 'b' && encoding != 'q')
        return false;

    const UINT sourceCodePage = resolver.resolve(input + start + 2, charsetEnd - (start + 2));
    if (sourceCodePage == 0)
        return false;

    const char* encodedText = input + encodingEnd + 1;
    const std::size_t encodedLength = textEnd - (encodingEnd + 1);

    std::string decodedBytes;
    const bool decoded = encoding == 'b'
        ? decodeBWord(encodedText, encodedLength, decodedBytes)
        : decodeQWord(encodedText, encodedLength, decodedBytes);
    if (!decoded)
        return false;

    converted.clear();
    converted.reserve(decodedBytes.size());
    if (!appendConverted(converted, decodedBytes, sourceCodePage, targetCodePage))
        return false;

    wordEnd = textEnd + 2;
    return true;
}

} // namespace

Rfc2047DecodeResult decodeRfc2047Header(const char* input, std::size_t inputLength,
                                        unsigned int targetCodePage, std::string& output)
{
    Rfc2047DecodeResult result = { 0, 0 };
    output.clear();
    output.reserve(inputLength);

    CharsetResolver resolver;
    std::size_t cursor = 0;
    bool previousWasEncodedWord = false;

    while (cursor < inputLength)
    {
        const std::size_t wordStart = findEncodedWordStart(input, cursor, inputLength);
        if (wordStart == inputLength)
            break;

        std::string converted;
        std::size_t wordEnd = wordStart;
        if (tryDecodeWord(input, inputLength, wordStart, targetCodePage, resolver, converted, wordEnd))
        {
            const std::size_t betweenLength = wordStart - cursor;
            if (!(previousWasEncodedWord && isLinearWhitespace(input + cursor, betweenLength)))
                output.append(input + cursor, betweenLength);

            output.append(converted);
            cursor = wordEnd;
            previousWasEncodedWord = true;
            ++result.decodedWords;
            continue;
        }

        // Preserve a malformed/unsupported candidate exactly and advance past
        // the marker so the scan remains linear even on hostile input.
        output.append(input + cursor, wordStart + 2 - cursor);
        cursor = wordStart + 2;
        previousWasEncodedWord = false;
        ++result.skippedWords;
    }

    output.append(input + cursor, inputLength - cursor);
    return result;
}
