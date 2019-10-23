#ifndef INTAN_INTPUT_PROCESSOR_H_
#define INTAN_INTPUT_PROCESSOR_H_

#include "LFPProcessor.h"
#include "intan/rhd2000evalboard.h"

using CableLengths = std::tuple<double, double, double, double>;

class LFPONLINEAPI IntanInputProcessor : public LFPProcessor
{
    Rhd2000EvalBoard _board;
    CableLengths _cable_lengths;
    int _num_data_streams;
    decltype(std::chrono::high_resolution_clock::now()) _old_time;

    // temporary buffers for data converted to 16-bit words
    std::vector<unsigned short> _amp_buf;
    //std::vector<unsigned short> _aux_buf;
    //std::vector<unsigned short> _adc_buf;

    // used for measuring, TODO separate profiler?
    int _proc_counter = 0;
    std::vector<int> _time_diffs;
    std::vector<bool> _read_success;
    std::vector<int> _read_blocks;

    bool openBoard();
    bool uploadBoardConfiguration();
    void calibrateBoardADC();
    // Find amplifiers, cable lengths and number of required data streams.
    bool findConnectedAmplifiers();
    void optimizeMUXSettings();
    void setBoardCableLengths();
    void disableBoardExternalDigitalOutControl();
    unsigned int getBlockNumInFifo();

    public:
        IntanInputProcessor(LFPBuffer *buf);
        virtual void process();
        virtual inline std::string name() { return "IntanInputProcessor"; }
        virtual ~IntanInputProcessor();
};

#endif
