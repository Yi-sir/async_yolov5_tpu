#include <condition_variable>
#include <functional>
#include <iostream>
#include <queue>
#include <thread>
#include <vector>
#include <atomic>

class Stream {
 public:
  Stream();
  ~Stream();

  template <typename Func, typename... Args>
  void sendApi(Func f, Args&&... args);

  void sync();

 private:
  static std::atomic<int> streamId;
  int curId;
  std::thread workThread;
  std::queue<std::function<void()>> apiQueue;
  int maxQueueLen = 20;

  void loop();

  std::mutex mtx;
  bool isRunning;

  int apiNum;
  int completeNum;

  std::condition_variable syncCV;
};

template <typename Func, typename... Args>
void Stream::sendApi(Func f, Args&&... args) {
  auto api = std::make_shared<std::function<void()>>(
      std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
  while (1) {
    std::unique_lock<std::mutex> lock(mtx);
    if (apiQueue.size() >= maxQueueLen) {
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }
    apiQueue.emplace([api]() { (*api)(); });
    break;
  }
  ++apiNum;
  return;
}