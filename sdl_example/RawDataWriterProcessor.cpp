#include "RawDataWriterProcessor.h"

RawDataWriterProcessor::RawDataWriterProcessor(LFPBuffer *buf):
                        AbstractWriterProcessor(buf,
                                                std::chrono::milliseconds(
                                                buf->config_->getInt("raw_data_writer.writing_interval", 100)))
{
    const int buf_capacity = buf->config_->getInt("sampling.rate", 30000) / 1000 * buf->config_->getInt("channel.num", 128) * _writing_interval.count() * 1.1; // samples per millisecond * channel num * writing interval * 110%
    _writing_buffer.reserve(buf_capacity);

    _output.open(buf->config_->getString("raw_data_writer.out_file"), std::ios::out | std::ios::binary);
}

RawDataWriterProcessor::~RawDataWriterProcessor()
{
    write_buffer();
}

void RawDataWriterProcessor::read_to_buffer()
{
    std::lock_guard<std::mutex> lg(buffer->chunk_access_mtx_);
    _writing_buffer.insert(_writing_buffer.end(),
                          reinterpret_cast<unsigned short*>(buffer->chunk_buf_), 
                          reinterpret_cast<unsigned short*>(buffer->chunk_buf_ + buffer->chunk_buf_ptr_in_));
    buffer->chunk_buf_ptr_in_ = 0;
}

void RawDataWriterProcessor::write_buffer()
{
    _output.write(reinterpret_cast<char*>(_writing_buffer.data()), 
                  sizeof(decltype(_writing_buffer)::value_type) * _writing_buffer.size());
    reportPerformance(_writing_buffer.size());
    _writing_buffer.clear();
}

void RawDataWriterProcessor::reportPerformance(int num_written)
{
#ifdef PROFILE_RAW_DATA_WRITER
    _num_written.push_back(num_written);

    if (_perf_proc_counter > TEST_STEPS)
    {
        const auto total = std::accumulate(_num_written.begin(), _num_written.end(), 0.0);
        std::cout << "Total written: " << total << std::endl;
        std::cout << "Avg written: " << total / _num_written.size() << std::endl;
        exit(0);
    }

    _perf_proc_counter++;
#endif
}
