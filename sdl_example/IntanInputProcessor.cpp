#include "IntanInputProcessor.h"
#include <iostream>


IntanInputProcessor::IntanInputProcessor(LFPBuffer *buf)
                    :LFPProcessor(buf),
                     counter(0)
{
}

void IntanInputProcessor::process()
{
    unsigned char datum;
    if (counter % 1000 < 500)
        datum = 0;
    else datum = 1;

    std::vector<unsigned char> data(128, datum);
    buffer->add_data(data.data(), 128 * sizeof(unsigned char));
    ++counter;
    std::cout << int(datum) << std::endl;
}

IntanInputProcessor::~IntanInputProcessor()
{
}

