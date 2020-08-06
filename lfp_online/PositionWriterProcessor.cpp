#include "PositionWriterProcessor.h"

PositionWriterProcessor::PositionWriterProcessor(LFPBuffer* buf):
                         AbstractWriterProcessor(buf, 
                                                 std::chrono::seconds(
                                                     buf->config_->getInt("pos_writer.writing_interval", 10))),
                         _last_written_pos(0)
{
    _output.open(buf->config_->getString("pos_writer.out_file"));
    _output << "timestamp,first_led_x,first_led_y,second_led_x,second_led_y" << std::endl; // header
}

PositionWriterProcessor::~PositionWriterProcessor()
{
    write_buffer();
}

void PositionWriterProcessor::read_to_buffer()
{
    if (_last_written_pos > buffer->pos_buf_pos_)
        _last_written_pos = buffer->pos_buf_pos_;

    while (_last_written_pos < buffer->pos_buf_pos_)
    {
        //std::cerr << _last_written_pos << " " << buffer->positions_buf_[_last_written_pos] << std::endl;
        _writing_buffer.push(buffer->positions_buf_[_last_written_pos++]);
    }
}

void PositionWriterProcessor::write_buffer()
{
    while (!_writing_buffer.empty())
    {
        const auto& pos = _writing_buffer.front();
        if (pos.valid && pos.timestamp_ > 0)
            _output << pos << std::endl;
        _writing_buffer.pop();
    }
}

