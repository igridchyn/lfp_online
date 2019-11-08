#include "FirewireCamera.h"
#include <algorithm>
#include <iostream>


namespace
{
void print_camera_video_mode_info(dc1394camera_t* camera, const dc1394video_mode_t& video_mode);

#ifdef DEBUG_CAMERA
#define PRINT_CAMERA_VIDEO_MODE_INFO(camera, video_mode) print_camera_video_mode_info(camera, video_mode);
#else
#define PRINT_CAMERA_VIDEO_MODE_INFO(camera, video_mode)
#endif
}

namespace camera
{

FirewireCamera::FirewireCamera(unsigned int width, unsigned int height, unsigned int pos_left, unsigned int pos_top)
{
    _context = dc1394_new();
    if (!_context)
        throw std::runtime_error("Cannot create camera, error with dc1394_new()");

    dc1394camera_list_t* camera_list;
    _err = dc1394_camera_enumerate(_context, &camera_list);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Cannot enumerate cameras.");

    if (camera_list->num == 0)
        throw std::runtime_error("No camera found");

    _camera = std::unique_ptr<dc1394camera_t>(dc1394_camera_new(_context, camera_list->ids[0].guid));
    if (!_camera)
        throw std::runtime_error("Failed to initialize camera with guid " + camera_list->ids[0].guid);

    std::cout << "Camera " << _camera->vendor << " " << _camera->model << " successfuly initialized." << std::endl;
    
    dc1394video_modes_t supported_video_modes;
    _err = dc1394_video_get_supported_modes(_camera.get(), &supported_video_modes);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Cannot get supported video modes.");

    // check if camera can do scalable MONO8 mode
    const auto color_coding = DC1394_COLOR_CODING_MONO8;
    const auto video_mode = 
    std::find_if(std::begin(supported_video_modes.modes), std::end(supported_video_modes.modes),
            [this, color_coding] (const auto& mode)
            {
                dc1394color_coding_t coding;
                dc1394_get_color_coding_from_video_mode(this->_camera.get(), mode, &coding);
                return dc1394_is_video_mode_scalable(mode); // && coding == color_coding;
            });
            
    if (video_mode == std::end(supported_video_modes.modes))
        throw std::runtime_error("Scalable video mode is not supported.");

    unsigned int max_width, max_height;
    _err = dc1394_format7_get_max_image_size(_camera.get(), *video_mode, &max_width, &max_height);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not read max image size.");

    _err = dc1394_video_set_mode(_camera.get(), *video_mode);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not set video mode.");

    _err = dc1394_video_set_operation_mode(_camera.get(), DC1394_OPERATION_MODE_1394B);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not set video operation mode.");

    _err = dc1394_format7_set_color_coding(_camera.get(), *video_mode, color_coding);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not set format7 colour coding.");

    if (width > max_width)
        throw std::runtime_error("Requested image width is bigger than maximal supported (" + std::to_string(max_width) + ".");

    if (height > max_height)
        throw std::runtime_error("Requested image height is bigger than maximal supported (" + std::to_string(max_height) + ".");

    _err = dc1394_format7_set_image_size(_camera.get(), *video_mode, width, height);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not set format7 image size.");

    _err = dc1394_format7_set_image_position(_camera.get(), *video_mode, 0, 0);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not set format7 image position.");

    _err = dc1394_video_set_iso_speed(_camera.get(), DC1394_ISO_SPEED_800);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not set ISO speed 800.");

    _err = dc1394_video_set_framerate(_camera.get(), _framerate);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not set video frame rate.");

    _err = dc1394_video_set_one_shot(_camera.get(), DC1394_ON);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not set one shot mode.");

    _err = dc1394_format7_set_roi(_camera.get(), *video_mode, color_coding, 8192, pos_left, pos_top, width, height);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not set video mode, color coding, frame rate, left, top, width, height for camera.");

    PRINT_CAMERA_VIDEO_MODE_INFO(_camera.get(), *video_mode);

    _err = dc1394_capture_setup(_camera.get(), _number_dms_buffers, DC1394_CAPTURE_FLAGS_DEFAULT);
    if (_err != DC1394_SUCCESS)
        throw std::runtime_error("Could not setup camera - make sure that the video mode and framerate are supported by your camera.");

    _rgb_frame = new dc1394video_frame_t();
    _rgb_frame->color_coding = DC1394_COLOR_CODING_RGB8;

    dc1394_video_set_transmission(_camera.get(), DC1394_ON);
}

FirewireCamera::~FirewireCamera()
{
    dc1394_video_set_transmission(_camera.get(), DC1394_OFF);
    dc1394_capture_stop(_camera.get());
    dc1394_camera_free(_camera.get());
    delete _rgb_frame;
}

const dc1394video_frame_t* FirewireCamera::captureFrame(bool rgb)
{
    if (_frame)
        dc1394_capture_enqueue(_camera.get(), _frame);

    const auto capture_success = dc1394_capture_dequeue(_camera.get(), DC1394_CAPTURE_POLICY_WAIT, &_frame) == DC1394_SUCCESS;

    if (!capture_success)
        return nullptr;

    if (!rgb)
        return _frame;

    const auto debayer_success = dc1394_debayer_frames(_frame, _rgb_frame, DC1394_BAYER_METHOD_BILINEAR) == DC1394_SUCCESS;

    return debayer_success ? _rgb_frame : nullptr;
}

}

namespace
{

void print_camera_video_mode_info(dc1394camera_t* camera, const dc1394video_mode_t& video_mode)
{
    unsigned int x, y;
    float z;
    long unsigned int xx;

    std::cout << "*********************** camera video mode info *********************** " << std::endl;
    dc1394_format7_get_image_size(camera, video_mode, &x, &y);
    std::cout << "get image size(x, y): " << x << ",  " << y << std::endl;

    dc1394_format7_get_image_position(camera, video_mode, &x, &y);
    std::cout << "get image position(x, y): " << x << ",  " << y << std::endl;

    dc1394_format7_get_packet_parameters(camera, video_mode, &x, &y);
    std::cout << "get packet parameters (min,  max): " << x << ",  " << y << std::endl;

    dc1394_format7_get_packet_size(camera, video_mode, &x);
    std::cout << "get packet size (bytes): " << x << std::endl;

    dc1394_format7_get_recommended_packet_size(camera, video_mode, &x);
    std::cout << "recommended packet size (bytes): " << x << std::endl;

    dc1394_format7_get_packets_per_frame(camera, video_mode, &x);
    std::cout << "packets per frame (bytes): " << x << std::endl;

    dc1394_format7_get_data_depth(camera, video_mode, &x);
    std::cout << "depth of pixels (bits): " << x << std::endl;

    dc1394_format7_get_frame_interval(camera, video_mode, &z);
    std::cout << "time interval between frames (ms): " << z << std::endl;

    dc1394_format7_get_total_bytes(camera, video_mode, &xx);
    std::cout << "bytes per frame: " << xx << std::endl;

    std::cout << "*********************** camera video mode info *********************** " << std::endl;
}

}
