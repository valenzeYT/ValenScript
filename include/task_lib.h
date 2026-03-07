#ifndef TASK_LIB_H
#define TASK_LIB_H

#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <unordered_map>

struct Value;

namespace task_lib {

void reset_state(int& nextTaskId,
                 int& nextChannelId,
                 std::unordered_map<int, std::shared_future<Value>>& tasks,
                 std::unordered_map<int, std::deque<Value>>& channels);

int spawn(int& nextTaskId,
          std::unordered_map<int, std::shared_future<Value>>& tasks,
          std::mutex& asyncMutex,
          std::function<Value()> fn);

bool done(std::unordered_map<int, std::shared_future<Value>>& tasks, std::mutex& asyncMutex, int id);
Value await(std::unordered_map<int, std::shared_future<Value>>& tasks, std::mutex& asyncMutex, int id);
Value result(std::unordered_map<int, std::shared_future<Value>>& tasks, std::mutex& asyncMutex, int id);

int channel_create(int& nextChannelId,
                   std::unordered_map<int, std::deque<Value>>& channels,
                   std::mutex& asyncMutex);

bool channel_send(std::unordered_map<int, std::deque<Value>>& channels,
                  std::mutex& asyncMutex,
                  int id,
                  const Value& value);

Value channel_recv(std::unordered_map<int, std::deque<Value>>& channels, std::mutex& asyncMutex, int id);

int set_timeout(int& nextTaskId,
                std::unordered_map<int, std::shared_future<Value>>& tasks,
                std::mutex& asyncMutex,
                double ms);

} // namespace task_lib

#endif
