#pragma once

#include <mutex>
#include <memory>
#include <queue>
#include <optional>
#include <condition_variable>

struct Request final
{

};

class ThreadSafeQueue final
{
public:
  void push(std::unique_ptr<Request>& request)
  {
    std::lock_guard lock{ m_ };
    queue_.push(std::move(request));
    cond_.notify_one();
  }

  void pop(std::unique_ptr<Request>& request)
  {
    std::unique_lock lock{ m_ };
    cond_.wait(lock, [this] { return !queue_.empty() || stopped; });
    if (!queue_.front())
      return;
    request = std::move(queue_.front());
    queue_.pop();

  }

  void done()
  {
    {
      std::lock_guard lock{ m_ };
      stopped = true;
      queue_.push(nullptr);
    }
    cond_.notify_all();
  }

private:
  std::queue<std::unique_ptr<Request>> queue_;
  mutable std::mutex m_;
  std::condition_variable cond_;
  bool stopped = false;
};