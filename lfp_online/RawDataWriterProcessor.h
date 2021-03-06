#ifndef RAW_DATA_WRITER_PROCESSOR_H_
#define RAW_DATA_WRITER_PROCESSOR_H_

#include <chrono>
#include "AbstractWriterProcessor.h"


// #define PROFILE_RAW_DATA_WRITER

class LFPONLINEAPI RawDataWriterProcessor : public AbstractWriterProcessor<std::vector<short>,
                                                                            std::chrono::milliseconds,
                                                                            std::ofstream>
{
#ifdef PROFILE_RAW_DATA_WRITER
    int _perf_proc_counter = 0;
    std::vector<int> _num_written_data;
    std::vector<int> _num_written_timestamps;
    static constexpr int TEST_STEPS = 10;
#endif
    void reportPerformance(int num_written, int num_written_timestamps);
    using DataType = decltype(_writing_buffer)::value_type;

    std::vector<int> _timestamps_buffer;
    std::ofstream _timestamps_file;

    protected:
        virtual void write_buffer();
        virtual void read_to_buffer();

    public:
        RawDataWriterProcessor(LFPBuffer *buf);
        virtual ~RawDataWriterProcessor();
        virtual inline std::string name() { return "RawDataWriterProcessor"; }
};

#endif
