// This file is part of Notepad++ plugin MIME Tools project
//Copyright (C)2013 Robert Meakins <rmeakins@users.sf.net>

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


#include "saml.h"
#include "b64.h"
#include "url.h"
#include "tdeflate.h"
#include "tinf.h"

#include <climits>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>


int samlEncode(std::string& dest, const char* samlStr, std::size_t samlLength)
{
  dest.clear();

  if (samlStr == nullptr || samlLength == 0)
    return 0;

  if (samlLength > static_cast<std::size_t>(SAML_MESSAGE_MAX_SIZE))
    return SAML_ENCODE_ERROR_DEFLATE;

  std::vector<unsigned char> compressed;
  if (!tdefl_compress_raw(compressed,
                          reinterpret_cast<const unsigned char*>(samlStr),
                          samlLength))
  {
    return SAML_ENCODE_ERROR_DEFLATE;
  }

  if (compressed.size() > std::numeric_limits<std::size_t>::max() - 2)
    return SAML_ENCODE_ERROR_BASE64;

  const std::size_t base64Groups = (compressed.size() + 2) / 3;
  if (base64Groups > std::numeric_limits<std::size_t>::max() / 4)
    return SAML_ENCODE_ERROR_BASE64;

  const std::size_t base64Capacity = base64Groups * 4;
  if (base64Capacity > static_cast<std::size_t>(INT_MAX))
    return SAML_ENCODE_ERROR_BASE64;

  std::string base64Text(base64Capacity, '\0');
  const int base64Length = base64Encode(
      &base64Text[0],
      reinterpret_cast<const char*>(compressed.data()),
      compressed.size(),
      0,
      true,
      false);

  if (base64Length < 0)
    return SAML_ENCODE_ERROR_BASE64;

  base64Text.resize(static_cast<std::size_t>(base64Length));

  // SAML HTTP-Redirect requires the Base64 value to be URL encoded. Use the
  // extended encoder so '+' is escaped as %2B instead of being vulnerable to
  // form/query parsers that interpret '+' as a space.
  const std::size_t base64Size = base64Text.size();
  if (base64Size > (static_cast<std::size_t>(INT_MAX) - 1) / 3)
    return SAML_ENCODE_ERROR_URLENCODE;

  const int urlCapacity = static_cast<int>(base64Size * 3 + 1);
  std::string urlText(static_cast<std::size_t>(urlCapacity), '\0');
  const int urlLength = AsciiToUrl(&urlText[0], base64Text.c_str(),
                                  urlCapacity, UrlEncodeMethod::extended);
  if (urlLength < 0 || urlLength >= urlCapacity)
    return SAML_ENCODE_ERROR_URLENCODE;

  urlText.resize(static_cast<std::size_t>(urlLength));
  dest = std::move(urlText);
  return urlLength;
}


int samlDecode(char *dest, const char *encodedSamlStr, int bufLength)
{
  char *pUrlDecodedText = new char[bufLength];
	
  memset(dest, 0, SAML_MESSAGE_MAX_SIZE);
  memset(pUrlDecodedText, 0, bufLength);


  // URL Decode
  int urlDecodedLen = UrlToAscii(pUrlDecodedText, encodedSamlStr, bufLength);

  if (urlDecodedLen < 0)
  {
    delete [] pUrlDecodedText;
	return SAML_DECODE_ERROR_URLDECODE;
  }

  char *base64DecodedText = new char[urlDecodedLen + 1];

  int base64DecodedLen = base64Decode(base64DecodedText, pUrlDecodedText, urlDecodedLen, true, false);

  delete[] pUrlDecodedText;

  if (base64DecodedLen < 0)
  {
    delete [] base64DecodedText;
    return SAML_DECODE_ERROR_BASE64DECODE;
  }

  base64DecodedText[base64DecodedLen] = '\0';


  // A SAML message should be longer than 10 chars
  if (base64DecodedLen < 10)
  {
	delete [] base64DecodedText;
	return SAML_DECODE_ERROR_BASE64DECODE;
  }

  // If the first 5 chars are "<?xml" or "<saml", no need to inflate
  if (   (base64DecodedText[0] == '<')
	  && (base64DecodedText[3] == 'm')
	  && (base64DecodedText[4] == 'l'))
  {
	memcpy(dest, base64DecodedText, base64DecodedLen);
	delete [] base64DecodedText;
    return int(base64DecodedLen);
  }
  

  // Inflate the Base64 decoded text
  char *inflatedText = new char[SAML_MESSAGE_MAX_SIZE];
  unsigned int inflatedTextLen = 0;
  
  tinf_init();
  int inflateReturnCode = tinf_uncompress(inflatedText, &inflatedTextLen, base64DecodedText);
  delete [] base64DecodedText;

  if (inflateReturnCode != TINF_OK)
  {
	delete [] inflatedText;
	return SAML_DECODE_ERROR_INFLATE;
  }
  
  memcpy(dest, inflatedText, inflatedTextLen);
  delete [] inflatedText;
  
  // If the first 5 chars are not "<?xml" or "<saml", there's a problem
  if (!( (dest[0] == '<')
	  && (dest[3] == 'm')
	  && (dest[4] == 'l')))
  {
	  return SAML_DECODE_ERROR_INFLATE;
  }
  return int(inflatedTextLen);
  
}