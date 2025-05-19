#pragma once
#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>

class ConnectionPool {
public:
    ConnectionPool(const std::string& dsn, int pool_size) {
        for (int i = 0; i < pool_size; ++i) {
            connections.push(std::make_shared<pqxx::connection>(dsn));
        }
    }

    std::shared_ptr<pqxx::connection> acquire() {
        std::unique_lock lock(mtx);
        cv.wait(lock, [&]{ return !connections.empty(); });
        auto conn = connections.front();
        connections.pop();
        return conn;
    }

    void release(std::shared_ptr<pqxx::connection> conn) {
        std::lock_guard lock(mtx);
        connections.push(conn);
        cv.notify_one();
    }

private:
    std::queue<std::shared_ptr<pqxx::connection>> connections;
    std::mutex mtx;
    std::condition_variable cv;
};
