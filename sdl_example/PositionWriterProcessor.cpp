#include "PositionWriterProcessor.h"

namespace
{
    void write_positions(std::queue<SpatialInfo>& positions, std::ostream& output);
}

PositionWriterProcessor::PositionWriterProcessor(LFPBuffer* buf):
                         LFPProcessor(buf),
                         _last_written_pos(0),
                         _writing_interval(buf->config_->getInt("pos_writer.writing_interval", 10)),
                         _output_file(buf->config_->getString("pos_writer.out_file")),
                         _writing_on(true)
{
    _output_file << "timestamp,first_led_x,first_led_y,second_led_x,second_led_y" << std::endl; // header
    _writing_thread = std::thread(&PositionWriterProcessor::write_positions_to_file, this);
    _writing_thread.detach();
}

PositionWriterProcessor::~PositionWriterProcessor()
{
    _writing_on = false;
    _writing_thread.join();
    write_positions(_positions, _output_file);
}

void PositionWriterProcessor::process()
{
    if (_last_written_pos > buffer->pos_buf_pos_)
        _last_written_pos = buffer->pos_buf_pos_;

    std::lock_guard<std::mutex> _lg(_positions_mutex);
    while (_last_written_pos < buffer->pos_buf_pos_)
    {
        //std::cerr << _last_written_pos << " " << buffer->positions_buf_[_last_written_pos] << std::endl;
        _positions.push(buffer->positions_buf_[_last_written_pos++]);
    }
}

void PositionWriterProcessor::write_positions_to_file()
{
    while (_writing_on)
    {
        write_positions(_positions, _output_file);
        std::this_thread::sleep_for(_writing_interval);
    }
}

namespace
{
    void write_positions(std::queue<SpatialInfo>& positions, std::ostream& output)
    {
        while (!positions.empty())
        {
            const auto& pos = positions.front();
            if (pos.valid && pos.timestamp_ > 0)
                output << pos << std::endl;
            positions.pop();
        }
    }
}
