#pragma once 

#include <vector>
#include <string>
#include "debug.hpp"

namespace op {

class buffer {
public:
    explicit buffer(int reserv = 1024) : mOffset(0), mMemOffset(0) {
        mBuff.reserve(reserv);
    }

    void clear() {
        mBuff.clear();
        mOffset = 0;
        mMemOffset = 0;
    }

    unsigned size() const {
        assert(mMemOffset <= mOffset);
        return (mOffset - mMemOffset);
    }

    void memorizeOffset() {
        mMemOffset = mOffset;
    }
    void clearMemOffset() {
        mMemOffset = 0;
    }

    void clearOffset() {
        mMemOffset = 0;
    }
    unsigned getOffset() const {
        return mOffset;
    }
    void setOffset(unsigned value) {
        if (value > mBuff.capacity()) {
            mBuff.reserve(value + 64);
        }
        mOffset = value;
    }

    // unsigned char

    void pushUChar(unsigned char value) {
        mBuff[mOffset] = value;
        setOffset(mOffset + 1);
    }
    unsigned char popUChar() {
        const unsigned int value = mBuff[mMemOffset];
        mMemOffset += 1;
        assert(mMemOffset <= mOffset);
        return value;
    }
    void setUChar(unsigned char value) {
        mBuff[mMemOffset] = value;
    }
    unsigned char getUChar() const {
        return mBuff[mMemOffset];
    }

    // unsigned int

    void pushUInt(unsigned int value) {
        mBuff[mOffset    ] = (value & 0x00ff);
        mBuff[mOffset + 1] = (value & 0xff00) >> 8;
        setOffset(mOffset + 2);
    }
    unsigned int popUInt() {
        const unsigned int value =
            (mBuff[mMemOffset + 1] << 8) | mBuff[mMemOffset];
        mMemOffset += 2;
        assert(mMemOffset <= mOffset);
        return value;
    }
    void setUInt(unsigned int value) {
        mBuff[mMemOffset    ] = (value & 0x00ff);
        mBuff[mMemOffset + 1] = (value & 0xff00) >> 8;
    }
    unsigned int getUInt() const {
        const unsigned int value = mBuff[mMemOffset]
            | (mBuff[mMemOffset + 1] << 8);
        return value;
    }

    // unsigned long

    void pushULong(unsigned long value) {
        mBuff[mOffset    ] = (value & 0x000000ff);
        mBuff[mOffset + 1] = (value & 0x0000ff00) >>  8;
        mBuff[mOffset + 2] = (value & 0x00ff0000) >> 16;
        mBuff[mOffset + 3] = (value & 0xff000000) >> 24;
        setOffset(mOffset + 4);
    }
    unsigned int popULong() {
        const unsigned long value = mBuff[mMemOffset]
            | (mBuff[mMemOffset + 1] <<  8)
            | (mBuff[mMemOffset + 2] << 16)
            | (mBuff[mMemOffset + 3] << 24);
        mMemOffset += 4;
        assert(mMemOffset <= mOffset);
        return value;
    }
    void setULong(unsigned long value) {
        mBuff[mMemOffset    ] = (value & 0x000000ff);
        mBuff[mMemOffset + 1] = (value & 0x0000ff00) >>  8;
        mBuff[mMemOffset + 2] = (value & 0x00ff0000) >> 16;
        mBuff[mMemOffset + 3] = (value & 0xff000000) >> 24;
    }
    unsigned int getULong() const {
        const unsigned long value = mBuff[mMemOffset]
            | (mBuff[mMemOffset + 1] <<  8)
            | (mBuff[mMemOffset + 2] << 16)
            | (mBuff[mMemOffset + 3] << 24);
        return value;
    }

    // string

    void pushString(const std::string & s) {
        unsigned char len = (s.size() > 255 ? 255 : s.size());

        // cut all '\0' chars
        for (;len > 0 && !s[len - 1]; --len);

        mBuff.reserve(mOffset + len + 2);
        pushUChar(len);
        char * p = (char*) &mBuff[mOffset];
        for (unsigned i = 0; i < len; ++i) {
            p[i] = s[i];
        }
        setOffset(mOffset + len);
        pushUChar(0);
    } 
    std::string popString() {
        const unsigned char len = popUChar();
        std::string s;
        s.assign((char*) &mBuff[mMemOffset], len);
        mMemOffset += len + 1;
        assert(mMemOffset <= mOffset);
        return s;
    }
    const char * getString() {
        return ((char*) &mBuff[mMemOffset + 1]);
    }

    unsigned char * data() {
        return (&mBuff[mMemOffset]);
    }
    const unsigned char * data() const {
        return (&mBuff[mMemOffset]);
    }

private:
    std::vector<unsigned char> mBuff;
    unsigned mOffset;    // index for pushing
    unsigned mMemOffset; // index for popping

    buffer(const buffer &);
    buffer& operator=(const buffer &);
};

};
