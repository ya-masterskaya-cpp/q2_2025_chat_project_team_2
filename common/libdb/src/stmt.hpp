#pragma once
#include <sqlite3.h>
#include <stdexcept>

class Stmt {
public:
    Stmt(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr) != SQLITE_OK)
            throw std::runtime_error("Failed to prepare SQL");
    }

    ~Stmt() {
        if (stmt_)
            sqlite3_finalize(stmt_);
    }

    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;

    Stmt(Stmt&& other) noexcept : stmt_(other.stmt_) {
        other.stmt_ = nullptr;
    }

    Stmt& operator=(Stmt&& other) noexcept {
        if (this != &other) {
            sqlite3_finalize(stmt_);
            stmt_ = other.stmt_;
            other.stmt_ = nullptr;
        }
        return *this;
    }

    void Bind(int index, std::string param) {
        sqlite3_bind_text(stmt_, index, param.c_str(), -1, SQLITE_TRANSIENT);
    }

    void Bind(int index, bool value) {
        sqlite3_bind_int(stmt_, index, static_cast<int>(value));
    }

    void Bind(int index, int64_t value) {
        sqlite3_bind_int64(stmt_, index, value);
    }

    std::string GetColumnText(int col) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, col));
        return text ? text : "";
    }

    sqlite3_stmt* Get() const {
        return stmt_; 
    }

private:
    sqlite3_stmt* stmt_ = nullptr;
};
