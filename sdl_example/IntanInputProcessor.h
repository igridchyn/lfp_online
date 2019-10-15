#ifndef INTAN_INTPUT_PROCESSOR_H_
#define INTAN_INTPUT_PROCESSOR_H_

#include "LFPProcessor.h"

class LFPONLINEAPI IntanInputProcessor : public LFPProcessor
{
    int counter;

    public:
        IntanInputProcessor(LFPBuffer *buf);
        virtual void process();
        virtual inline std::string name() { return "IntanInputProcessor"; }
        virtual ~IntanInputProcessor();
};
#endif
