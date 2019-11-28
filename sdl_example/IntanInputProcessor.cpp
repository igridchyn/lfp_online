#include "IntanInputProcessor.h"
#include "intan/dataBuffer.h"
#include "intan/rhd2000datablock.h"
#include "intan/rhd2000registers.h"
#include "intan/utils.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

#define CHIP_ID_RHD2132  1
#define CHIP_ID_RHD2216  2
#define CHIP_ID_RHD2164  4
#define CHIP_ID_RHD2164_B  1000
#define REGISTER_59_MISO_A  53
#define REGISTER_59_MISO_B  58
// #define SAMPLES_PER_DATA_BLOCK 30 // also defined in rhd2000DataBlock.h

// private namespace, auxiliary functions
namespace
{

int getDeviceId(Rhd2000DataBlock *dataBlock, int stream, int &register59Value);
template <typename T>
Rhd2000EvalBoard::AmplifierSampleRate numToSampleRate(const T& num);

}

IntanInputProcessor::IntanInputProcessor(LFPBuffer *buf):
                    LFPProcessor(buf),
                    _empty_fifo_step(buffer->config_->getInt("intan.empty_fifo_step", 5)),
                    _proc_counter(0)
{
    if (openBoard() && uploadBoardConfiguration())
    {
        _board.initialize();
        calibrateBoardADC();
        findConnectedAmplifiers();
        _board.setSampleRate(numToSampleRate(buf->SAMPLING_RATE));
        //Now that we have set our sampling rate, we can set the MISO sampling delay which is dependent on the sample rate.
        setBoardCableLengths();
        disableBoardExternalDigitalOutControl();

        // allocate local data buffers
        _amp_buf.reserve(32 * _num_data_streams);
        //_aux_buf.reserve(3 * _num_data_streams);
        //_adc_buf.reserve(8);

        _board.setContinuousRunMode(true);
        _board.run();
        std::cout << "Sampling rate: " << _board.getSampleRate() << "Hz" << std::endl;
    }
}

IntanInputProcessor::~IntanInputProcessor()
{
}


#ifdef PROFILE_INTAN
constexpr int TEST_PROC_STEPS = 500;
#define REPORT_PERFORMANCE(success, num_blocks_to_read) reportPerformance(success, num_blocks_to_read)
#else
#define REPORT_PERFORMANCE
#endif

#ifdef PROFILE_INTAN
void IntanInputProcessor::reportPerformance(bool read_success, int num_blocks_to_read)
{
    const auto now = std::chrono::high_resolution_clock::now();
    const auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - _old_time).count();
    _old_time = now;

    _read_success.push_back(read_success);
    _read_blocks.push_back(num_blocks_to_read);
    _time_diffs.emplace_back(diff);

    if (_perf_proc_counter > TEST_PROC_STEPS)
    {
        std::cout << "Reading time: " << std::endl <<
            " max: " << *std::max_element(_time_diffs.begin()+1, _time_diffs.end()) <<
            " avg: " << std::accumulate(_time_diffs.begin()+1, _time_diffs.end(), 0.0) / double(_time_diffs.size()-1) << std::endl;

        std::cout << "Blocks read: " << std::endl <<
            " max: " << *std::max_element(_read_blocks.begin(), _read_blocks.end()) << 
            " avg: " << std::accumulate(_read_blocks.begin(), _read_blocks.end(), 0) / double(_read_blocks.size()) << std::endl;

        std::cout << "All readings succesful: " << std::boolalpha << std::all_of(_read_success.begin(), _read_success.end(), [](const auto& x){ return x; }) << std::endl;
        std::cout << "Blocks left: " << getBlockNumInFifo() << std::endl;

        exit(0);
    }

    ++_perf_proc_counter;
}
#endif

void IntanInputProcessor::process()
{
    auto num_blocks_to_read = 1;
    if(_proc_counter % _empty_fifo_step == 0)
    {
       num_blocks_to_read = getBlockNumInFifo();
       _proc_counter -= _empty_fifo_step; // to prevent overflow possible during very looong sessions
    }

    if (num_blocks_to_read <= 0)
        return;

    const auto _amp_data_capture = [this](int sample, int timestamp, const unsigned char raw_data[], int data_size) 
    {
        intan::utils::convertUsbWords(_amp_buf, raw_data, data_size);
        buffer->add_data(reinterpret_cast<unsigned char*>(_amp_buf.data()), _amp_buf.size() * sizeof(unsigned short));
        // (_amp_buf[i] - 32768) * 0.195
        last_sample_timestamp_ = timestamp;
        _amp_buf.clear();
    };
    auto success = _board.readRawData(num_blocks_to_read, _amp_data_capture);
    if (success)
    {
        buffer->has_last_sample_timestamp_ = true;
        buffer->last_sample_timestamp_ = last_sample_timestamp_;
    }

    REPORT_PERFORMANCE(success, num_blocks_to_read);

    ++_proc_counter;
}

bool IntanInputProcessor::openBoard()
{
    auto status = _board.open();
    if (status == 1)
        std::cout << "IntanInputProcessor RHD2000 USB Interface Board opened successfully" << std::endl;
    else if (status < 1) 
    {
        if (status == -1) 
        {
            std::cerr << "Cannot load Opal Kelly FrontPanel DLL" << std::endl;
            std::cerr << "Opal Kelly USB drivers not installed. " << std::endl;
        } 
        else 
            std::cerr << "Intan RHD2000 USB Interface Board Not Found" << std::endl;

        return false;
    }

    return status;
}

bool IntanInputProcessor::uploadBoardConfiguration()
{
    // TODO move as class params?
    fs::path data_dir = "../intan/data";
    fs::path bit_file = data_dir / "main.bit";
    auto status = _board.uploadFpgaBitfile(bit_file.string());

    if (!status) 
    {
        std::cerr << "FPGA Configuration File " << bit_file << " Upload Error" << std::endl;
        std::cerr << "Cannot upload configuration file to FPGA" << std::endl;
        return false;
    }
    else
        std::cout << "FPGA Configuration File uploaded successfully" << std::endl;

    return status;
}

void IntanInputProcessor::calibrateBoardADC()
{
  // Select RAM Bank 0 for AuxCmd3 initially, so the ADC is calibrated.
  _board.selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd3, 0);
  _board.selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd3, 0);
  _board.selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd3, 0);
  _board.selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd3, 0);
  
  // Since our longest command sequence is 60 commands, we run the SPI
  // interface for 60 samples.
  _board.setMaxTimeStep(60);
  _board.setContinuousRunMode(false);
  
  // Start SPI interface.
  _board.run();
    
  // Wait for the 60-sample run to complete.
  while (_board.isRunning()) {}
  
  // Read the resulting single data block from the USB interface.
  // We don't need to do anything with this data block; it's used to configure
  // the RHD2000 amplifier chips and to run ADC calibration.
  Rhd2000DataBlock data_block(_board.getNumEnabledDataStreams());
  _board.readDataBlock(&data_block);
  
  // Now that ADC calibration has been performed, we switch to the command sequence
  // that does not execute ADC calibration.
  // command with no fast setttle 
  _board.selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd3,1);
  _board.selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd3,1);
  _board.selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd3,1);
  _board.selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd3,1);
    
  _board.setCableLengthMeters(Rhd2000EvalBoard::PortA, 0.0);
  _board.setCableLengthMeters(Rhd2000EvalBoard::PortB, 0.0);
  _board.setCableLengthMeters(Rhd2000EvalBoard::PortC, 0.0);
  _board.setCableLengthMeters(Rhd2000EvalBoard::PortD, 0.0);

  std::cout << "ADC calibration finished" << std::endl;
}

bool IntanInputProcessor::findConnectedAmplifiers()
{
    int stream, id;
    std::array<int, 4> num_channels_on_port{0, 0, 0, 0};
    std::array<int, MAX_NUM_DATA_STREAMS> chip_id;
    chip_id.fill(-1);
    std::array<int, MAX_NUM_DATA_STREAMS> port_index;
    port_index.fill(-1);
    std::array<int, MAX_NUM_DATA_STREAMS> port_index_old;
    port_index_old.fill(-1);
    std::array<int, MAX_NUM_DATA_STREAMS> chip_id_old;
    chip_id_old.fill(-1);

    std::array<Rhd2000EvalBoard::BoardDataSource, 8> init_stream_ports
    {
        Rhd2000EvalBoard::PortA1,
            Rhd2000EvalBoard::PortA2,
            Rhd2000EvalBoard::PortB1,
            Rhd2000EvalBoard::PortB2,
            Rhd2000EvalBoard::PortC1,
            Rhd2000EvalBoard::PortC2,
            Rhd2000EvalBoard::PortD1,
            Rhd2000EvalBoard::PortD2 
    };

    std::array<Rhd2000EvalBoard::BoardDataSource, 8> init_stream_ddr_ports
    {
        Rhd2000EvalBoard::PortA1Ddr,
            Rhd2000EvalBoard::PortA2Ddr,
            Rhd2000EvalBoard::PortB1Ddr,
            Rhd2000EvalBoard::PortB2Ddr,
            Rhd2000EvalBoard::PortC1Ddr,
            Rhd2000EvalBoard::PortC2Ddr,
            Rhd2000EvalBoard::PortD1Ddr,
            Rhd2000EvalBoard::PortD2Ddr 
    };

    optimizeMUXSettings();

    // Enable all data streams, and set sources to cover one or two chips
    // on Ports A-D.
    _board.setDataSource(0, init_stream_ports[0]);
    _board.setDataSource(1, init_stream_ports[1]);
    _board.setDataSource(2, init_stream_ports[2]);
    _board.setDataSource(3, init_stream_ports[3]);
    _board.setDataSource(4, init_stream_ports[4]);
    _board.setDataSource(5, init_stream_ports[5]);
    _board.setDataSource(6, init_stream_ports[6]);
    _board.setDataSource(7, init_stream_ports[7]);

    port_index_old[0] = 0;
    port_index_old[1] = 0;
    port_index_old[2] = 1;
    port_index_old[3] = 1;
    port_index_old[4] = 2;
    port_index_old[5] = 2;
    port_index_old[6] = 3;
    port_index_old[7] = 3;

    _board.enableDataStream(0, true);
    _board.enableDataStream(1, true);
    _board.enableDataStream(2, true);
    _board.enableDataStream(3, true);
    _board.enableDataStream(4, true);
    _board.enableDataStream(5, true);
    _board.enableDataStream(6, true);
    _board.enableDataStream(7, true);

    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortA,
            Rhd2000EvalBoard::AuxCmd3, 0);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortB,
            Rhd2000EvalBoard::AuxCmd3, 0);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortC,
            Rhd2000EvalBoard::AuxCmd3, 0);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortD,
            Rhd2000EvalBoard::AuxCmd3, 0);

    // Since our longest command sequence is 60 commands, we run the SPI
    // interface for 60 samples.
    _board.setMaxTimeStep(60);
    _board.setContinuousRunMode(false);

    std::array<int, MAX_NUM_DATA_STREAMS> sum_good_delays;
    sum_good_delays.fill(0);
    std::array<int, MAX_NUM_DATA_STREAMS> index_first_good_delay;
    index_first_good_delay.fill(-1);
    std::array<int, MAX_NUM_DATA_STREAMS> index_second_good_delay;
    index_second_good_delay.fill(-1);

    // Run SPI command sequence at all 16 possible FPGA MISO delay settings
    // to find optimum delay for each SPI interface cable.
    int delay;
    Rhd2000DataBlock data_block(_board.getNumEnabledDataStreams());
    int register_59_value;

    for (delay = 0; delay < 16; ++delay) 
    {
        _board.setCableDelay(Rhd2000EvalBoard::PortA, delay);
        _board.setCableDelay(Rhd2000EvalBoard::PortB, delay);
        _board.setCableDelay(Rhd2000EvalBoard::PortC, delay);
        _board.setCableDelay(Rhd2000EvalBoard::PortD, delay);

        // Start SPI interface.
        _board.run();

        // Wait for the 60-sample run to complete.
        while (_board.isRunning()) {}

        // Read the resulting single data block from the USB interface.
        _board.readDataBlock(&data_block);

        // Read the Intan chip ID number from each RHD2000 chip found.
        // Record delay settings that yield good communication with the chip.
        for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) 
        {
            id = getDeviceId(&data_block, stream, register_59_value);
            if (id == CHIP_ID_RHD2132 || id == CHIP_ID_RHD2216 || (id == CHIP_ID_RHD2164 && register_59_value == REGISTER_59_MISO_A)) 
            {
                // std::cerr << "Delay: " << delay << " on stream " << stream << " is good." << endl;

                sum_good_delays[stream] += 1;
                if (index_first_good_delay[stream] == -1) 
                {
                    index_first_good_delay[stream] = delay;
                    chip_id_old[stream] = id;
                } 
                else if (index_second_good_delay[stream] == -1) 
                {
                    index_second_good_delay[stream] = delay;
                    chip_id_old[stream] = id;
                }
            }
        }
    }

    // count the number of Intan chips 
    int num_chips = std::count_if(sum_good_delays.begin(), sum_good_delays.end(), [](const int& x) { return x > 0; });

    std::cout << "Number of Intan chips: " << num_chips << std::endl;
    if(num_chips == 0)
    {
        std::cerr << "No Intan chip was detected" << std::endl;
        return false;
    }

    // Set cable delay settings that yield good communication with each
    // RHD2000 chip.
    std::array<int, MAX_NUM_DATA_STREAMS> optimum_delay;
    optimum_delay.fill(0);

    for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) 
    {
        if (sum_good_delays[stream] == 1 || sum_good_delays[stream] == 2) 
        {
            optimum_delay[stream] = index_first_good_delay[stream];
        } 
        else if (sum_good_delays[stream] > 2) 
        {
            optimum_delay[stream] = index_second_good_delay[stream];
        }
    }

    _board.setCableDelay(Rhd2000EvalBoard::PortA, std::max(optimum_delay[0], optimum_delay[1]));
    _board.setCableDelay(Rhd2000EvalBoard::PortB, std::max(optimum_delay[2], optimum_delay[3]));
    _board.setCableDelay(Rhd2000EvalBoard::PortC, std::max(optimum_delay[4], optimum_delay[5]));
    _board.setCableDelay(Rhd2000EvalBoard::PortD, std::max(optimum_delay[6], optimum_delay[7]));

    auto cable_length_portA = _board.estimateCableLengthMeters(std::max(optimum_delay[0], optimum_delay[1]));
    auto cable_length_portB = _board.estimateCableLengthMeters(std::max(optimum_delay[2], optimum_delay[3]));
    auto cable_length_portC = _board.estimateCableLengthMeters(std::max(optimum_delay[4], optimum_delay[5]));
    auto cable_length_portD = _board.estimateCableLengthMeters(std::max(optimum_delay[6], optimum_delay[7]));
    _cable_lengths = std::make_tuple(cable_length_portA, cable_length_portB, cable_length_portC, cable_length_portD);
    
    // Now that we know which RHD2000 amplifier chips are plugged into each SPI port,
    // add up the total number of amplifier channels on each port and calcualate the number
    // of data streams necessary to convey this data over the USB interface.
    _num_data_streams = 0;
    bool rhd2216ChipPresent = false;
    for (stream = 0; stream < MAX_NUM_DATA_STREAMS; ++stream) 
    {
        if (chip_id_old[stream] == CHIP_ID_RHD2216) 
        {
            _num_data_streams++;

            if (_num_data_streams <= MAX_NUM_DATA_STREAMS) 
                num_channels_on_port[port_index_old[stream]] += 16;

            rhd2216ChipPresent = true;
        }

        if (chip_id_old[stream] == CHIP_ID_RHD2132) 
        {
            _num_data_streams++;

            if (_num_data_streams <= MAX_NUM_DATA_STREAMS) 
                num_channels_on_port[port_index_old[stream]] += 32;
        }

        if (chip_id_old[stream] == CHIP_ID_RHD2164) 
        {
            _num_data_streams += 2;

            if (_num_data_streams <= MAX_NUM_DATA_STREAMS) 
                num_channels_on_port[port_index_old[stream]] += 64;
        }
    }

    std::cout << "Number of amplifier channels: \n";
    for (int i = 0; i < 4; i++)
        std::cout << '\t' << "Port " << i << " : " << num_channels_on_port[i] << " channels";

    auto num_amplifier_channels = std::accumulate(num_channels_on_port.begin(), num_channels_on_port.end(), 0);
    std::cout << '\t' << "Total: " << num_amplifier_channels << std::endl;
    std::cout << "Number of streams required: " << _num_data_streams << std::endl;

    // If the user plugs in more chips than the USB interface can support, throw
    // up a warning that not all channels will be displayed.
    if (_num_data_streams > 8) 
        std::cerr << "Capacity of USB Interface Exceeded" << std::endl
            << "This RHD2000 USB interface _board.can support only 256  amplifier channels.\n";

    // Reconfigure USB data streams in consecutive order to accommodate all connected chips.
    stream = 0;
    for (int oldStream = 0; oldStream < MAX_NUM_DATA_STREAMS; ++oldStream) 
    {
        if ((chip_id_old[oldStream] == CHIP_ID_RHD2216) && (stream < MAX_NUM_DATA_STREAMS)) 
        {
            chip_id[stream] = CHIP_ID_RHD2216;
            port_index[stream] = port_index_old[oldStream];
            _board.enableDataStream(stream, true);
            _board.setDataSource(stream, init_stream_ports[oldStream]);
            stream++;
        } 
        else if ((chip_id_old[oldStream] == CHIP_ID_RHD2132) && (stream < MAX_NUM_DATA_STREAMS)) 
        {
            chip_id[stream] = CHIP_ID_RHD2132;
            port_index[stream] = port_index_old[oldStream];
            _board.enableDataStream(stream, true);
            _board.setDataSource(stream, init_stream_ports[oldStream]);
            stream++ ;
        } 
        else if ((chip_id_old[oldStream] == CHIP_ID_RHD2164) && (stream < MAX_NUM_DATA_STREAMS - 1)) 
        {
            chip_id[stream] = CHIP_ID_RHD2164;
            chip_id[stream + 1] =  CHIP_ID_RHD2164_B;
            port_index[stream] = port_index_old[oldStream];
            port_index[stream + 1] = port_index_old[oldStream];
            _board.enableDataStream(stream, true);
            _board.enableDataStream(stream + 1, true);
            _board.setDataSource(stream, init_stream_ports[oldStream]);
            _board.setDataSource(stream + 1, init_stream_ddr_ports[oldStream]);
            stream += 2;
        }
    }

    std::cout << "Number of usb streams enabled: " << stream << "\n";

    // Disable unused data streams.
    for (; stream < MAX_NUM_DATA_STREAMS; ++stream)
        _board.enableDataStream(stream, false);

    // enable the ports as needed
    std::array<bool, 4> port_enabled;
    std::transform(num_channels_on_port.begin(), num_channels_on_port.end(), port_enabled.begin(), [](const int& nc) { return nc > 0; });

    return true;
}

void IntanInputProcessor::optimizeMUXSettings()
{
    _board.setSampleRate(Rhd2000EvalBoard::SampleRate30000Hz);

    // Set up an RHD2000 register object using this sample rate to
    // optimize MUX-related register settings.
    Rhd2000Registers chip_registers(_board.getSampleRate());
    chip_registers.setDigOutLow();   // Take auxiliary output out of HiZ mode.

    // Create command lists to be uploaded to auxiliary command slots.
    vector<int> command_list;
    auto command_sequence_length = chip_registers.createCommandListUpdateDigOut(command_list);

    // Create a command list for the AuxCmd1 slot.  This command sequence will continuously
    // update Register 3, which controls the auxiliary digital output pin on each RHD2000 chip.
    // In concert with the v1.4 Rhythm FPGA code, this permits real-time control of the digital
    // output pin on chips on each SPI port.
    _board.uploadCommandList(command_list, Rhd2000EvalBoard::AuxCmd1, 0);
    _board.selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd1, 0, command_sequence_length - 1);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd1, 0);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd1, 0);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd1, 0);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd1, 0);

    // Next, we'll create a command list for the AuxCmd2 slot.  This command sequence
    // will sample the temperature sensor and other auxiliary ADC inputs.
    command_sequence_length = chip_registers.createCommandListTempSensor(command_list);
    _board.uploadCommandList(command_list, Rhd2000EvalBoard::AuxCmd2, 0);
    _board.selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd2, 0, command_sequence_length - 1);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd2, 0);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd2, 0);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd2, 0);
    _board.selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd2, 0);

    // For the AuxCmd3 slot, we will create three command sequences.  All sequences
    // will configure and read back the RHD2000 chip registers, but one sequence will
    // also run ADC calibration.  Another sequence will enable amplifier 'fast settle'.

    // Before generating register configuration command sequences, set amplifier
    // bandwidth paramters.
    chip_registers.setDspCutoffFreq(1.0);
    chip_registers.setLowerBandwidth(0.1);
    chip_registers.setUpperBandwidth(30000.0);
    chip_registers.enableDsp(true);

    chip_registers.createCommandListRegisterConfig(command_list, true);
    // Upload version with ADC calibration to AuxCmd3 RAM Bank 0.
    _board.uploadCommandList(command_list, Rhd2000EvalBoard::AuxCmd3, 0);
    _board.selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0, command_sequence_length - 1);

    command_sequence_length = chip_registers.createCommandListRegisterConfig(command_list, false);
    // Upload version with no ADC calibration to AuxCmd3 RAM Bank 1.
    _board.uploadCommandList(command_list, Rhd2000EvalBoard::AuxCmd3, 1);
    _board.selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0, command_sequence_length - 1);

    chip_registers.setFastSettle(true);
    command_sequence_length = chip_registers.createCommandListRegisterConfig(command_list, false);

    // Upload version with fast settle enabled to AuxCmd3 RAM Bank 2.
    // _board.uploadCommandList(command_list, Rhd2000EvalBoard::AuxCmd3, 2);
    // _board.selectAuxCommandLength(Rhd2000EvalBoard::AuxCmd3, 0, command_sequence_length - 1);
    // chip_registers.setFastSettle(false);

    // select the one with fast settle disabled
    // _board.selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd3,1);
    // _board.selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd3,1);
    // _board.selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd3,1);
    // _board.selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd3,1);
}

void IntanInputProcessor::setBoardCableLengths()
{
    _board.setCableLengthMeters(Rhd2000EvalBoard::PortA, std::get<0>(_cable_lengths));
    _board.setCableLengthMeters(Rhd2000EvalBoard::PortB, std::get<1>(_cable_lengths));
    _board.setCableLengthMeters(Rhd2000EvalBoard::PortC, std::get<2>(_cable_lengths));
    _board.setCableLengthMeters(Rhd2000EvalBoard::PortD, std::get<3>(_cable_lengths));
}

void IntanInputProcessor::disableBoardExternalDigitalOutControl()
{
    _board.enableExternalDigOut(Rhd2000EvalBoard::PortA, false);
    _board.enableExternalDigOut(Rhd2000EvalBoard::PortB, false);
    _board.enableExternalDigOut(Rhd2000EvalBoard::PortC, false);
    _board.enableExternalDigOut(Rhd2000EvalBoard::PortD, false);
    _board.setExternalDigOutChannel(Rhd2000EvalBoard::PortA, 0);
    _board.setExternalDigOutChannel(Rhd2000EvalBoard::PortB, 0);
    _board.setExternalDigOutChannel(Rhd2000EvalBoard::PortC, 0);
    _board.setExternalDigOutChannel(Rhd2000EvalBoard::PortD, 0);
}

unsigned int IntanInputProcessor::getBlockNumInFifo()
{
    return _board.numWordsInFifo() / Rhd2000DataBlock::calculateDataBlockSizeInWords(_board.getNumEnabledDataStreams());
}

namespace
{

int getDeviceId(Rhd2000DataBlock *data_block, int stream, int &register_59_value)
{
    // First, check ROM registers 32-36 to verify that they hold 'INTAN', and
    // the initial chip name ROM registers 24-26 that hold 'RHD'.
    // This is just used to verify that we are getting good data over the SPI
    // communication channel.
    bool intanChipPresent = ((char) data_block->auxiliaryData[stream][2][32] == 'I' &&
                             (char) data_block->auxiliaryData[stream][2][33] == 'N' &&
                             (char) data_block->auxiliaryData[stream][2][34] == 'T' &&
                             (char) data_block->auxiliaryData[stream][2][35] == 'A' &&
                             (char) data_block->auxiliaryData[stream][2][36] == 'N' &&
                             (char) data_block->auxiliaryData[stream][2][24] == 'R' &&
                             (char) data_block->auxiliaryData[stream][2][25] == 'H' &&
                             (char) data_block->auxiliaryData[stream][2][26] == 'D');

    // If the SPI communication is bad, return -1.  Otherwise, return the Intan
    // chip ID number stored in ROM regstier 63.

    register_59_value = intanChipPresent ? data_block->auxiliaryData[stream][2][23] : -1; // Register 59
    return intanChipPresent ? data_block->auxiliaryData[stream][2][19] : -1; // chip ID (Register 63)
}

template <typename T>
Rhd2000EvalBoard::AmplifierSampleRate numToSampleRate(const T& num)
{
    switch (num) 
    {
        case T(1000):
            return Rhd2000EvalBoard::SampleRate1000Hz;
            break;
        case T(1250):
            return Rhd2000EvalBoard::SampleRate1250Hz;
            break;
        case T(1500):
            return Rhd2000EvalBoard::SampleRate1500Hz;
            break;
        case T(2000):
            return Rhd2000EvalBoard::SampleRate2000Hz;
            break;
        case T(2500):
            return Rhd2000EvalBoard::SampleRate2500Hz;
            break;
        case T(3000):
            return Rhd2000EvalBoard::SampleRate3000Hz;
            break;
        case T(3333):
            return Rhd2000EvalBoard::SampleRate3333Hz;
            break;
        case T(4000):
            return Rhd2000EvalBoard::SampleRate4000Hz;
            break;
        case T(5000):
            return Rhd2000EvalBoard::SampleRate5000Hz;
            break;
        case T(6250):
            return Rhd2000EvalBoard::SampleRate6250Hz;
            break;
        case T(8000):
            return Rhd2000EvalBoard::SampleRate8000Hz;
            break;
        case T(10000):
            return Rhd2000EvalBoard::SampleRate10000Hz;
            break;
        case T(12500):
            return Rhd2000EvalBoard::SampleRate12500Hz;
            break;
        case T(15000):
            return Rhd2000EvalBoard::SampleRate15000Hz;
            break;
        case T(20000):
            return Rhd2000EvalBoard::SampleRate20000Hz;
            break;
        case T(25000):
            return Rhd2000EvalBoard::SampleRate25000Hz;
            break;
        case T(30000):
            return Rhd2000EvalBoard::SampleRate30000Hz;
            break;
        default:
            throw std::runtime_error("Wrong sampling rate."); // TODO custom exception?
    }
}

}
