#pragma once

#include <sqlite3.h>

namespace op {
namespace sqlite3 {

class BaseSqlite3 {
protected:
    int mRC;
public:
    BaseSqlite3() : mRC(SQLITE_INTERNAL) {}
    virtual ~BaseSqlite3() {}

    bool isOk()   const { return (mRC == SQLITE_OK);   }
    bool isRow()  const { return (mRC == SQLITE_ROW);  }
    bool isDone() const { return (mRC == SQLITE_DONE); }
}; // BaseSqlite3

class Statement : public BaseSqlite3 {
    ::sqlite3_stmt * mStmt;
public:
    Statement() : mStmt(0) {}
    Statement(sqlite3_stmt * stmt) : mStmt(stmt) {}
    Statement(const Statement & o) { assert(!"Not implimented."); }
    Statement(Statement && o) : mStmt(0) { std::swap(mStmt, o.mStmt); }
    ~Statement() { this->finalize(); }

    int step()  { return (mRC = sqlite3_step(mStmt));  }
    int reset() { return (mRC = sqlite3_reset(mStmt)); }

    void finalize() {
        if (mStmt) {
            sqlite3_finalize(mStmt);
            mStmt = 0;
        }
    }
/*
    SQLITE_API double sqlite3_column_double(sqlite3_stmt*, int iCol);
    SQLITE_API int sqlite3_column_int(sqlite3_stmt*, int iCol);
    SQLITE_API sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol);
    SQLITE_API const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);
    SQLITE_API const void *sqlite3_column_text16(sqlite3_stmt*, int iCol);
*/
    double getDouble(unsigned index = 0) {
        return sqlite3_column_double(mStmt, index);
    }
    void setDouble(double value, unsigned index = 1) {
        mRC = sqlite3_bind_double(mStmt, index, value);
    }

    int getInt(unsigned index = 0) {
        return sqlite3_column_int(mStmt, index);
    }
    void setInt(int value, unsigned index = 1) {
        mRC = sqlite3_bind_int(mStmt, index, value);
    }

    const char* getText(unsigned index = 0) {
        return ((const char*)sqlite3_column_text(mStmt, index));
    }
    void setText(char* value, int len, unsigned index = 1) {
        mRC = sqlite3_bind_text(mStmt, index, value, len, 0);
    }

    void getBlob(std::vector<uint8_t> & data, unsigned index = 0) {
        data.clear();
        const uint8_t *ptr = (const uint8_t*) sqlite3_column_blob(mStmt, index);
        int len = sqlite3_column_bytes(mStmt, index);
        if (ptr) data.assign(ptr, ptr + len);
    }
    void bindBlob(const std::vector<uint8_t> & data, unsigned index = 1) {
        assert(mStmt);
        mRC = sqlite3_bind_blob(mStmt, index, data.data(), data.size(), NULL);
    }
}; // Statement

class Connection : public BaseSqlite3 {
    ::sqlite3 *mDB;
public:
    Connection() : mDB(0) {}
    ~Connection() { this->close(); }

    bool open(const std::string & path) {
        if (mDB) this->close();
        mRC = sqlite3_open(path.c_str(), &mDB);
        return (isOk());
    }

    void close() {
        if (mDB) {
            sqlite3_close(mDB);
            mDB = nullptr;
        }
    }

    bool execute(const char * sql) {
        assert(mDB);
        char * errmsg = nullptr;
        mRC = sqlite3_exec(mDB, sql, NULL, NULL, &errmsg);
        return (mRC == SQLITE_OK);
    }

    bool execute(const std::string & sql) {
        return execute(sql.c_str());
    }

    std::string errorString() const {
        assert(mDB);
        const char *errmsg = sqlite3_errmsg(mDB);
        return std::string(errmsg);
    }

    Statement prepare(const std::string & sql) {
        sqlite3_stmt * stmt = 0;
        mRC = sqlite3_prepare_v2(mDB, sql.c_str(), -1, &stmt, NULL);
        return Statement(isOk() ? stmt : 0);
    }
}; // Connection
} // namespace sqlite3
} // namespace op
