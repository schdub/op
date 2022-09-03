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
#include <cctype>
#include <iomanip>
#include <sstream>
#include "strutils.hpp"

namespace op {

/*
 * A class that able to parse string with representation of URL
 * for example "protocol://host:port/path?query_string".
 * And also Encode/Decode using urlencode() for invalid URL characters.
 */

class URL {
public:
    std::string QueryString;
    std::string Path;
    std::string Protocol;
    std::string Host;
    std::string Port;

static URL Parse(const std::string & uri) {
    URL result;
    if (uri.length() == 0) {
        return result;
    }
    typedef std::string::const_iterator iterator_t;
    iterator_t uriEnd = uri.end();
    // get query start
    iterator_t queryStart = std::find(uri.begin(), uriEnd, '?');
    // protocol
    iterator_t protocolStart = uri.begin();
    iterator_t protocolEnd = std::find(protocolStart, uriEnd, ':');
    if (protocolEnd != uriEnd) {
        std::string prot = &*(protocolEnd);
        if ((prot.length() > 3) && (prot.substr(0, 3) == "://")) {
            result.Protocol = std::string(protocolStart, protocolEnd);
            protocolEnd += 3;   //      ://
        } else {
            protocolEnd = uri.begin();  // no protocol
        }
    } else {
        protocolEnd = uri.begin();  // no protocol
    }
    // hostname
    iterator_t hostStart = protocolEnd;
    iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart
    iterator_t hostEnd = std::find(protocolEnd, 
        (pathStart != uriEnd) ? pathStart : queryStart, ':');  // check for port
    result.Host = std::string(hostStart, hostEnd);
    // port
    if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == L':')) {
        // we've a port
        hostEnd++;
        iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
        result.Port = std::string(hostEnd, portEnd);
    }
    // path
    if (pathStart != uriEnd) {
        result.Path = std::string(pathStart + 1, queryStart);
    }
    // query string
    if (queryStart != uriEnd) {
        result.QueryString = std::string(queryStart + 1, uri.end());
    }
    return result;
}   // Parse

static std::string Encode(const std::string & value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
} // Encode

static std::string Decode(const std::string & value) {
    std::ostringstream unescaped;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n;) {
        std::string::value_type c = (*i++);
        if (c != '%') {
            unescaped << (c == '+' ? ' ' : c);
            continue;
        }

        if (i + 2 > n) {
            break;
        }
        std::string::value_type h = (*i++);
        std::string::value_type l = (*i++);
        c = 0;
        op::StrUtils::hexByte(h, l, &c);
        unescaped << ((unsigned char) c);
    }

    return unescaped.str();
} // Decode

};  // URL
}   // namespace op {