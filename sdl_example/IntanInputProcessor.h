#ifndef INTAN_INTPUT_PROCESSOR_H_
#define INTAN_INTPUT_PROCESSOR_H_

#include "LFPProcessor.h"
#include "intan/rhd2000evalboard.h"

using CableLengths = std::tuple<double, double, double, double>;

class LFPONLINEAPI IntanInputProcessor : public LFPProcessor
{
    int _counter;
    Rhd2000EvalBoard _board;
    CableLengths _cable_lengths;

    bool openBoard();
    bool uploadBoardConfiguration();
    void calibrateBoardADC();
    // Find amplifiers and cable lengths.
    bool findConnectedAmplifiers();
    void optimizeMUXSettings();
    void disableBoardExternalDigitalOutControl();

    public:
        IntanInputProcessor(LFPBuffer *buf);
        virtual void process();
        virtual inline std::string name() { return "IntanInputProcessor"; }
        virtual ~IntanInputProcessor();
};

#endif
