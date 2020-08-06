#ifndef ABSTRACT_WRITER_PROCESSOR
#define ABSTRACT_WRITER_PROCESSOR

#include "LFPProcessor.h"


template <typename Buffer, typename WritingInterval, typename Output>
class LFPONLINEAPI AbstractWriterProcessor : public LFPProcessor
{
    std::mutex _writing_buffer_mutex;
    std::thread _writing_thread;
    std::atomic_bool _writing_on;

    void write_buffer_thread()
    {
        while(_writing_on)
        {
            {
                std::lock_guard lg(_writing_buffer_mutex);
                write_buffer();
            }
            std::this_thread::sleep_for(_writing_interval);
        }
    }

    protected:
        WritingInterval _writing_interval;
        Buffer _writing_buffer;
        Output _output;

        virtual void write_buffer() = 0;
        virtual void read_to_buffer() = 0;

    public:
        AbstractWriterProcessor(LFPBuffer *buf, const WritingInterval& writing_interval):
                                LFPProcessor(buf),
                                _writing_interval(writing_interval),
                                _writing_on(true)
        {
            _writing_thread = std::thread(&AbstractWriterProcessor::write_buffer_thread, this);
            _writing_thread.detach();
        }

        virtual ~AbstractWriterProcessor()
        {
            _writing_on = false;
            _writing_thread.join();
            // write_buffer();
        }

        virtual void process()
        {
            std::lock_guard<std::mutex> lg(_writing_buffer_mutex);
            read_to_buffer();
        }
};

#endif
