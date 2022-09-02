#pragma once

#include <string>
#include <map>
#include <cstdio>
#include <vector>

#include "blowfish.h"

namespace op {

static unsigned sMagic = 0xdeaddaed;

class Storage {
	std::wstring mPath;
	bool mPacked; 
	bool mOpened;

	std::vector<unsigned char> mData;
    std::map<unsigned, std::wstring> mValues;
    std::vector<unsigned> mValueKeys;

public:
	Storage(const std::wstring & path)
        : mPath(path), mPacked(true), mOpened(false) {}
	~Storage() {}

	bool load() {
        mData.swap(std::vector<unsigned char>());
        FILE *file = _wfopen(mPath.c_str(), L"rb");
		if (!file) return false;

        fseek(file, 0 , SEEK_END);
        int len = ftell(file);
        fseek(file, 0 , SEEK_SET);

        mData.resize(len);
        fread((char*)&mData[0], len, sizeof(unsigned char), file);

		fclose(file);

        mOpened = true;
        mPacked = true;
        return true;
	}

	bool flush() {
        FILE *file = _wfopen(mPath.c_str(), L"wb+");
        if (!file) return false;
        fwrite((char*) &mData[0], mData.size(), sizeof(unsigned char), file );
        fclose(file);
        return true;
    }

	bool unpack(const std::wstring & key) {
        mPacked = false;

        unsigned char * p = &mData[0];
        unsigned char * e = p + mData.size();
        unsigned l, k = 0;
        bool err = false;

        mValueKeys.clear();
        mValues.clear();

        if (mData.size() == 0) return true;

        op::CBlowFish bf((unsigned char*)key.c_str(), key.size() * sizeof(key[0]));
        bf.Decrypt(&mData[0], mData.size());

        if (*((unsigned*) p) != sMagic) return false;
        p += sizeof(unsigned);

        // TODO: avoid buffer overruns

        for (; p < e;) {
            l = *((unsigned*) p);
            p += sizeof(unsigned);
            if (p + l >= e) {
                err = true;
                break;
            }
            mValueKeys.push_back(k);
            mValues[k++] = std::wstring((wchar_t*)p, l);
            l *= sizeof(wchar_t);
            p += l;
        }

        return true;
	}

    void pack(const std::wstring & key) {
        // calculate in bytes
        unsigned count = 0;
        for (unsigned i = 0, k; i < mValueKeys.size(); ++i) {
            k = mValueKeys[i];
            count += (mValues[k].size() * sizeof(wchar_t) + sizeof(unsigned));
        }
        if (count == 0) return;

        // prepare raw data
        count += sizeof(unsigned);
        for (;count%8; ++count);
        mData.resize(count);

        unsigned char * p = &mData[0];
        *((unsigned*) p)  = sMagic;
        p += sizeof(unsigned);

        for (unsigned i = 0, l; i < mValueKeys.size(); ++i) {
            std::wstring & v = mValues[mValueKeys[i]];

            l = v.size();

            *((unsigned*) p) = l;
            p += sizeof(unsigned);

            l *= sizeof(wchar_t);

            ::memcpy(p, v.c_str(), l);
//            for (unsigned j = 0; i < v.size(); v[j++] = L'\0');

            p += l;
        }

        // TODO: crypt

        op::CBlowFish bf((unsigned char*)key.c_str(), key.size() * sizeof(key[0]));
        bf.Encrypt(&mData[0], mData.size());

        mPacked = true;
    }

	std::wstring get(unsigned id, const std::wstring & defValue = std::wstring()) {
        return ((mValues.find(id) == mValues.end()) ? defValue : mValues[id]);
	}

    void set(unsigned id, const std::wstring & value) {
        mValues[id] = value;
        bool found = false;
        for (unsigned i = 0; i < mValueKeys.size(); ++i) {
            if (mValueKeys[i] == id) {
                found = true;
                break;
            }
        }
        if (!found) mValueKeys.push_back(id);
    }
};

} // namespace op
