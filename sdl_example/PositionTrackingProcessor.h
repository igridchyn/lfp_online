#ifndef POSITION_TRACKING_PROCESSOR_H_
#define POSITION_TRACKING_PROCESSOR_H_

#include "LFPProcessor.h"
#include "camera/FirewireCamera.h"
#include <opencv2/imgproc.hpp>

#define PROFILE_POS_TRACKING

class LFPONLINEAPI PositionTrackingProcessor : public LFPProcessor
{
    camera::FirewireCamera _camera;

    std::thread _detection_thread;
    std::mutex _position_detector_mutex;
    std::atomic_bool _detection_on;

    std::queue<SpatialInfo> _animal_positions;

    int _binary_threshold;

    // used for measuring performance, TODO separate profiler?
#ifdef PROFILE_POS_TRACKING
    decltype(std::chrono::high_resolution_clock::now()) _old_time;
    std::vector<int> _time_diffs;
    std::vector<int> _num_spots_detected;
    void reportPerformance(unsigned int detected_spots);
    int _perf_proc_counter = 0;
#endif

    void detect_positions();

    public:
        PositionTrackingProcessor(LFPBuffer *buf);
        virtual ~PositionTrackingProcessor();
        virtual void process();
        virtual inline std::string name() { return "PositionTrackingProcessor"; }
};

#endif
