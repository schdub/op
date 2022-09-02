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

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib,"ws2_32.lib")
  #define CLOSE_SOCKET(sd) ::shutdown(sd, 2), ::closesocket(sd)
  #define socklen_t        int  
  #define NETINIT          { WSADATA wd; if (WSAStartup(0x202, &wd)) { WARN("WSAStartup error %d", WSAGetLastError()); } }
  #define IS_EAGAIN        (WSAGetLastError() == WSAETIMEDOUT)
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <errno.h>
  #include <unistd.h>
  #define INVALID_SOCKET   (-1)
  #define SOCKET_ERROR     (-1)
  #define CLOSE_SOCKET(sd) ::shutdown(sd, 2), ::close(sd)
  #define SOCKET           int
  #define NETINIT
  #define IS_EAGAIN        (errno == EAGAIN)
#endif // !WIN32

#include "op/debug.hpp"
#include <sstream>
#include <vector>
#include <cstring> // memset

namespace op {

class NetUtils {
public:
    static bool lookupIPv4(const char * host, struct sockaddr_in * addr, int socktype) {
        unsigned long ret = inet_addr(host);
        if (ret != INADDR_NONE && ret != INADDR_ANY) {
            memcpy(&addr->sin_addr.s_addr, &ret, sizeof(ret));
            return true;
        }
        struct addrinfo hints, *p = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
        hints.ai_socktype = socktype;
        int rv = getaddrinfo(host, NULL, &hints, &p);
        if (rv != 0) {
            WARN("getaddrinfo: %s", gai_strerror(rv));
            return false;
        } 
        memcpy(addr, p->ai_addr, sizeof(struct sockaddr_in));
        freeaddrinfo(p);
        return true;
    }
};

class UDPSocket {
    SOCKET mSocket;
    sockaddr_in mAddr;

    UDPSocket(const UDPSocket &);
    UDPSocket operator= (const UDPSocket &);

public:
    explicit UDPSocket(SOCKET sd)
        : mSocket(sd)
    {}

    explicit UDPSocket(const char * host, unsigned port)
        : mSocket(INVALID_SOCKET)
    {
        mSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (mSocket == INVALID_SOCKET) {
            WARN("socket() err='%d'", get_lasterror());
            return;
        }
        ::memset(&mAddr, 0, sizeof(mAddr));
        if (!op::NetUtils::lookupIPv4(host, &mAddr, SOCK_DGRAM)) {
            WARN("lookup() failed err=%d", get_lasterror());
            return;
        }
        mAddr.sin_family = AF_INET;
        mAddr.sin_port   = htons(port);
    }

    ~UDPSocket() {
        CLOSE_SOCKET(mSocket);
    }

    void release() { mSocket = INVALID_SOCKET; }

    SOCKET sd() const { return mSocket; };

    bool isOk() const { return mSocket != INVALID_SOCKET; }

    bool isEAGAIN() const { return IS_EAGAIN; }

    static int write_all(SOCKET sd, const char * buff, int count,
                         sockaddr_in * addr, int addr_len) {
        return sendto(sd, buff, count, 0, (sockaddr*) addr, addr_len);
    }

    int write_all(const char * buff, int count) {
        return write_all(mSocket, buff, count, &mAddr, sizeof(mAddr));
    }

    static void gracefulclose(SOCKET sd) {
        CLOSE_SOCKET(sd);
    }

    static int get_lasterror() {
        #if WIN32
            return WSAGetLastError();
        #else
            return errno;
        #endif
    }

    static void set_reuseaddr(SOCKET sd, bool value) {
        const int optval = value ? 1 : 0;
        ::setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
    }
};

class TCPSocket {
public:
    explicit TCPSocket(SOCKET sd)
        : mSocket(sd)
    {}

    explicit TCPSocket(const char * host, unsigned port)
        : mSocket(INVALID_SOCKET)
    {
        mSocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (mSocket == INVALID_SOCKET) {
            WARN("socket() err='%d'", get_lasterror());
            return;
        }
        sockaddr_in addr;
        ::memset(&addr, 0, sizeof(addr));
        if (!op::NetUtils::lookupIPv4(host, &addr, SOCK_STREAM)) {
            WARN("lookup() failed err=%d", get_lasterror());
            return;
        }
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);

        if (::connect(mSocket, (sockaddr*) &addr, sizeof(addr)) < 0) {
            WARN("connect() err='%d'", get_lasterror());
            CLOSE_SOCKET(mSocket);
            mSocket = INVALID_SOCKET;
            return;
        }
    }

    ~TCPSocket() {
        CLOSE_SOCKET(mSocket);
    }

    void release() { mSocket = INVALID_SOCKET; }

    SOCKET sd() const { return mSocket; };

    bool isOk() const { return mSocket != INVALID_SOCKET; }

    bool isEAGAIN() const { return IS_EAGAIN; }

    static int write_all(SOCKET sd, const char * buff, int count) {
        int total = 0;
        for (int iret = 0; total < count;) {
            iret = send(sd, buff + total, count - total, 0);
            if (iret <= 0) {
                WARN("send() err='%d'", get_lasterror());
                total = iret;
                break;
            }
            total += iret;
        }
        return total;
    }

    int recvthis(char * buff, int count) {
        return recv(mSocket, buff, count, 0);
    }

    int write_all(const char * buff, int count) {
        return write_all(mSocket, buff, count);
    }

    static int read_all(SOCKET sd, char * buff, int count) {
        int total = 0;
        for (int iret = 0; total < count;) {
            iret = recv(sd, buff + total, count - total, 0);
            if (iret == 0) break;
            if (iret < 0) {
                WARN("recv() sd=%d err='%d'", sd, get_lasterror());
                total = iret;
                break;
            }
            total += iret;
        }
        return total;
    }

    int read_all(char * buff, int count) {
        return read_all(mSocket, buff, count);
    }
    static void gracefulclose(SOCKET sd) {
        CLOSE_SOCKET(sd);
    }

    static int get_lasterror() {
        #if WIN32
            return WSAGetLastError();
        #else
            return errno;
        #endif
    }

    static void set_keepalive(SOCKET sd, bool value) {
        const int optval = value ? 1 : 0;
        ::setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, sizeof(optval));
    }
    static void set_reuseaddr(SOCKET sd, bool value) {
        const int optval = value ? 1 : 0;
        ::setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
    }
    static void set_rcvtimeo(SOCKET sd, unsigned sec = 5, unsigned msec = 0) {
        timeval optval;
        optval.tv_sec  = sec;
        optval.tv_usec = msec;
        ::setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&optval, sizeof(optval));
    }

private:
    SOCKET mSocket;

    TCPSocket(const TCPSocket &);
    TCPSocket operator= (const TCPSocket &);
};

class TCPServer {
public:
    explicit TCPServer(unsigned port) { mIsOk = listenPort(port); }
    virtual ~TCPServer() {}
    virtual void incomingConnection(SOCKET sd, sockaddr * sa, socklen_t sa_len) = 0;

    // no errors?
    bool isOk() { return mIsOk; }

    // main server loop
    void loop() {
        if (!mIsOk) {
            WARN("can't enter server loop");
            return;
        }

        LOG("+LOOP");

        SOCKET      in_socket;
        sockaddr_in in_addr;
        socklen_t   in_addr_len;
        for (;;) {
            if (listen(mSrvSocket , 10) == SOCKET_ERROR) {
                WARN("listen() err='%d'", TCPSocket::get_lasterror());
                break;
            }
            in_addr_len = sizeof(in_addr);
            in_socket   = ::accept(mSrvSocket, (sockaddr*) &in_addr, &in_addr_len);
            if (in_socket <= 0) {
                WARN("accept() err='%d'", TCPSocket::get_lasterror());
                break;
            }
            LOG("accept() ip='%s'", inet_ntoa(in_addr.sin_addr));
            this->incomingConnection(in_socket, (sockaddr*) &in_addr, in_addr_len);
        }

        CLOSE_SOCKET(mSrvSocket);

        LOG("-LOOP");
    }

protected:
    SOCKET mSrvSocket;
    bool mIsOk;

    bool listenPort(unsigned port, bool reuseaddr = false) {
        bool bRet = false;
        mSrvSocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (mSrvSocket  == INVALID_SOCKET) {
            WARN("socket() err='%d'", TCPSocket::get_lasterror());
        } else {
            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family      = AF_INET;
            addr.sin_port        = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;

            if (::bind(mSrvSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
                CLOSE_SOCKET(mSrvSocket);
                WARN("bind() err='%d'", TCPSocket::get_lasterror());
            } else {
                TCPSocket::set_reuseaddr(mSrvSocket, reuseaddr);
                bRet = true;
            }
        }
        return bRet;
    }
};

class HTTP {
private:
    op::TCPSocket mSocket;
    std::vector<char> mData;
    const char * mBody;
    std::string mHost;
    bool mKeepAlive;
    bool mGzip;
    std::string mCookie;
    unsigned mTimeout;

public:
    explicit HTTP(const std::string & host, unsigned port = 80)
        : mSocket(host.c_str(), port)
        , mBody(0)
        , mHost(host)
        , mKeepAlive(false)
        , mGzip(false)
        , mTimeout((unsigned)-1)
    {}

    const char * headers() const { return &mData[0]; }
    const char * body()    const { return mBody; }

    void setKeepAlive(bool value){ mKeepAlive = value; }
    bool isKeepAlive() const     {  return mKeepAlive; }

    void setGzip(bool value)     { mGzip = value; }
    bool isGzip() const          {  return mGzip; }

    void setCookie(const std::string & v) { mCookie = v; }

    void setTimeout(unsigned v) { mTimeout = v; }

    int GET(const std::string & uri, const std::string & data = std::string()) {
        return request("GET", uri, data);
    }

    int POST(const std::string & uri, const std::string & data = std::string()) {
        return request("POST", uri, data);
    }

    int request(const std::string & method, const std::string & uri, const std::string & d = std::string()) {
        if (!mSocket.isOk()) {
            return 500;
        }

//        bool hasTimeout = (mTimeout != (unsigned) -1);
//        if (hasTimeout) {
//            TCPSocket::set_rcvtimeo(mSocket.sd(), 0, 300);
//        }

        bool postMethod = (!method.compare("POST"));

        std::stringstream ss;
        ss << method << " ";
        if (uri.at(0) != '/')
            ss << '/';
        ss << uri;
        if (!postMethod && !d.empty()) {
            ss << "?" << d;
        }
        ss << " HTTP/1.1\r\n";
        ss << "Host: " <<  mHost << "\r\n";
        if (isGzip()) {
            ss << "Accept-Encoding: gzip, deflate\r\n";
        }
        ss << "User-Agent: opHttp/1.1\r\n";
        if (postMethod) {
            ss << "Content-Type: application/x-www-form-urlencoded\r\n"
               << "Content-Length: " << d.size() << "\r\n";
        }
        if (!mCookie.empty()) {
            ss << "Cookie: " << mCookie << "\r\n";
        }
        if (isKeepAlive() == false) {
            ss << "Connection: close\r\n";
        }
        ss << "\r\n";
        if (postMethod) {
            ss << d;
        }

        {
        const std::string & h = ss.str();
        mSocket.write_all(h.c_str(), h.size());
#if (OPNET_HTTP_LOG_REQ == 1)
        JUSTLOG("%s", h.c_str());
#endif
        }

        unsigned total = 0;
//        unsigned tsStart = GetTickCount();
        for (;;) {
            if (total + 4096 > mData.size())
                mData.resize(mData.size() + 4096);
            int rc = mSocket.read_all(&mData[0]+total, 4096);
            if (rc == 0) {
                break;
            }
            if (rc < 0) {
                return 500;
            }
            total += (unsigned) rc;
//            if (hasTimeout && (GetTickCount() - tsStart >= mTimeout))
//                return 500;
        }
        if (mData.empty()) {
            return 500;
        }
        // разделяем заголовки и тело отклика
        mData[total] = '\0';
#if (OPNET_HTTP_LOG_RESP == 1)
        JUSTLOG("%s", &mData[0]);
#endif
        char * p = strstr(&mData[0], "\r\n\r\n");
        mBody = p ? p + 4 : NULL;

        // ищем ответ сервера
        p = strchr(&mData[0], ' ');
        return p ? atoi(++p) : 500;
    }
};

} // namespace op
