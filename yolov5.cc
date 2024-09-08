#include "yolov5.hpp"

#include <chrono>

#include "ff_decode.h"
#include "streamContext.h"

int n = 4;
int dev_id = 0;
int testNum = 1e3;
std::string video_path = "/home/xyz/projects/videos/monitor.mp4";
std::string bmodel_path =
    "/home/xyz/projects/sophon-stream/samples/yolov5/data/models/"
    "BM1684X_tpukernel/yolov5s_tpukernel_int8_1b.bmodel";
std::string tpu_kernel_module_path =
    "/home/xyz/projects/sophon-stream/3rdparty/tpu_kernel_module/"
    "libbm1684x_kernel_module.so";
std::string coco_names =
    "/home/xyz/projects/sophon-stream/samples/yolov5/data/coco.names";
float conf_thresh = 0.5;
float nms_thesh = 0.5;
using YoloV5Ptr = std::shared_ptr<YoloV5>;
// using imgPtr = std::shared_ptr<bm_image>;

streamContext ctx(n);
std::vector<VideoDecFFM> decoders(n);
std::vector<BMNNHandlePtr> handles(n);
std::vector<BMNNContextPtr> bm_ctxes(n);
std::vector<YoloV5Ptr> nets(n);
std::vector<YoloV5*> netsPtr(n);
TimeStamp yolov5_ts;
std::vector<std::vector<bm_image>> imgs(n, std::vector<bm_image>(1));
int frame_id = 0, eof = 0, sampleInterval = 1;
std::vector<std::vector<YoloV5BoxVec>> boxes(n);
int64_t pts = 0;

void init() {
  for (int i = 0; i < n; ++i) {
    handles[i] = std::make_shared<BMNNHandle>(dev_id);
    decoders[i].openDec(&(handles[i]->handle()), video_path.c_str());
    decoders[i].setFps(-1);
    bm_ctxes[i] =
        std::make_shared<BMNNContext>(handles[i], bmodel_path.c_str());
    nets[i] = std::make_shared<YoloV5>(bm_ctxes[i]);
    nets[i]->Init(conf_thresh, nms_thesh, tpu_kernel_module_path, coco_names);
    nets[i]->enableProfile(&yolov5_ts);
    netsPtr[i] = nets[i].get();
  }
}

void func() {
  auto decFunc = [&](int idx) {
    imgs[idx][0] = decoders[idx].grab(frame_id, eof, pts, sampleInterval);
  };
  int totalNum = testNum;
  auto start_ = std::chrono::high_resolution_clock::now();
  while (1) {
    for (int i = 0; i < n; ++i) {
      ctx.sendApi(i, decFunc, i);
      ctx.sendApi(i, &YoloV5::Detect, nets[i].get(), std::ref(imgs[i]),
                  std::ref(boxes[i]));
      ctx.sendApi(i, bm_image_destroy, std::ref(imgs[i][0]));
      --totalNum;
    }
    if (totalNum <= 0) {
      break;
    }
  }
  ctx.syncAll();
  auto end_ = std::chrono::high_resolution_clock::now();
  auto dur_ =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_ - start_)
          .count();
  float fps = (testNum / (1.0 * dur_)) * 1e3;
  std::cout << "fps = " << fps << std::endl;
  return;
}

int main() {
  init();
  func();
  return 0;
}