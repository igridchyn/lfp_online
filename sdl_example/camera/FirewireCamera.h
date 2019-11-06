#ifndef FIREWIRE_CAMERA_H_
#define FIREWIRE_CAMERA_H_

#include <memory>
#include <dc1394/dc1394.h>

#define DEBUG_CAMERA


namespace camera
{

class FirewireCamera
{
    static constexpr int _number_dms_buffers = 10;
    static constexpr dc1394framerate_t _framerate = DC1394_FRAMERATE_60;
    // static constexpr int _framerate = 60;

    dc1394_t* _context;
    std::unique_ptr<dc1394camera_t> _camera;

    std::unique_ptr<dc1394video_frame_t> _rgb_frame;
    dc1394video_frame_t* _frame = nullptr;

    dc1394featureset_t _features;
    dc1394framerates_t _framerates;
    dc1394speed_t _iso_speed;

    dc1394error_t _err;

    public:
    // width x height - image resolution
    // pos_left, pos_top - position of roi top left corner
    //      default position of (0, 0) means that roi is in the top left corner
    //      roi size is width x height
    FirewireCamera(unsigned int width, unsigned int height, unsigned int pos_left = 0, unsigned int pos_top = 0);
    virtual ~FirewireCamera();

    const dc1394video_frame_t* captureFrame();
};

}

#endif
