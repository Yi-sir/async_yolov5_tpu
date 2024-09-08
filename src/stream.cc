#include <stream.h>
#include <sys/prctl.h>

std::atomic<int> Stream::streamId(0);

Stream::Stream() {
  isRunning = true;
  apiNum = 0;
  completeNum = 0;
  curId = streamId;
  streamId.fetch_add(1);
  workThread = std::thread(&Stream::loop, this);
}

Stream::~Stream() {
  isRunning = false;
  sync();
  workThread.join();
}

void Stream::loop() {
  prctl(PR_SET_NAME, std::to_string(curId).c_str());
  while (1) {
    std::function<void()> api;
    {
      std::unique_lock<std::mutex> lock(mtx);
      if (!isRunning) {
        return;
      }
      if (apiQueue.empty()) {
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        continue;
      }
      api = std::move(apiQueue.front());
      apiQueue.pop();
    }
    api();
    ++completeNum;
    syncCV.notify_one();
  }
  return;
}

void Stream::sync() {
  std::unique_lock<std::mutex> lock(mtx);
  syncCV.wait(lock, [this] { return apiNum == completeNum; });
  return;
}