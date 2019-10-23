
namespace intan
{
namespace utils
{

template <typename T>
T convertUsbWord(const unsigned char usbBuffer[])
{
    constexpr auto word_len = sizeof(T);
    T res{0};

    for (int i = 0; i < word_len; ++i)
        res |= T(usbBuffer[i]) << (8 * i);

    return res;
}

template <typename T>
void convertUsbWords(std::vector<T>& result, const unsigned char usbBuffer[], int size)
{
    constexpr auto word_len = sizeof(T);

    for (int i = 0; i < size; i += word_len)
        result[i / word_len] = convertUsbWord<T>(usbBuffer + i);
}

}
}
