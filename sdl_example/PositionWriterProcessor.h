#ifndef POSITION_WRITER_PROCESSOR_H_
#define POSITION_WRITER_PROCESSOR_H_

#include <chrono>
#include "AbstractWriterProcessor.h"



class LFPONLINEAPI PositionWriterProcessor : public AbstractWriterProcessor<std::queue<SpatialInfo>, 
                                                                            std::chrono::seconds,
                                                                            std::ofstream>
{
    int _last_written_pos;

    protected:
        virtual void write_buffer();
        virtual void read_to_buffer();

    public:
        PositionWriterProcessor(LFPBuffer *buf);
        virtual ~PositionWriterProcessor();
        virtual inline std::string name() { return "PositionWriterProcessor"; }
};
#endif
