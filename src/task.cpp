#include "../include/interpreter.h"
#include <chrono>
#include <deque>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace task_lib {

void reset_state(int& nextTaskId,
                 int& nextChannelId,
                 std::unordered_map<int, std::shared_future<Value>>& tasks,
                 std::unordered_map<int, std::deque<Value>>& channels) {
    tasks.clear();
    channels.clear();
    nextTaskId = 1;
    nextChannelId = 1;
}

int spawn(int& nextTaskId,
          std::unordered_map<int, std::shared_future<Value>>& tasks,
          std::mutex& asyncMutex,
          std::function<Value()> fn) {
    auto fut = std::async(std::launch::async, [fn = std::move(fn)]() mutable -> Value {
        return fn();
    }).share();
    std::lock_guard<std::mutex> lock(asyncMutex);
    int id = nextTaskId++;
    tasks[id] = std::move(fut);
    return id;
}

bool done(std::unordered_map<int, std::shared_future<Value>>& tasks, std::mutex& asyncMutex, int id) {
    std::shared_future<Value> fut;
    {
        std::lock_guard<std::mutex> lock(asyncMutex);
        auto it = tasks.find(id);
        if (it == tasks.end()) throw std::runtime_error("task_done: task not found");
        fut = it->second;
    }
    return fut.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}

Value await(std::unordered_map<int, std::shared_future<Value>>& tasks, std::mutex& asyncMutex, int id) {
    std::shared_future<Value> fut;
    {
        std::lock_guard<std::mutex> lock(asyncMutex);
        auto it = tasks.find(id);
        if (it == tasks.end()) throw std::runtime_error("await: task not found");
        fut = it->second;
    }
    Value out = fut.get();
    {
        std::lock_guard<std::mutex> lock(asyncMutex);
        tasks.erase(id);
    }
    return out;
}

Value result(std::unordered_map<int, std::shared_future<Value>>& tasks, std::mutex& asyncMutex, int id) {
    std::shared_future<Value> fut;
    {
        std::lock_guard<std::mutex> lock(asyncMutex);
        auto it = tasks.find(id);
        if (it == tasks.end()) throw std::runtime_error("task_result: task not found");
        fut = it->second;
    }
    if (fut.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        return Value::fromNumber(0.0);
    }
    return fut.get();
}

int channel_create(int& nextChannelId,
                   std::unordered_map<int, std::deque<Value>>& channels,
                   std::mutex& asyncMutex) {
    std::lock_guard<std::mutex> lock(asyncMutex);
    int id = nextChannelId++;
    channels[id] = {};
    return id;
}

bool channel_send(std::unordered_map<int, std::deque<Value>>& channels,
                  std::mutex& asyncMutex,
                  int id,
                  const Value& value) {
    std::lock_guard<std::mutex> lock(asyncMutex);
    auto it = channels.find(id);
    if (it == channels.end()) throw std::runtime_error("channel_send: channel not found");
    it->second.push_back(value);
    return true;
}

Value channel_recv(std::unordered_map<int, std::deque<Value>>& channels, std::mutex& asyncMutex, int id) {
    std::lock_guard<std::mutex> lock(asyncMutex);
    auto it = channels.find(id);
    if (it == channels.end()) throw std::runtime_error("channel_recv: channel not found");
    if (it->second.empty()) return Value::fromNumber(0.0);
    Value value = it->second.front();
    it->second.pop_front();
    return value;
}

int set_timeout(int& nextTaskId,
                std::unordered_map<int, std::shared_future<Value>>& tasks,
                std::mutex& asyncMutex,
                double ms) {
    return spawn(nextTaskId, tasks, asyncMutex, [ms]() -> Value {
        if (ms > 0.0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(ms)));
        }
        return Value::fromBool(true);
    });
}

} // namespace task_lib
