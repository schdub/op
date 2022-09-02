//
// Copyright (C) 2018 Oleg Polivets. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#pragma once
#include <string>
#include <algorithm>

namespace op {

// класс для парсинга URL
class URL {
public:
std::string QueryString, // параметры запроса
    Path,     // путь на сервере
    Protocol, // протокол
    Host,     // хост
    Port;     // порт
// функция парсинга
static URL Parse(const std::string &uri) {
    URL result;
    if (uri.length() == 0)
        return result;
    typedef std::string::const_iterator iterator_t;
    iterator_t uriEnd = uri.end();
    // get query start
    iterator_t queryStart = std::find(uri.begin(), uriEnd, '?');
    // протокол
    iterator_t protocolStart = uri.begin();
    iterator_t protocolEnd = std::find(protocolStart, uriEnd, ':');            //"://");
    if (protocolEnd != uriEnd) {
        std::string prot = &*(protocolEnd);
        if ((prot.length() > 3) && (prot.substr(0, 3) == "://")) {
            result.Protocol = std::string(protocolStart, protocolEnd);
            protocolEnd += 3;   //      ://
        } else
            protocolEnd = uri.begin();  // no protocol
    } else
        protocolEnd = uri.begin();  // no protocol
    // хост
    iterator_t hostStart = protocolEnd;
    iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart
    iterator_t hostEnd = std::find(protocolEnd, 
        (pathStart != uriEnd) ? pathStart : queryStart,
        ':');  // check for port
    result.Host = std::string(hostStart, hostEnd);
    // порт
    if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == L':')) {
        // we have a port
        hostEnd++;
        iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
        result.Port = std::string(hostEnd, portEnd);
    }
    // путь
    if (pathStart != uriEnd)
        result.Path = std::string(pathStart+1, queryStart);
    // параметры запроса
    if (queryStart != uriEnd)
        result.QueryString = std::string(queryStart+1, uri.end());
    return result;
}   // Parse

static std::string Encode(const std::string & encoded) {
   const char DEC2HEX[] = "0123456789ABCDEF";
   const unsigned char * pSrc = (const unsigned char *)encoded.c_str();
   const int SRC_LEN = encoded.length();
   unsigned char * const pStart = new unsigned char[SRC_LEN * 3];
   unsigned char * pEnd = pStart;
   const unsigned char * const SRC_END = pSrc + SRC_LEN;
   for (; pSrc < SRC_END; ++pSrc) {
      if (SAFE[*pSrc]) 
         *pEnd++ = *pSrc;
      else {
         // escape this char
         *pEnd++ = '%';
         *pEnd++ = DEC2HEX[*pSrc >> 4];
         *pEnd++ = DEC2HEX[*pSrc & 0x0F];
      }
   }
   std::string sResult((char *)pStart, (char *)pEnd);
   delete [] pStart;
   return sResult;
} // Encode

static std::string Decode(const std::string & decoded) {
   // Note from RFC1630: "Sequences which start with a percent
   // sign but are not followed by two hexadecimal characters
   // (0-9, A-F) are reserved for future extension"
   const char DEC2HEX[] = "0123456789ABCDEF";
   const unsigned char * pSrc = (const unsigned char *)decoded.c_str();
   const int SRC_LEN = decoded.length();
   const unsigned char * const SRC_END = pSrc + SRC_LEN;
   // last decodable '%' 
   const unsigned char * const SRC_LAST_DEC = SRC_END - 2;
   char * const pStart = new char[SRC_LEN];
   char * pEnd = pStart;
   while (pSrc < SRC_LAST_DEC) {
      if (*pSrc == '%') {
         char dec1, dec2;
         if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)])
            && -1 != (dec2 = HEX2DEC[*(pSrc + 2)])) {
            *pEnd++ = (dec1 << 4) + dec2;
            pSrc += 3;
            continue;
         }
      }
      *pEnd++ = *pSrc++;
   }
   // the last 2- chars
   while (pSrc < SRC_END)
      *pEnd++ = *pSrc++;
   std::string sResult(pStart, pEnd);
   delete [] pStart;
   return sResult;
} // Decode

};  // URL
}   // namespace op {