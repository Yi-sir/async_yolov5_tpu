#include <streamContext.h>

streamContext::streamContext(int n) {
  streamVec.resize(n);
  for (int i = 0; i < n; ++i) {
    streamVec[i] = std::make_shared<Stream>();
  }
}

streamContext::~streamContext() { syncAll(); }

void streamContext::syncAll() {
  for (auto& stream : streamVec) {
    stream->sync();
  }
  return;
}

void streamContext::syncOne(int i) {
  if (i < 0 || i >= streamVec.size()) {
    throw std::invalid_argument("Sync Idx is Invalid");
  }
  streamVec[i]->sync();
  return;
}
