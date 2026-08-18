// This file is part of Notepad++ plugin MIME Tools project
// Copyright (C)2023 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Enhance Base64 features, and rewrite Base64 encode/decode implementation
// Copyright 2019 by Paul Nankervis <paulnank@hotmail.com>

#include <vector>
#include <climits>

#include "PluginInterface.h"
#include "mimeTools.h"
#include "b64.h"
#include "qp.h"
#include "url.h"
#include "saml.h"
#include "rfc2047.h"

const TCHAR PLUGIN_NAME[] = TEXT("MIME Tools");
const int nbFunc = 30;

HINSTANCE g_hInst = nullptr;
NppData nppData;
FuncItem funcItem[nbFunc];
HWND g_hAboutDlg = nullptr;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/)
{
	switch (reasonForCall)
	{
		case DLL_PROCESS_ATTACH:
		{
			g_hInst = (HINSTANCE)hModule;

			funcItem[0]._pFunc = convertToBase64FromAscii;
			funcItem[1]._pFunc = convertToBase64FromAscii_pad;
			funcItem[2]._pFunc = convertToBase64FromAscii_pad_byline;
			funcItem[3]._pFunc = convertToBase64FromAscii_B64Format;
			funcItem[4]._pFunc = convertToBase64FromAscii_byline;
			funcItem[5]._pFunc = convertToAsciiFromBase64;
			funcItem[6]._pFunc = convertToAsciiFromBase64_strict;
			funcItem[7]._pFunc = convertToAsciiFromBase64_whitespaceReset;
			funcItem[8]._pFunc = convertToBase64FromUtf16Le;
			funcItem[9]._pFunc = convertToUtf16LeFromBase64;
			funcItem[10]._pFunc = NULL;
			funcItem[11]._pFunc = convertToQuotedPrintable;
			funcItem[12]._pFunc = convertToAsciiFromQuotedPrintable;
			funcItem[13]._pFunc = NULL;
			funcItem[14]._pFunc = convertMimeHeaderDecode;
			funcItem[15]._pFunc = NULL;
			funcItem[16]._pFunc = convertURLMinEncode;
			funcItem[17]._pFunc = convertURLMinEncodeByLine;
			funcItem[18]._pFunc = convertURLEncodeExtended;
			funcItem[19]._pFunc = convertURLEncodeExtendedByLine;
			funcItem[20]._pFunc = convertURLFullEncode;
			funcItem[21]._pFunc = convertURLFullEncodeByLine;
			funcItem[22]._pFunc = convertURLDecode;
			funcItem[23]._pFunc = NULL;
			funcItem[24]._pFunc = urlconvertToBase64FromAscii;
			funcItem[25]._pFunc = urlconvertToAsciiFromBase64;
			funcItem[26]._pFunc = NULL;
			funcItem[27]._pFunc = convertSamlDecode;
			funcItem[28]._pFunc = NULL;
			funcItem[29]._pFunc = about;
			lstrcpy(funcItem[0]._itemName, TEXT("Base64 Encode"));
			lstrcpy(funcItem[1]._itemName, TEXT("Base64 Encode with padding"));
			lstrcpy(funcItem[2]._itemName, TEXT("Base64 Encode with padding by line"));
			lstrcpy(funcItem[3]._itemName, TEXT("Base64 Encode with Unix EOL"));
			lstrcpy(funcItem[4]._itemName, TEXT("Base64 Encode by line"));
			lstrcpy(funcItem[5]._itemName, TEXT("Base64 Decode"));
			lstrcpy(funcItem[6]._itemName, TEXT("Base64 Decode strict"));
			lstrcpy(funcItem[7]._itemName, TEXT("Base64 Decode by line"));
			lstrcpy(funcItem[8]._itemName, TEXT("Base64 Encode UTF-16LE (PowerShell)"));
			lstrcpy(funcItem[9]._itemName, TEXT("Base64 Decode UTF-16LE (PowerShell)"));
			lstrcpy(funcItem[10]._itemName, TEXT("-SEPARATOR-"));
			lstrcpy(funcItem[11]._itemName, TEXT("Quoted-printable Encode"));
			lstrcpy(funcItem[12]._itemName, TEXT("Quoted-printable Decode"));
			lstrcpy(funcItem[13]._itemName, TEXT("-SEPARATOR-"));
			lstrcpy(funcItem[14]._itemName, TEXT("MIME Header Decode (RFC 2047)"));
			lstrcpy(funcItem[15]._itemName, TEXT("-SEPARATOR-"));
			lstrcpy(funcItem[16]._itemName, TEXT("URL Encode (RFC1738)"));
			lstrcpy(funcItem[17]._itemName, TEXT("URL Encode (RFC1738) by line"));
			lstrcpy(funcItem[18]._itemName, TEXT("URL Encode (Extended)"));
			lstrcpy(funcItem[19]._itemName, TEXT("URL Encode (Extended) by line"));
			lstrcpy(funcItem[20]._itemName, TEXT("URL Encode (Full)"));
			lstrcpy(funcItem[21]._itemName, TEXT("URL Encode (Full) by line"));
			lstrcpy(funcItem[22]._itemName, TEXT("URL Decode"));
			lstrcpy(funcItem[23]._itemName, TEXT("-SEPARATOR-"));
			lstrcpy(funcItem[24]._itemName, TEXT("URL Base64 Encode"));
			lstrcpy(funcItem[25]._itemName, TEXT("URL Base64 Decode"));
			lstrcpy(funcItem[26]._itemName, TEXT("-SEPARATOR-"));
			lstrcpy(funcItem[27]._itemName, TEXT("SAML Decode"));
			lstrcpy(funcItem[28]._itemName, TEXT("-SEPARATOR-"));
			lstrcpy(funcItem[29]._itemName, TEXT("About"));
			for (int i = 0; i < nbFunc; i++)
			{
				funcItem[i]._init2Check = false;
				funcItem[i]._pShKey = NULL;
			}
		}
		break;

		case DLL_PROCESS_DETACH:
		break;

		case DLL_THREAD_ATTACH:
		break;

		case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode)
{
	switch (notifyCode->nmhdr.code)
	{
		case NPPN_DARKMODECHANGED:
		{
			if (g_hAboutDlg)
			{
				::SendMessage(nppData._nppHandle, NPPM_DARKMODESUBCLASSANDTHEME, static_cast<WPARAM>(NppDarkMode::dmfHandleChange), reinterpret_cast<LPARAM>(g_hAboutDlg));
				::SetWindowPos(g_hAboutDlg, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // to redraw titlebar
			}
			break;
		}
	}
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif //UNICODE

// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// https://github.com/notepad-plus-plus/notepad-plus-plus/issues
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM)
{
	return TRUE;
}

HWND getCurrentScintillaHandle()
{
    int currentEdit;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
	return (currentEdit == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
};

namespace
{
	void showUtf16Base64Error(const TCHAR* message)
	{
		::MessageBox(nppData._nppHandle, message, TEXT("Base64 UTF-16LE"), MB_OK | MB_ICONERROR);
	}

	UINT getDocumentCodePage(HWND hScintilla)
	{
		const UINT codePage = static_cast<UINT>(
			::SendMessage(hScintilla, SCI_GETCODEPAGE, 0, 0));
		return codePage == 0 ? CP_ACP : codePage;
	}

	bool getSingleSelection(HWND hScintilla, size_t& start, size_t& end, std::vector<char>& selectedText)
	{
		const size_t nbSelections = static_cast<size_t>(
			::SendMessage(hScintilla, SCI_GETSELECTIONS, 0, 0));
		if (nbSelections > 1)
			return false;

		start = static_cast<size_t>(::SendMessage(hScintilla, SCI_GETSELECTIONSTART, 0, 0));
		end = static_cast<size_t>(::SendMessage(hScintilla, SCI_GETSELECTIONEND, 0, 0));
		if (end < start)
		{
			const size_t tmp = start;
			start = end;
			end = tmp;
		}

		const size_t selectedLength = end - start;
		if (selectedLength == 0)
			return false;

		selectedText.resize(selectedLength + 1);
		::SendMessage(hScintilla, SCI_SETTARGETSTART, start, 0);
		::SendMessage(hScintilla, SCI_SETTARGETEND, end, 0);
		::SendMessage(hScintilla, SCI_GETTARGETTEXT, 0,
			reinterpret_cast<LPARAM>(selectedText.data()));
		return true;
	}

	bool documentBytesToUtf16(HWND hScintilla, const char* text, size_t length,
		std::vector<wchar_t>& utf16)
	{
		if (length > static_cast<size_t>(INT_MAX))
			return false;
		if (length == 0)
		{
			utf16.clear();
			return true;
		}

		const UINT codePage = getDocumentCodePage(hScintilla);
		const DWORD flags = codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
		const int inputLength = static_cast<int>(length);
		const int utf16Length = ::MultiByteToWideChar(
			codePage, flags, text, inputLength, nullptr, 0);
		if (utf16Length <= 0)
			return false;

		utf16.resize(static_cast<size_t>(utf16Length));
		return ::MultiByteToWideChar(
			codePage, flags, text, inputLength, utf16.data(), utf16Length) == utf16Length;
	}

	bool isValidUtf16(const wchar_t* text, size_t length)
	{
		for (size_t i = 0; i < length; ++i)
		{
			const unsigned int unit = static_cast<unsigned int>(text[i]);
			if (unit >= 0xD800 && unit <= 0xDBFF)
			{
				if (i + 1 >= length)
					return false;
				const unsigned int next = static_cast<unsigned int>(text[++i]);
				if (next < 0xDC00 || next > 0xDFFF)
					return false;
			}
			else if (unit >= 0xDC00 && unit <= 0xDFFF)
			{
				return false;
			}
		}
		return true;
	}

	bool utf16ToDocumentBytes(HWND hScintilla, const wchar_t* text, size_t length,
		std::vector<char>& output)
	{
		if (length > static_cast<size_t>(INT_MAX))
			return false;
		if (length == 0)
		{
			output.clear();
			return true;
		}

		const UINT codePage = getDocumentCodePage(hScintilla);
		DWORD flags = 0;
		BOOL usedDefaultChar = FALSE;
		LPBOOL usedDefaultCharPtr = nullptr;

		if (codePage == CP_UTF8)
		{
			flags = WC_ERR_INVALID_CHARS;
		}
		else if (codePage != CP_UTF7 && codePage != 54936)
		{
			flags = WC_NO_BEST_FIT_CHARS;
			usedDefaultCharPtr = &usedDefaultChar;
		}

		const int inputLength = static_cast<int>(length);
		const int outputLength = ::WideCharToMultiByte(
			codePage, flags, text, inputLength, nullptr, 0, nullptr, usedDefaultCharPtr);
		if (outputLength <= 0 || usedDefaultChar)
			return false;

		output.resize(static_cast<size_t>(outputLength));
		usedDefaultChar = FALSE;
		const int convertedLength = ::WideCharToMultiByte(
			codePage, flags, text, inputLength, output.data(), outputLength,
			nullptr, usedDefaultCharPtr);
		return convertedLength == outputLength && !usedDefaultChar;
	}

	void replaceSelection(HWND hScintilla, size_t start, size_t end,
		const char* text, size_t length)
	{
		::SendMessage(hScintilla, SCI_SETTARGETSTART, start, 0);
		::SendMessage(hScintilla, SCI_SETTARGETEND, end, 0);
		::SendMessage(hScintilla, SCI_REPLACETARGET, static_cast<WPARAM>(length),
			reinterpret_cast<LPARAM>(text));
		::SendMessage(hScintilla, SCI_SETSEL, start, start + length);
	}
}

void convertToBase64FromUtf16Le()
{
	static_assert(sizeof(wchar_t) == 2, "UTF-16LE Base64 requires 16-bit wchar_t on Windows.");

	HWND hCurrScintilla = getCurrentScintillaHandle();
	size_t start = 0;
	size_t end = 0;
	std::vector<char> selectedText;
	if (!getSingleSelection(hCurrScintilla, start, end, selectedText))
		return;

	const size_t selectedLength = end - start;
	std::vector<wchar_t> utf16;
	if (!documentBytesToUtf16(hCurrScintilla, selectedText.data(), selectedLength, utf16))
	{
		showUtf16Base64Error(TEXT("The selected text cannot be converted to UTF-16 without decoding errors."));
		return;
	}

	if (utf16.size() > static_cast<size_t>(INT_MAX) / sizeof(wchar_t))
	{
		showUtf16Base64Error(TEXT("The selected text is too large to Base64 encode as UTF-16LE."));
		return;
	}
	const size_t utf16ByteLength = utf16.size() * sizeof(wchar_t);
	const size_t fullGroups = utf16ByteLength / 3;
	const size_t tailLength = (utf16ByteLength % 3) == 0 ? 0 : 4;
	if (fullGroups > (static_cast<size_t>(INT_MAX) - tailLength) / 4)
	{
		showUtf16Base64Error(TEXT("The selected text is too large to Base64 encode as UTF-16LE."));
		return;
	}
	const size_t encodedLength = fullGroups * 4 + tailLength;
	std::vector<char> encodedText(encodedLength);

	const int len = base64Encode(
		encodedText.data(),
		reinterpret_cast<const char*>(utf16.data()),
		utf16ByteLength,
		0,
		true,
		false);
	if (len < 0 || static_cast<size_t>(len) != encodedLength)
	{
		showUtf16Base64Error(TEXT("Base64 UTF-16LE encoding failed."));
		return;
	}

	replaceSelection(hCurrScintilla, start, end, encodedText.data(), encodedLength);
}

void convertToUtf16LeFromBase64()
{
	static_assert(sizeof(wchar_t) == 2, "UTF-16LE Base64 requires 16-bit wchar_t on Windows.");

	HWND hCurrScintilla = getCurrentScintillaHandle();
	size_t start = 0;
	size_t end = 0;
	std::vector<char> selectedText;
	if (!getSingleSelection(hCurrScintilla, start, end, selectedText))
		return;

	const size_t selectedLength = end - start;
	if (selectedLength > static_cast<size_t>(INT_MAX))
	{
		showUtf16Base64Error(TEXT("The selected Base64 text is too large to decode."));
		return;
	}

	// Base64 decoding never produces more bytes than its input. Allocate aligned
	// UTF-16 storage directly so no decoded-byte -> wchar_t copy is required.
	std::vector<wchar_t> utf16((selectedLength + sizeof(wchar_t) - 1) / sizeof(wchar_t));
	const int decodedByteLength = base64Decode(
		reinterpret_cast<char*>(utf16.data()),
		selectedText.data(),
		selectedLength,
		true,
		false);
	if (decodedByteLength < 0)
	{
		showUtf16Base64Error(TEXT("The selection is not valid Base64."));
		return;
	}
	if ((decodedByteLength % static_cast<int>(sizeof(wchar_t))) != 0)
	{
		showUtf16Base64Error(TEXT("Decoded Base64 has an odd byte count and is not valid UTF-16LE."));
		return;
	}

	size_t utf16Length = static_cast<size_t>(decodedByteLength) / sizeof(wchar_t);
	size_t offset = 0;
	if (utf16Length > 0)
	{
		if (utf16[0] == static_cast<wchar_t>(0xFEFF))
		{
			offset = 1; // Optional UTF-16LE BOM.
		}
		else if (utf16[0] == static_cast<wchar_t>(0xFFFE))
		{
			showUtf16Base64Error(TEXT("Decoded data has a UTF-16BE BOM. This command expects UTF-16LE."));
			return;
		}
	}

	const wchar_t* utf16Text = utf16.data() + offset;
	utf16Length -= offset;
	if (!isValidUtf16(utf16Text, utf16Length))
	{
		showUtf16Base64Error(TEXT("Decoded Base64 contains malformed UTF-16 surrogate pairs."));
		return;
	}

	std::vector<char> documentText;
	if (!utf16ToDocumentBytes(hCurrScintilla, utf16Text, utf16Length, documentText))
	{
		showUtf16Base64Error(TEXT("Decoded UTF-16LE text cannot be represented in the current document encoding."));
		return;
	}

	replaceSelection(hCurrScintilla, start, end,
		documentText.empty() ? "" : documentText.data(), documentText.size());
}

void convertAsciiToBase64(size_t wrapLength, bool padFlag, bool byLineFlag)
{
	HWND hCurrScintilla = getCurrentScintillaHandle();

	size_t nbSelections =
		::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);

	if (nbSelections > 1)
		return;

	size_t selectedLength =
		::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);

	if (selectedLength == 0)
		return;

	std::vector<char> selectedText(selectedLength + 1);

	::SendMessage(hCurrScintilla, SCI_TARGETFROMSELECTION, 0, 0);
	::SendMessage(
		hCurrScintilla,
		SCI_GETTARGETTEXT,
		0,
		reinterpret_cast<LPARAM>(selectedText.data())
	);

	/*
	 * Padded-per-line already uses the std::string implementation.
	 * Do not allocate/copy another temporary output buffer.
	 */
	if (padFlag && byLineFlag)
	{
		std::string encodedString;

		int len = base64EncodeWithPaddingByLine(
			encodedString,
			selectedText.data(),
			selectedLength
		);

		if (len < 0)
		{
			::MessageBox(
				nppData._nppHandle,
				TEXT("Input is too large to encode."),
				TEXT("Base64"),
				MB_OK | MB_ICONERROR
			);
			return;
		}

		::SendMessage(hCurrScintilla, SCI_TARGETFROMSELECTION, 0, 0);
		::SendMessage(
			hCurrScintilla,
			SCI_REPLACETARGET,
			len,
			reinterpret_cast<LPARAM>(encodedString.data())
		);

		return;
	}

	size_t bufferLength;

	if (byLineFlag)
	{
		/*
		 * Unpadded Base64 encoding of a one-byte line produces
		 * two output bytes. CR/LF bytes are copied unchanged.
		 *
		 * Therefore 2 * input length is a safe upper bound.
		 */
		bufferLength = selectedLength * 2 + 1;
	}
	else
	{
		/*
		 * Standard padded Base64 upper bound.
		 */
		const size_t baseLength =
			((selectedLength + 2) / 3) * 4;

		size_t lineBreaks = 0;

		if (wrapLength > 0 && baseLength > 0)
			lineBreaks = (baseLength - 1) / wrapLength;

		bufferLength =
			baseLength +
			lineBreaks +
			1;
	}

	std::vector<char> encodedText(bufferLength);

	int len = base64Encode(
		encodedText.data(),
		selectedText.data(),
		selectedLength,
		wrapLength,
		padFlag,
		byLineFlag
	);

	if (len < 0 ||
		static_cast<size_t>(len) > encodedText.size())
	{
		::MessageBox(
			nppData._nppHandle,
			TEXT("Base64 output exceeded the allocated buffer."),
			TEXT("Base64"),
			MB_OK | MB_ICONERROR
		);
		return;
	}

	::SendMessage(hCurrScintilla, SCI_TARGETFROMSELECTION, 0, 0);
	::SendMessage(
		hCurrScintilla,
		SCI_REPLACETARGET,
		len,
		reinterpret_cast<LPARAM>(encodedText.data())
	);
}

void urlconvertAsciiToBase64(size_t /*wrapLength*/, bool /*padFlag*/, bool /*byLineFlag*/)
{
	HWND hCurrScintilla = getCurrentScintillaHandle();

	const size_t nbSelections =
		::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);

	if (nbSelections > 1)
		return;

	const size_t selectedLength =
		::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);

	if (selectedLength == 0)
		return;

	// SCI_GETSELTEXT appends a terminating NUL, so allocate one extra byte.
	std::vector<char> selectedText(selectedLength + 1);

	::SendMessage(
		hCurrScintilla,
		SCI_GETSELTEXT,
		0,
		reinterpret_cast<LPARAM>(selectedText.data())
	);

	/*
	 * Unpadded Base64URL output length:
	 *
	 * 3 input bytes -> 4 output bytes
	 * 1 remaining byte -> 2 output bytes
	 * 2 remaining bytes -> 3 output bytes
	 */
	const size_t fullGroups = selectedLength / 3;
	const size_t remainder = selectedLength % 3;
	const size_t tailLength =
		remainder == 0 ? 0 : remainder + 1;

	/*
	 * base64UrlEncode() returns int, so reject an input whose encoded
	 * result cannot be represented by that API.
	 *
	 * This check is performed before multiplying by 4, so the size_t
	 * calculation cannot overflow.
	 */
	if (fullGroups >
		(static_cast<size_t>(INT_MAX) - tailLength) / 4)
	{
		::MessageBox(
			nppData._nppHandle,
			TEXT("The selected text is too large to Base64URL encode."),
			TEXT("Base64URL"),
			MB_OK | MB_ICONERROR
		);

		return;
	}

	const size_t encodedLength =
		fullGroups * 4 + tailLength;

	std::vector<char> encodedText(encodedLength);

	const int len = base64UrlEncode(
		encodedText.data(),
		selectedText.data(),
		selectedLength
	);

	if (len < 0 ||
		static_cast<size_t>(len) != encodedLength)
	{
		::MessageBox(
			nppData._nppHandle,
			TEXT("Base64URL encoding failed."),
			TEXT("Base64URL"),
			MB_OK | MB_ICONERROR
		);

		return;
	}

	::SendMessage(
		hCurrScintilla,
		SCI_TARGETFROMSELECTION,
		0,
		0
	);

	::SendMessage(
		hCurrScintilla,
		SCI_REPLACETARGET,
		static_cast<WPARAM>(len),
		reinterpret_cast<LPARAM>(encodedText.data())
	);
}

void convertToBase64FromAscii()
{
	convertAsciiToBase64(0, false, false);
}

void urlconvertToBase64FromAscii()
{
	urlconvertAsciiToBase64(0, false, false);
}

void convertToBase64FromAscii_pad()
{
	convertAsciiToBase64(0, true, false);
}

void convertToBase64FromAscii_pad_byline()
{
	convertAsciiToBase64(0, true, true);
}

void convertToBase64FromAscii_B64Format()
{
	convertAsciiToBase64(64, true, false);
}

void convertToBase64FromAscii_byline()
{
	convertAsciiToBase64(0, false, true);
}


void convertBase64ToAscii(bool strictFlag, bool whitespaceReset)
{
	HWND hCurrScintilla = getCurrentScintillaHandle();

	size_t nbSelections = ::SendMessage(
		hCurrScintilla,
		SCI_GETSELECTIONS,
		0,
		0
	);

	if (nbSelections > 1)
		return;

	size_t selectedLength = ::SendMessage(
		hCurrScintilla,
		SCI_GETSELTEXT,
		0,
		0
	);

	if (selectedLength == 0)
		return;

	std::vector<char> selectedText(selectedLength + 1);
	std::vector<char> decodedText(selectedLength + 1);

	::SendMessage(
		hCurrScintilla,
		SCI_TARGETFROMSELECTION,
		0,
		0
	);

	::SendMessage(
		hCurrScintilla,
		SCI_GETTARGETTEXT,
		0,
		reinterpret_cast<LPARAM>(selectedText.data())
	);

	int len = base64Decode(
		decodedText.data(),
		selectedText.data(),
		selectedLength,
		strictFlag,
		whitespaceReset
	);

	if (len < 0)
	{
		::MessageBox(
			nppData._nppHandle,
			TEXT("Problem!"),
			TEXT("Base64"),
			MB_OK | MB_ICONERROR
		);

		return;
	}

	::SendMessage(
		hCurrScintilla,
		SCI_TARGETFROMSELECTION,
		0,
		0
	);

	::SendMessage(
		hCurrScintilla,
		SCI_REPLACETARGET,
		len,
		reinterpret_cast<LPARAM>(decodedText.data())
	);
}


void urlconvertBase64ToAscii(bool strictFlag, bool whitespaceReset)
{
	HWND hCurrScintilla = getCurrentScintillaHandle();
	size_t nbSelections = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);
	if (nbSelections > 1) return;

	size_t selectedLength = ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
	if (selectedLength == 0) return;

	std::vector<char> selectedText(selectedLength + 1);
	std::vector<char> decodedText(selectedLength + 1);

	::SendMessage(hCurrScintilla, SCI_TARGETFROMSELECTION, 0, 0);
	::SendMessage(
		hCurrScintilla,
		SCI_GETTARGETTEXT,
		0,
		reinterpret_cast<LPARAM>(selectedText.data())
	);

	int len = base64UrlDecode(
		decodedText.data(),
		selectedText.data(),
		selectedLength,
		strictFlag,
		whitespaceReset
	);

	if (len < 0)
	{
		::MessageBox(
			nppData._nppHandle,
			TEXT("Problem!"),
			TEXT("Base64URL"),
			MB_OK
		);
		return;
	}

	::SendMessage(hCurrScintilla, SCI_TARGETFROMSELECTION, 0, 0);
	::SendMessage(
		hCurrScintilla,
		SCI_REPLACETARGET,
		len,
		reinterpret_cast<LPARAM>(decodedText.data())
	);
}

void convertToAsciiFromBase64()
{
	convertBase64ToAscii(false, false);
}

void urlconvertToAsciiFromBase64()
{
	urlconvertBase64ToAscii(false, false);
}

void convertToAsciiFromBase64_strict()
{
	convertBase64ToAscii(true, false);
}

void convertToAsciiFromBase64_whitespaceReset()
{
	convertBase64ToAscii(false, true);
}


void convertMimeHeaderDecode()
{
	HWND hCurrScintilla = getCurrentScintillaHandle();
	const size_t nbSelections = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);
	if (nbSelections > 1) return;

	size_t start = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONSTART, 0, 0);
	size_t end = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONEND, 0, 0);
	if (end < start)
	{
		const size_t tmp = start;
		start = end;
		end = tmp;
	}
	const size_t selectedLength = end - start;
	if (selectedLength == 0) return;

	char* selectedText = new char[selectedLength + 1];
	::SendMessage(hCurrScintilla, SCI_SETTARGETSTART, start, 0);
	::SendMessage(hCurrScintilla, SCI_SETTARGETEND, end, 0);
	::SendMessage(hCurrScintilla, SCI_GETTARGETTEXT, 0, reinterpret_cast<LPARAM>(selectedText));

	std::string decodedText;
	const unsigned int documentCodePage = static_cast<unsigned int>(
		::SendMessage(hCurrScintilla, SCI_GETCODEPAGE, 0, 0));
	const Rfc2047DecodeResult result = decodeRfc2047Header(
		selectedText, selectedLength, documentCodePage, decodedText);

	if (result.decodedWords == 0)
	{
		::MessageBox(nppData._nppHandle,
			TEXT("No valid RFC 2047 encoded-words were found in the selection."),
			TEXT("MIME Header Decode"), MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		::SendMessage(hCurrScintilla, SCI_SETTARGETSTART, start, 0);
		::SendMessage(hCurrScintilla, SCI_SETTARGETEND, end, 0);
		::SendMessage(hCurrScintilla, SCI_REPLACETARGET, decodedText.size(),
			reinterpret_cast<LPARAM>(decodedText.data()));
		::SendMessage(hCurrScintilla, SCI_SETSEL, start, start + decodedText.size());

		if (result.skippedWords != 0)
		{
			::MessageBox(nppData._nppHandle,
				TEXT("Some malformed, unsupported, or unrepresentable encoded-words were left unchanged."),
				TEXT("MIME Header Decode"), MB_OK | MB_ICONWARNING);
		}
	}

	delete[] selectedText;
}

void convertURLMinEncode()
{
	convertURLEncode (UrlEncodeMethod::RFC1738);
}

void convertURLEncodeExtended()
{
	convertURLEncode (UrlEncodeMethod::extended);
}

void convertURLFullEncode()
{
	convertURLEncode (UrlEncodeMethod::full);
}

void convertURLMinEncodeByLine()
{
	convertURLEncode (UrlEncodeMethod::RFC1738, true);
}

void convertURLEncodeExtendedByLine()
{
	convertURLEncode (UrlEncodeMethod::extended, true);
}

void convertURLFullEncodeByLine()
{
	convertURLEncode (UrlEncodeMethod::full, true);
}

void convertURLEncode (UrlEncodeMethod method, bool isByLine)
{
  HWND hCurrScintilla = getCurrentScintillaHandle();
  size_t nbSelections = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);
  if (nbSelections > 1) return;
  size_t selBufLen = ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
  if (selBufLen == 0) return;

  char * selectedText = new char[selBufLen + 1];
  ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, (LPARAM)selectedText);

  // this line is added to walk around Scintilla 201 bug
  size_t destBufLen = strlen(selectedText) * 3 + 1;
  char* pEncodedText = new char[destBufLen];
  
  int len = AsciiToUrl(pEncodedText, selectedText, int(destBufLen), method, isByLine);

  size_t start = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONSTART, 0, 0);
  size_t end = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONEND, 0, 0);
  if (end < start)
  {
    size_t tmp = start;
    start = end;
    end = tmp;
  }
  ::SendMessage(hCurrScintilla, SCI_SETTARGETSTART, start, 0);
  ::SendMessage(hCurrScintilla, SCI_SETTARGETEND, end, 0);
  ::SendMessage(hCurrScintilla, SCI_REPLACETARGET, len, (LPARAM)pEncodedText);
  ::SendMessage(hCurrScintilla, SCI_SETSEL, start, start+len);

  delete [] pEncodedText;
  delete [] selectedText;
}

void convertURLDecode()
{
  HWND hCurrScintilla = getCurrentScintillaHandle();
  size_t nbSelections = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);
  if (nbSelections > 1) return;
  size_t selBufLen = ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
  if (selBufLen == 0) return;

  char * selectedText = new char[selBufLen + 1];
  ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, (LPARAM)selectedText);

  // this line is added to walk around Scintilla 201 bug
  size_t destBufLen = strlen(selectedText) + 1;
  char* pDecodedText = new char[destBufLen + 1];

  int len = UrlToAscii(pDecodedText, selectedText, int(destBufLen));

  if (len <= -1)
    ::MessageBox(nppData._nppHandle, TEXT("Encoding Invalid!"), TEXT("URL Decode"), MB_OK);
  else
  {
    size_t start = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONSTART, 0, 0);
    size_t end = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONEND, 0, 0);
    if (end < start)
    {
      size_t tmp = start;
      start = end;
      end = tmp;
    }
    ::SendMessage(hCurrScintilla, SCI_SETTARGETSTART, start, 0);
    ::SendMessage(hCurrScintilla, SCI_SETTARGETEND, end, 0);
    ::SendMessage(hCurrScintilla, SCI_REPLACETARGET, len, (LPARAM)pDecodedText);
    ::SendMessage(hCurrScintilla, SCI_SETSEL, start, start+len);
  }	

  delete [] pDecodedText;
  delete [] selectedText;
}

enum qpOp {qp_encode, qp_decode};

void quotedPrintableConvert(qpOp op)
{
	HWND hCurrScintilla = getCurrentScintillaHandle();
	size_t nbSelections = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);
	if (nbSelections > 1) return;
    size_t bufLength = ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
    if (bufLength == 0) return;

	char * selectedText = new char[bufLength + 1];
    ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, (LPARAM)selectedText);

	char *qpText;
	QuotedPrintable qp;

	if (op == qp_decode)
	{
		qpText = qp.decode(selectedText);
		if (!qpText)
		{
			::MessageBox(nppData._nppHandle, TEXT("It's not a valid Quoted-printable text"), TEXT("Quoted-printable decode error"), MB_OK);
			return;
		}
	}
	else
		qpText = qp.encode(selectedText);

	if (qpText == NULL)
		::MessageBox(nppData._nppHandle, TEXT("Problem!"), TEXT("Quoted-printable encoding"), MB_OK);
	else
	{
		size_t start = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONSTART, 0, 0);
		size_t end = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONEND, 0, 0);
		if (end < start)
		{
			size_t tmp = start;
			start = end;
			end = tmp;
		}
		::SendMessage(hCurrScintilla, SCI_SETTARGETSTART, start, 0);
		::SendMessage(hCurrScintilla, SCI_SETTARGETEND, end, 0);
		::SendMessage(hCurrScintilla, SCI_REPLACETARGET, strlen(qpText), (LPARAM)qpText);
		::SendMessage(hCurrScintilla, SCI_SETSEL, start, start+strlen(qpText));
	}
	delete [] selectedText;
}

void convertToAsciiFromQuotedPrintable()
{
	quotedPrintableConvert(qp_decode);
}

void convertToQuotedPrintable()
{
	quotedPrintableConvert(qp_encode);
}

BOOL CALLBACK aboutDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message) 
	{
		case WM_COMMAND:
			switch (LOWORD(wParam))
            {
                case IDCLOSE :
			    {
					::EndDialog(hwnd, 0);
					g_hAboutDlg = nullptr;
					return  TRUE;
				}
			}
			return FALSE;
	}
	return FALSE;
}

void about()
{
	g_hAboutDlg = ::CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), nppData._nppHandle, (DLGPROC)aboutDlgProc, (LPARAM)NULL);
		    
	// Go to center
	RECT rc;
	::GetClientRect(nppData._nppHandle, &rc);
	POINT center;
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	center.x = rc.left + w/2;
	center.y = rc.top + h/2;
	::ClientToScreen(nppData._nppHandle, &center);

	RECT dlgRect;
	::GetClientRect(g_hAboutDlg, &dlgRect);
	int x = center.x - (dlgRect.right - dlgRect.left)/2;
	int y = center.y - (dlgRect.bottom - dlgRect.top)/2;
	::SendMessage(nppData._nppHandle, NPPM_DARKMODESUBCLASSANDTHEME, static_cast<WPARAM>(NppDarkMode::dmfInit), reinterpret_cast<LPARAM>(g_hAboutDlg));

	::SetWindowPos(g_hAboutDlg, HWND_TOP, x, y, (dlgRect.right - dlgRect.left), (dlgRect.bottom - dlgRect.top), SWP_SHOWWINDOW);
}

void convertSamlDecode()
{
  HWND hCurrScintilla = getCurrentScintillaHandle();
  size_t nbSelections = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);
  if (nbSelections > 1) return;
  size_t bufLength = ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
  if (bufLength == 0) return;

  char *selectedText = new char[bufLength + 1];
  char *samlDecodedText = new char[SAML_MESSAGE_MAX_SIZE];
  ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, (LPARAM)selectedText);

  // this line is added to walk around Scintilla 201 bug
  bufLength = strlen(selectedText) + 1;


  int len = samlDecode(samlDecodedText, selectedText, int(bufLength));
  
  switch (len) 
  {
    case 0:
	  ::MessageBox(nppData._nppHandle, TEXT("SAML Decode returned zero size."), TEXT("SAML Decode"), MB_OK);
	  break;
    case SAML_DECODE_ERROR_URLDECODE:
	  ::MessageBox(nppData._nppHandle, TEXT("Could not URL Decode text."), TEXT("SAML Decode"), MB_OK);
	  break;
	case SAML_DECODE_ERROR_BASE64DECODE:
	  ::MessageBox(nppData._nppHandle, TEXT("Could not BASE64 Decode text after URL Decoding."), TEXT("SAML Decode"), MB_OK);
	  break;
	case SAML_DECODE_ERROR_INFLATE:
	  ::MessageBox(nppData._nppHandle, TEXT("Could not inflate text after BASE64 Decoding."), TEXT("SAML Decode"), MB_OK);
	  break;
	default:
      size_t start = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONSTART, 0, 0);
      size_t end = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONEND, 0, 0);
      if (end < start)
      {
        size_t tmp = start;
        start = end;
        end = tmp;
      }
      ::SendMessage(hCurrScintilla, SCI_SETTARGETSTART, start, 0);
      ::SendMessage(hCurrScintilla, SCI_SETTARGETEND, end, 0);
      ::SendMessage(hCurrScintilla, SCI_REPLACETARGET, len, (LPARAM)samlDecodedText);
      ::SendMessage(hCurrScintilla, SCI_SETSEL, start, start+len);
  }
  
  delete [] selectedText;
  delete [] samlDecodedText;
}