//
// Copyright (C) 2009-2017 Oleg Polivets. All rights reserved.
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

#include "strutils.hpp"

namespace op {
class Settings {
typedef std::map<std::string, std::string> ConfigMap;
public:
    Settings() {}

    const ConfigMap & map() const {
        return mMap;
    }

    void enumStart() {
        mIter = mMap.begin();
    }

    bool enumNext(const std::string ** k, const std::string ** v) {
        *k = NULL;
        *v = NULL;
        if (mIter != mMap.end()) {
            *k = &(mIter->first);
            *v = &(mIter->second);
            ++mIter;
        }
        return (mIter != mMap.end());
    }

    bool parse (const char * path) {
        bool ok = false;
        FILE * fp = fopen(path, "r");
        if (fp != NULL) {
            ok = true;
            char buff[4096], *ptr, *r;
            ConfigMap ret;
            std::string kk;
            while (!feof(fp)) {
                0[buff]=0;
                r = fgets(buff, sizeof(buff)-1, fp);
                if (r == NULL) break;
                if (buff[0]=='#') continue;
                if (buff[0]=='[') {
                    kk = std::string(buff+1, strlen(buff)-3);
                    continue;
                }
                ptr = strchr(buff, '=');
                if (ptr) {
                    *ptr++ = '\0';
                    std::string k(buff);
                    std::string v(ptr);
                    op::StrUtils::trim(k);
                    op::StrUtils::trim(v);
                    ret[kk.empty() ? k : kk+'/'+k ] = v;
                }
            }
            fclose(fp);
            mMap  = ret;
            mIter = mMap.begin();
        }
        return ok;
    }

    const std::string & asString (const std::string & key, const std::string & def = std::string()) const  {
        ConfigMap::const_iterator it = mMap.find(key);
        return it != mMap.end() ? it->second : def;
    }

    bool asBool (const std::string & key, const bool def = false) const  {
        std::string s(asString(key));
        return (s.empty() ? def : ( !s.compare("true") || !s.compare("yes") || !s.compare("1") ));
    }

    int asInt (const std::string & key, const int def = 0) const {
        std::string s(asString(key));
        return (s.empty() ? def : atoi(s.c_str()));
    }

private:
    ConfigMap mMap;
    ConfigMap::const_iterator mIter;
}; // class Settings
}; // namespace op
