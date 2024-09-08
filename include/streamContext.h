#include <stream.h>

class streamContext {
 public:
  streamContext() = delete;
  streamContext(int n);
  void syncAll();
  void syncOne(int i);

  template <typename Func, typename... Args>
  void sendApi(int i, Func f, Args&&... args);
  ~streamContext();

 private:
  std::vector<std::shared_ptr<Stream>> streamVec;
};

template <typename Func, typename... Args>
void streamContext::sendApi(int i, Func f, Args&&... args) {
  if (i < 0 || i >= streamVec.size()) {
    throw std::invalid_argument("Sync Idx is Invalid");
  }
  streamVec[i]->sendApi(std::forward<Func>(f), std::forward<Args>(args)...);
  return;
}