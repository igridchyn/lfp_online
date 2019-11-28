#ifndef POSITION_WRITER_PROCESSOR_H_
#define POSITION_WRITER_PROCESSOR_H_

#include <chrono>
#include "LFPProcessor.h"


class LFPONLINEAPI PositionWriterProcessor : public LFPProcessor
{
    std::mutex _positions_mutex;
    std::queue<SpatialInfo> _positions;
    std::chrono::seconds _writing_interval;
    int _last_written_pos;

    std::thread _writing_thread;
    std::atomic_bool _writing_on;
    std::ofstream _output_file;

    public:
        PositionWriterProcessor(LFPBuffer *buf);
        virtual ~PositionWriterProcessor();
        virtual void process();
        virtual inline std::string name() { return "PositionWriterProcessor"; }
        void write_positions_to_file();
};

#endif
