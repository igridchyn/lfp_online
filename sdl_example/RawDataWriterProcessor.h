#ifndef RAW_DATA_WRITER_PROCESSOR_H_
#define RAW_DATA_WRITER_PROCESSOR_H_

#include <chrono>
#include "AbstractWriterProcessor.h"


// #define PROFILE_RAW_DATA_WRITER

class LFPONLINEAPI RawDataWriterProcessor : public AbstractWriterProcessor<std::vector<unsigned short>, 
                                                                            std::chrono::milliseconds,
                                                                            std::ofstream>
{
#ifdef PROFILE_RAW_DATA_WRITER
    int _perf_proc_counter = 0;
    std::vector<int> _num_written;
    static constexpr int TEST_STEPS = 10;
#endif
    void reportPerformance(int num_written);

    protected:
        virtual void write_buffer();
        virtual void read_to_buffer();

    public:
        RawDataWriterProcessor(LFPBuffer *buf);
        virtual ~RawDataWriterProcessor();
        virtual inline std::string name() { return "RawDataWriterProcessor"; }
};

#endif
