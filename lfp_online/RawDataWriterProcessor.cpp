#include "RawDataWriterProcessor.h"

RawDataWriterProcessor::RawDataWriterProcessor(LFPBuffer *buf):
                        AbstractWriterProcessor(buf,
                                                std::chrono::milliseconds(
                                                buf->config_->getInt("raw_data_writer.writing_interval", 100)))
{
    const int buf_capacity = buf->config_->getInt("sampling.rate", 30000) / 1000 * buf->config_->getInt("channel.num", 128) * _writing_interval.count() * 1.1; // samples per millisecond * channel num * writing interval * 110%
    _writing_buffer.reserve(buf_capacity);

    _output.open(buf->config_->getString("raw_data_writer.data_file"), std::ios::out | std::ios::binary);
    _timestamps_file.open(buf->config_->getString("raw_data_writer.timestamps_file"), std::ios::out | std::ios::binary);
}

RawDataWriterProcessor::~RawDataWriterProcessor()
{
    write_buffer();
}

void RawDataWriterProcessor::read_to_buffer()
{
    std::lock_guard<std::mutex> lg(buffer->chunk_access_mtx_);
    _writing_buffer.insert(_writing_buffer.end(),
                          reinterpret_cast<DataType*>(buffer->chunk_buf_),
                          reinterpret_cast<DataType*>(buffer->chunk_buf_ + buffer->chunk_buf_ptr_in_));
    _timestamps_buffer.insert(_timestamps_buffer.end(),
                             buffer->timestamp_buf_.begin(),
                             buffer->timestamp_buf_.end());
    // std::cout << buffer->chunk_buf_ptr_in_ << " " << _writing_buffer.size() << std::endl;

    buffer->chunk_buf_ptr_in_ = 0;
    buffer->timestamp_buf_.clear();
}

void RawDataWriterProcessor::write_buffer()
{
    _output.write(reinterpret_cast<char*>(_writing_buffer.data()), 
                  sizeof(DataType) * _writing_buffer.size());
    _timestamps_file.write(reinterpret_cast<char*>(_timestamps_buffer.data()),
                           sizeof(int) * _timestamps_buffer.size());
    // std::cout << sizeof(DataType) << " " << _writing_buffer.size() << std::endl;
    reportPerformance(_writing_buffer.size(), _timestamps_buffer.size());
    _writing_buffer.clear();
    _timestamps_buffer.clear();
}

void RawDataWriterProcessor::reportPerformance(int num_written_data, int num_written_timestamps)
{
#ifdef PROFILE_RAW_DATA_WRITER
    _num_written_data.push_back(num_written_data);
    _num_written_timestamps.push_back(num_written_timestamps);

    if (_perf_proc_counter > TEST_STEPS)
    {
        const auto total = std::accumulate(_num_written_data.begin(), _num_written_data.end(), 0.0);
        std::cout << "Total written data: " << total << std::endl;
        std::cout << "Avg written data: " << total / _num_written_data.size() << std::endl;

        total = std::accumulate(_num_written_timestamps.begin(), _num_written_timestamps.end(), 0.0);
        std::cout << "Total written timestamps: " << total << std::endl;
        std::cout << "Avg written timestamps: " << total / _num_written_timestamps.size() << std::endl;
        exit(0);
    }

    _perf_proc_counter++;
#endif
}
