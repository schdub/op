//
// Copyright (C) 2009-2022 Oleg Polivets. All rights reserved.
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
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <algorithm>
#include <map>
#include <stdexcept>

#if defined(WIN32)
#include <Windows.h>
#endif

namespace op {

class StrUtils {
public:
    static bool starts_with(const std::string & a, const std::string & b) {
        return b.size() <= a.size() && a.compare(0, b.size(), b) == 0;
    }

    static bool ends_with(const std::string & a, const std::string & b) {
        if (a.size() < b.size()) {
            return false;
        }
        return (0 == a.compare(a.size() - b.size(), b.size(), b));
    }

#ifdef WIN32
    static std::string fromUtf16ToWindows1251(const std::wstring & wstr) {
        std::string convertedString;
        int requiredSize = WideCharToMultiByte(1251, 0, wstr.c_str(), -1, 0, 0, 0, 0);
        if (requiredSize > 0) {
            std::vector<char> buffer(requiredSize);
            WideCharToMultiByte(1251, 0, wstr.c_str(), -1, &buffer[0], requiredSize, 0, 0);
            convertedString.assign(buffer.begin(), buffer.end() - 1);
        }
        return convertedString;
    }
    static std::string fromUtf16ToUtf8(const std::wstring & wstr) {
        std::string convertedString;
        int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
        if (requiredSize > 0) {
            std::vector<char> buffer(requiredSize);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], requiredSize, 0, 0);
            convertedString.assign(buffer.begin(), buffer.end() - 1);
        }
        return convertedString;
    }
    static std::string fromUtf8ToUtf16(const std::string & str) {
        std::string convertedString;
        int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
        if (requiredSize > 0) {
            std::vector<char> buffer(requiredSize);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, (PWSTR) &buffer[0], requiredSize);
            convertedString.assign(buffer.begin(), buffer.end() - 1);
        }
        return convertedString;
    }
#endif

    static std::wstring utf8_to_utf16(const std::string& utf8) {
        std::vector<unsigned long> unicode;
        size_t i = 0;
        while (i < utf8.size()) {
            unsigned long uni;
            size_t todo;
            bool error = false;
            unsigned char ch = utf8[i++];
            if (ch <= 0x7F) {
                uni = ch;
                todo = 0;
            } else if (ch <= 0xBF) {
                throw std::logic_error("not a UTF-8 string");
            } else if (ch <= 0xDF) {
                uni = ch&0x1F;
                todo = 1;
            } else if (ch <= 0xEF) {
                uni = ch&0x0F;
                todo = 2;
            } else if (ch <= 0xF7) {
                uni = ch&0x07;
                todo = 3;
            } else {
                throw std::logic_error("not a UTF-8 string");
            }
            for (size_t j = 0; j < todo; ++j) {
                if (i == utf8.size())
                    throw std::logic_error("not a UTF-8 string");
                unsigned char ch = utf8[i++];
                if (ch < 0x80 || ch > 0xBF)
                    throw std::logic_error("not a UTF-8 string");
                uni <<= 6;
                uni += ch & 0x3F;
            }
            if (uni >= 0xD800 && uni <= 0xDFFF)
                throw std::logic_error("not a UTF-8 string");
            if (uni > 0x10FFFF)
                throw std::logic_error("not a UTF-8 string");
            unicode.push_back(uni);
        }
        std::wstring utf16;
        for (size_t i = 0; i < unicode.size(); ++i) {
            unsigned long uni = unicode[i];
            if (uni <= 0xFFFF) {
                utf16 += (wchar_t)uni;
            } else {
                uni -= 0x10000;
                utf16 += (wchar_t)((uni >> 10) + 0xD800);
                utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
            }
        }
        return utf16;
    }

    static std::string to_string(int value) {
        char buff[34];
        0[buff] = 0;
        sprintf(buff, "%d", value);
        return std::string(buff);
    }
    static std::string to_string(double value) {
        char buff[34];
        0[buff] = 0;
        sprintf(buff, "%f", value);
        return std::string(buff);
    }

    template <typename C> std::basic_string<C> replaceAll(
        const std::basic_string<C> & original,
        const std::basic_string<C> & from,
        const std::basic_string<C> & to,
        size_t begin = 0
    ) {
        std::basic_string<C> str(original);
        for (size_t index = begin; (index = str.find(from, index)) != std::string::npos;) {
            str.replace(index, from.length(), to);
            index += to.length();
        }
        return str;
    }

    static void replace(
        std::string & s,
        const std::string & a,
        const std::string & b,
        size_t begin = 0
    ) {
        for (size_t idx = begin ;; idx += b.length()) {
            idx = s.find(a, idx);
            if (idx == std::string::npos) break;
            s.replace(idx, a.length(), b);
        }
    }

    static std::string replace(
        const std::string & s,
        const std::string & a,
        const std::string & b,
        size_t begin = 0
    ) {
        std::string r(s);
        op::StrUtils::replace(r, a, b, begin);
        return r;
    }

    static void trim(std::string & s) {
        if (s.empty() == false) { // right
            std::string::iterator p;
            for (p = s.end(); p != s.begin() && ::isspace(*--p););
            if (::isspace(*p) == 0) ++p;
            s.erase(p, s.end());
            if (s.empty() == false) { // left
                for (p = s.begin(); p != s.end() && ::isspace(*p++););
                if (p == s.end() || ::isspace(*p) == 0) --p;
                s.erase(s.begin(), p);
            }
        }
    }

    static std::string toLower(const std::string & s) {
        std::string t(s.begin(), s.end());
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);
        return t;
    }
    static void toLower(std::string & t) {
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    }
    static std::wstring toLower(const std::wstring & s) {
        std::wstring t(s.begin(), s.end());
        std::transform(t.begin(), t.end(), t.begin(), ::towlower);
        return t;
    }
    static void toLower(std::wstring & t) {
        std::transform(t.begin(), t.end(), t.begin(), ::towlower);
    }

    static std::string simplified(const std::string & s,
        const std::string & word_separator = std::string(" ")) {
        int i = 0, ie = s.length();
        for (; i < ie && ::isspace(s[i]); ++i);
        if (i >= ie) return std::string();
        std::string tmp;
        tmp = s[i++];
        for (;;) {
            for (;i < ie && !::isspace(s[i]); ++i) tmp += s[i];
            for (;i < ie &&  ::isspace(s[i]); ++i);
            if (i >= ie) break;
            tmp += word_separator;
        }
        return tmp;
    }

    typedef std::vector< std::string > StringList;
    static StringList split(const std::string & s, char ch = ' ') {
        StringList ret;
        if (s.empty()) return ret;
        std::string tmp(s);
        char * b = (char*) tmp.data(), * e = 0;
        for (;;) {
            e = ::strchr(b, ch);
            if (e == 0) {
                if (*b) ret.push_back(b);
                break;
            }
            *e++ = '\0';
            if (*b) ret.push_back(b);
            b = e;
        }
        return ret;
    }

    typedef std::vector< std::wstring > StringListW;
    static StringListW split(const std::wstring & s, wchar_t ch = ' ') {
        StringListW ret;
        if (s.empty()) return ret;
        std::wstring tmp(s);
        wchar_t * b = (wchar_t*) tmp.data(), * e = 0;
        for (;;) {
            e = ::wcschr(b, ch);
            if (e == 0) {
                if (*b) ret.push_back(b);
                break;
            }
            *e++ = L'\0';
            if (*b) ret.push_back(b);
            b = e;
        }
        return ret;
    }

    template <class C>
    static std::basic_string<C> & Xor(std::basic_string<C> & s, unsigned key = 0xdeaddaed) {
        for (unsigned i = 0; i < s.size(); ++i) s[i] = s[i] ^ ((((unsigned char*)&key)[(i%4)]));
        return s;
    }

    static std::wstring ToW(const std::string & s) {
        return std::wstring(s.begin(), s.end());
    }

    //
    // bin
    //

    static std::string toBin(const unsigned char * s, unsigned len) {
        std::string ret;
        for (unsigned i = 0; i < len; ++i) {
            for (int j = 7; j >= 0; --j) {
                ret.push_back((s[i] & (1<<j)) ? '1' : '0');
            }
        }
        return ret;
    }
    template <class C>
    static std::string toBin(const std::basic_string<C> & s) {
        unsigned char * p = (unsigned char*)s.c_str();
        unsigned const l  = s.length() * sizeof(typename
            std::basic_string<C>::value_type);
        return toBin(p, l);
    } 

    static std::vector<unsigned char> fromBin(const unsigned char * s, unsigned len) {
        std::vector<unsigned char> ret;
        if (len % 8) {
            len = len / 8;
        }
        for (unsigned i = 0; i < len; i += 8) {
            unsigned char t = 0;
            for (int j = 0; j < 8; ++j) {
                if (s[i+j] == '1') {
                    t |= (1 << (7 - j));
                }
            }
            ret.push_back(t);
        }
        return ret;
    }

    //
    // HEX
    //

    static std::string toHex(const unsigned char * s, unsigned len, bool lower_case = false) {
        char h, l, c = lower_case ? 'a' : 'A';
        std::string ret;
        for (unsigned i = 0; i < len; ++i) {
            h = (0xf0 & s[i]) >> 4;
            if (h <= 9) { h += '0'; } else { h = h - 0x0a + c; }
            ret.push_back(h);
            l = 0x0f & s[i];
            if (l <= 9) { l += '0'; } else { l = l - 0x0a + c; }
            ret.push_back(l);
        }
        return ret;
    } 
    template <class C>
    static std::string toHex(const std::basic_string<C> & s, bool lower_case = false) {
        unsigned char * p = (unsigned char*)s.c_str();
        unsigned const  l = s.length() * sizeof(typename std::basic_string<C>::value_type);
        return toHex(p, l, lower_case);
    }
    static bool hexNibble(char ch, char * pOut) {
        bool ok = true;
        if (ch >= 'A' && ch <= 'Z') { *pOut = ch - 'A' + 0x0A; } else
        if (ch >= 'a' && ch <= 'z') { *pOut = ch - 'a' + 0x0A; } else
        if (ch >= '0' && ch <= '9') { *pOut = ch - '0'; } else {
            *pOut = 0;
            ok = false;
        }
        return ok;
    }
    static bool hexByte(char h, char l, char * pOut) {
        bool ok = hexNibble(h, &h) && hexNibble(l, &l);
        *pOut = (h << 4)|l;
        return ok;
    }
    static std::string fromHex(const std::string & hexed) {
        std::string ret;
        int ie = hexed.length();
        ie -= (ie % 2 == 0) ? 1 : 2;
        for (int i = 0; i <= ie;) {
            char h = hexed[i++];
            char l = hexed[i++];
            char t = 0;
            hexByte(h, l, &t);
            ret.push_back(t);
        }
        return ret;
    }

#if 0
    //
    // Base58
    //

    static std::string toBase58(const std::string & plain_text) {
        static const std::string base58_chars =
            "123456789"
            "ABCDEFGHJKLMNPQRSTUVWXYZ"
            "abcdefghijkmnopqrstuvwxyz";
        const unsigned char * bytes_to_encode =
            (const unsigned char *) plain_text.c_str();
        size_t in_len = plain_text.size();
        std::string ret;
        int i = 0;
        unsigned char ch3[3];
        unsigned char ch4[4];
        while (in_len--) {
            ch3[i++] = *(bytes_to_encode++);
            if (i != 3) continue;
            ch4[0] = ( ch3[0] & 0xfc) >> 2;
            ch4[1] = ((ch3[0] & 0x03) << 4) + ((ch3[1] & 0xf0) >> 4);
            ch4[2] = ((ch3[1] & 0x0f) << 2) + ((ch3[2] & 0xc0) >> 6);
            ch4[3] = ch3[2]   & 0x3f;
            for(i = 0; i < 4; ret += base58_chars[ch4[i++]]);
            i = 0;
        }
        if (i) {
            for(int j = i; j < 3;  ch3[j++] = '\0') ;
            ch4[0] = (ch3[0] & 0xfc) >> 2;
            ch4[1] = ((ch3[0] & 0x03) << 4) + ((ch3[1] & 0xf0) >> 4);
            ch4[2] = ((ch3[1] & 0x0f) << 2) + ((ch3[2] & 0xc0) >> 6);
            ch4[3] = ch3[2] & 0x3f;
            for (int j = 0; (j < i + 1); ret += base58_chars[ch4[j++]]) ;
            //for (; i++ < 3; ret += '=');
        }
        return ret;
    }
    static std::string fromBase58(const std::string & encoded_string) {
        static const std::string base58_chars =
            "123456789"
            "ABCDEFGHJKLMNPQRSTUVWXYZ"
            "abcdefghijkmnopqrstuvwxyz";
        int in_len = encoded_string.size();
        int i = 0, j = 0, in_ = 0;
        unsigned char ch4[4], ch3[3];
        std::string ret;
        while (in_len-- && (encoded_string[in_] != '=') &&
              (::isalnum(encoded_string[in_]) ||
              (encoded_string[in_] == '+')    ||
              (encoded_string[in_] == '/'))) {
            ch4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i <4; i++) { ch4[i] = base58_chars.find(ch4[i]); }
                ch3[0] = (ch4[0] << 2) + ((ch4[1] & 0x30) >> 4);
                ch3[1] = ((ch4[1] & 0xf) << 4) + ((ch4[2] & 0x3c) >> 2);
                ch3[2] = ((ch4[2] & 0x3) << 6) + ch4[3];
                for (i = 0; i < 3; ret += ch3[i++]);
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 4; ch4[j++] = 0);
            for (j = 0; j < 4; j++) { ch4[j] = base58_chars.find(ch4[j]); }
            ch3[0] = (ch4[0] << 2) + ((ch4[1] & 0x30) >> 4);
            ch3[1] = ((ch4[1] & 0xf) << 4) + ((ch4[2] & 0x3c) >> 2);
            ch3[2] = ((ch4[2] & 0x3) << 6) + ch4[3];
            for (j = 0; (j < i - 1); ret += ch3[j++]);
        }
        return ret;
    }
#endif

    //
    // Base64
    //

    static std::string toBase64(const std::string & plain_text) {
        static const std::string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        const unsigned char * bytes_to_encode =
            (const unsigned char *) plain_text.c_str();
        size_t in_len = plain_text.size();
        std::string ret;
        int i = 0;
        unsigned char ch3[3];
        unsigned char ch4[4];
        while (in_len--) {
            ch3[i++] = *(bytes_to_encode++);
            if (i != 3) continue;
            ch4[0] = ( ch3[0] & 0xfc) >> 2;
            ch4[1] = ((ch3[0] & 0x03) << 4) + ((ch3[1] & 0xf0) >> 4);
            ch4[2] = ((ch3[1] & 0x0f) << 2) + ((ch3[2] & 0xc0) >> 6);
            ch4[3] = ch3[2]   & 0x3f;
            for(i = 0; i < 4; ret += base64_chars[ch4[i++]]);
            i = 0;
        }
        if (i) {
            for(int j = i; j < 3;  ch3[j++] = '\0') ;
            ch4[0] = (ch3[0] & 0xfc) >> 2;
            ch4[1] = ((ch3[0] & 0x03) << 4) + ((ch3[1] & 0xf0) >> 4);
            ch4[2] = ((ch3[1] & 0x0f) << 2) + ((ch3[2] & 0xc0) >> 6);
            ch4[3] = ch3[2] & 0x3f;
            for (int j = 0; (j < i + 1); ret += base64_chars[ch4[j++]]) ;
            for (; i++ < 3; ret += '=');
        }
        return ret;
    }
    static std::string fromBase64(const std::string & encoded_string) {
        static const std::string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        int in_len = encoded_string.size();
        int i = 0, j = 0, in_ = 0;
        unsigned char ch4[4], ch3[3];
        std::string ret;
        while (in_len-- && (encoded_string[in_] != '=') &&
              (::isalnum(encoded_string[in_]) ||
              (encoded_string[in_] == '+')    ||
              (encoded_string[in_] == '/'))) {
            ch4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i <4; i++) { ch4[i] = base64_chars.find(ch4[i]); }
                ch3[0] = (ch4[0] << 2) + ((ch4[1] & 0x30) >> 4);
                ch3[1] = ((ch4[1] & 0xf) << 4) + ((ch4[2] & 0x3c) >> 2);
                ch3[2] = ((ch4[2] & 0x3) << 6) + ch4[3];
                for (i = 0; i < 3; ret += ch3[i++]);
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 4; ch4[j++] = 0);
            for (j = 0; j < 4; j++) { ch4[j] = base64_chars.find(ch4[j]); }
            ch3[0] = (ch4[0] << 2) + ((ch4[1] & 0x30) >> 4);
            ch3[1] = ((ch4[1] & 0xf) << 4) + ((ch4[2] & 0x3c) >> 2);
            ch3[2] = ((ch4[2] & 0x3) << 6) + ch4[3];
            for (j = 0; (j < i - 1); ret += ch3[j++]);
        }
        return ret;
    }

};

} // namespace op {
