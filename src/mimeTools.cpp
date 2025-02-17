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

#include "PluginInterface.h"
#include "mimeTools.h"
#include "b64.h"
#include "qp.h"
#include "url.h"
#include "saml.h"


const TCHAR PLUGIN_NAME[] = TEXT("MIME Tools");
const int nbFunc = 23;

HINSTANCE g_hInst = nullptr;;
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

			funcItem[8]._pFunc = NULL;
			funcItem[9]._pFunc = convertToQuotedPrintable;
			funcItem[10]._pFunc = convertToAsciiFromQuotedPrintable;

			funcItem[11]._pFunc = NULL;
			funcItem[12]._pFunc = convertURLMinEncode;
			funcItem[13]._pFunc = convertURLMinEncodeByLine;
			funcItem[14]._pFunc = convertURLEncodeExtended;
			funcItem[15]._pFunc = convertURLEncodeExtendedByLine;
			funcItem[16]._pFunc = convertURLFullEncode;
			funcItem[17]._pFunc = convertURLFullEncodeByLine;
			funcItem[18]._pFunc = convertURLDecode;
			
			funcItem[19]._pFunc = NULL;
			funcItem[20]._pFunc = convertSamlDecode;

			funcItem[21]._pFunc = NULL;
			funcItem[22]._pFunc = about;

			lstrcpy(funcItem[0]._itemName, TEXT("Base64 Encode"));
			lstrcpy(funcItem[1]._itemName, TEXT("Base64 Encode with padding"));
			lstrcpy(funcItem[2]._itemName, TEXT("Base64 Encode with padding by line"));
			lstrcpy(funcItem[3]._itemName, TEXT("Base64 Encode with Unix EOL"));
			lstrcpy(funcItem[4]._itemName, TEXT("Base64 Encode by line"));
			lstrcpy(funcItem[5]._itemName, TEXT("Base64 Decode"));
			lstrcpy(funcItem[6]._itemName, TEXT("Base64 Decode strict"));
			lstrcpy(funcItem[7]._itemName, TEXT("Base64 Decode by line"));

			lstrcpy(funcItem[8]._itemName, TEXT("-SEPARATOR-"));

			lstrcpy(funcItem[9]._itemName, TEXT("Quoted-printable Encode"));
			lstrcpy(funcItem[10]._itemName, TEXT("Quoted-printable Decode"));
			
			lstrcpy(funcItem[11]._itemName, TEXT("-SEPARATOR-"));

			lstrcpy(funcItem[12]._itemName, TEXT("URL Encode (RFC1738)"));
			lstrcpy(funcItem[13]._itemName, TEXT("URL Encode (RFC1738) by line"));
			lstrcpy(funcItem[14]._itemName, TEXT("URL Encode (Extended)"));
			lstrcpy(funcItem[15]._itemName, TEXT("URL Encode (Extended) by line"));
			lstrcpy(funcItem[16]._itemName, TEXT("URL Encode (Full)"));
			lstrcpy(funcItem[17]._itemName, TEXT("URL Encode (Full) by line"));
			lstrcpy(funcItem[18]._itemName, TEXT("URL Decode"));
			
			lstrcpy(funcItem[19]._itemName, TEXT("-SEPARATOR-"));

			lstrcpy(funcItem[20]._itemName, TEXT("SAML Decode"));
			
			lstrcpy(funcItem[21]._itemName, TEXT("-SEPARATOR-"));

			lstrcpy(funcItem[22]._itemName, TEXT("About"));

			funcItem[0]._init2Check = false;
			funcItem[1]._init2Check = false;
			funcItem[2]._init2Check = false;
			funcItem[3]._init2Check = false;
			funcItem[4]._init2Check = false;
			funcItem[5]._init2Check = false;
			funcItem[6]._init2Check = false;
			funcItem[7]._init2Check = false;
			funcItem[8]._init2Check = false;
			funcItem[9]._init2Check = false;
			funcItem[10]._init2Check = false;
			funcItem[11]._init2Check = false;
			funcItem[12]._init2Check = false;
			funcItem[13]._init2Check = false;
			funcItem[14]._init2Check = false;
			funcItem[15]._init2Check = false;
			funcItem[16]._init2Check = false;
			funcItem[17]._init2Check = false;
			funcItem[18]._init2Check = false;

			// If you don't need the shortcut, you have to make it NULL
			funcItem[0]._pShKey = NULL;
			funcItem[1]._pShKey = NULL;
			funcItem[2]._pShKey = NULL;
			funcItem[3]._pShKey = NULL;
			funcItem[4]._pShKey = NULL;
			funcItem[5]._pShKey = NULL;
			funcItem[6]._pShKey = NULL;
			funcItem[7]._pShKey = NULL;
			funcItem[8]._pShKey = NULL;
			funcItem[9]._pShKey = NULL;
			funcItem[10]._pShKey = NULL;
			funcItem[11]._pShKey = NULL;
			funcItem[12]._pShKey = NULL;
			funcItem[13]._pShKey = NULL;
			funcItem[14]._pShKey = NULL;
			funcItem[15]._pShKey = NULL;
			funcItem[16]._pShKey = NULL;
			funcItem[17]._pShKey = NULL;
			funcItem[18]._pShKey = NULL;
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



void convertAsciiToBase64(size_t wrapLength, bool padFlag, bool byLineFlag)
{
	HWND hCurrScintilla = getCurrentScintillaHandle();
	size_t nbSelections = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);
	if (nbSelections > 1) return;

	size_t selectedLength = ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
	if (selectedLength == 0) return;

	char *selectedText = new char[selectedLength + 1];
	::SendMessage(hCurrScintilla, SCI_TARGETFROMSELECTION, 0, 0);
	::SendMessage(hCurrScintilla, SCI_GETTARGETTEXT, 0, (LPARAM)selectedText);

	size_t bufferLength = (selectedLength + 2) / 3 * 4 + 1;
	if (wrapLength > 0)
	{
		bufferLength += bufferLength / wrapLength;
	}

	char *encodedText = new char[bufferLength + 1];
	int len;

	if (padFlag && byLineFlag) 
	{
		delete[] encodedText;
		std::string encodedString;
		encodedString.reserve((selectedLength / 3 + 1) * 4);
		len = base64EncodeWithPaddingByLine(encodedString, selectedText, selectedLength);
		//encodedText = encodedString.c_str();
		encodedText = new char[len];
		encodedString.copy(encodedText, len);
	}
	else {
		len = base64Encode(encodedText, selectedText, selectedLength, wrapLength, padFlag, byLineFlag);
		encodedText[len] = '\0';
	}
    ::SendMessage(hCurrScintilla, SCI_TARGETFROMSELECTION, 0, 0);
    ::SendMessage(hCurrScintilla, SCI_REPLACETARGET, len, (LPARAM)encodedText);

	delete[] selectedText;
	delete[] encodedText;

}


void convertToBase64FromAscii()
{
	convertAsciiToBase64(0, false, false);
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
	size_t nbSelections = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONS, 0, 0);
	if (nbSelections > 1) return;
	size_t selectedLength = ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
	if (selectedLength == 0) return;

	char *selectedText = new char[selectedLength + 1];
	::SendMessage(hCurrScintilla, SCI_TARGETFROMSELECTION, 0, 0);
	::SendMessage(hCurrScintilla, SCI_GETTARGETTEXT, 0, (LPARAM)selectedText);

	char *decodedText = new char[selectedLength];

	int len = base64Decode(decodedText, selectedText, selectedLength, strictFlag, whitespaceReset);

	if (len < 0)
	{
		::MessageBox(nppData._nppHandle, TEXT("Problem!"), TEXT("Base64"), MB_OK);
	}
	else
	{
		decodedText[len] = '\0';
		::SendMessage(hCurrScintilla, SCI_TARGETFROMSELECTION, 0, 0);
        ::SendMessage(hCurrScintilla, SCI_REPLACETARGET, len, (LPARAM)decodedText);
	}

	delete[] selectedText;
	delete[] decodedText;

}

void convertToAsciiFromBase64()
{
	convertBase64ToAscii(false, false);
}

void convertToAsciiFromBase64_strict()
{
	convertBase64ToAscii(true, false);
}

void convertToAsciiFromBase64_whitespaceReset()
{
	convertBase64ToAscii(false, true);
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