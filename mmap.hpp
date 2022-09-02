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

#ifdef WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/mman.h>
#define INVALID_HANDLE_VALUE (-1)
#define HANDLE int
#endif

namespace op {

class MMap {
public:
    MMap()
        : mFD(INVALID_HANDLE_VALUE)
        , mBegin(0)
        , mEnd(0)
    {}

    ~MMap() {
        closeIt();
    }

    const char* begin() const { return mBegin; }
    const char*   end() const { return mEnd;   }

    void closeIt() {
#ifdef WIN32
        if (mBegin) UnmapViewOfFile(mBegin);
#else
        if (mFD != INVALID_HANDLE_VALUE) ::close(mFD);
#endif
        mBegin = 0;
        mEnd = 0;
        mFD = INVALID_HANDLE_VALUE;
    }

    bool open(const char * path) {
        closeIt();

        HANDLE fd;
        size_t size;
        bool ok = false;

        mBegin = NULL;
        mEnd = NULL;
        mFD = INVALID_HANDLE_VALUE;

#ifdef WIN32
        HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_HANDLE_VALUE) {
            ok = false;
        } else {
            HANDLE hMapping = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
            LPVOID pBuff = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
            if (pBuff == NULL) {
                ok = false;
            } else {
                ok = true;
                mBegin = (LPSTR)pBuff;
                mEnd   = mBegin + GetFileSize(hFile, 0);
                // во время маппинга увеличиваются счетчики для данных объектов
                // ну а эти хэндлы уже не нужны
                CloseHandle(hMapping);
            }
            CloseHandle(hFile);
        }
#else
        struct stat s;
        fd = ::open(path, O_RDONLY);
        ::fstat (fd, & s);
        size = s.st_size;
        void *ret = ::mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (ret != MAP_FAILED) {
            ok = true;
            mBegin = (char*) ret;
            mEnd   = mBegin + size;
        }
#endif
        return ok;
    }
private:
    HANDLE mFD;
    const char * mBegin;
    const char * mEnd;
}; // class MMap
}; // namespace op
