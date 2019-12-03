#include "PositionTrackingProcessor.h"
#include <iostream>
#include <filesystem>
#include <future>
#include <cctype>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>

namespace fs = std::filesystem;

constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
constexpr int DEFAULT_LEFT_POS = 0;
constexpr int DEFAULT_TOP_POS = 0;


namespace
{
    int getLEDChannel(std::string&& channel);
    SpatialInfo detectPositionRGB(cv::Mat& first_led_frame, cv::Mat& second_led_frame, int bin_thr_1, int bin_thr_2, int timestamp);
    SpatialInfo detectPositionGreyscale(cv::Mat& frame, int bin_thr, int timestamp);
}

#ifdef PROFILE_POS_TRACKING
constexpr int TEST_PROC_STEPS = 10000;
#define REPORT_PERFORMANCE() reportPerformance()
#else
#define REPORT_PERFORMANCE()
#endif

PositionTrackingProcessor::PositionTrackingProcessor(LFPBuffer *buf):
                    LFPProcessor(buf),
                    _camera(buffer->config_->getInt("pos_track.im_width", DEFAULT_WIDTH), 
                            buffer->config_->getInt("pos_track.im_height", DEFAULT_HEIGHT), 
                            buffer->config_->getInt("pos_track.im_pos_left", DEFAULT_LEFT_POS), 
                            buffer->config_->getInt("pos_track.im_pos_top", DEFAULT_TOP_POS)),
                    _binary_threshold_1(buffer->config_->getInt("pos_track.bin_thr_1", 75)),
                    _binary_threshold_2(buffer->config_->getInt("pos_track.bin_thr_2", 15)),
                    _rgb_mode(buffer->config_->getBool("pos_track.rgb_mode", true)),
                    _first_led_channel(getLEDChannel(buffer->config_->getString("pos_track.first_led_channel", "red"))),
                    _second_led_channel(getLEDChannel(buffer->config_->getString("pos_track.second_led_channel", "blue"))),
                    _detection_on(false)
{
}

PositionTrackingProcessor::~PositionTrackingProcessor()
{
    _detection_on = false;
    _detection_thread.join();
}

void PositionTrackingProcessor::process()
{
    if (!_detection_on)
    {
        _detection_on = true;
        _detection_thread = std::thread(&PositionTrackingProcessor::detect_positions, this);
        _detection_thread.detach();
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(10));

    {
        std::lock_guard _position_detector_guard(_position_detector_mutex);
        while (!_animal_positions.empty())
        {
            const auto& pos = _animal_positions.front();

            buffer->positions_buf_[buffer->pos_buf_pos_] = pos;
            buffer->AdvancePositionBufferPointer();
            // std::cout << pos << std::endl;

            _animal_positions.pop();
        }
    }
}

void PositionTrackingProcessor::detect_positions()
{
    const auto frame_type = _rgb_mode ? CV_8UC3 : CV_8UC1;

    while(_detection_on)
    {
        const auto raw_frame = _camera.captureFrame(_rgb_mode);

        cv::Mat frame(raw_frame->size[1], raw_frame->size[0],  // height, width
                frame_type, raw_frame->image,
                raw_frame->stride);
        const int timestamp = buffer->has_last_sample_timestamp_.load() ? buffer->last_sample_timestamp_.load() : raw_frame->timestamp;
        // cv::imshow("frame", frame);
        const auto pos = detectPosition(frame, timestamp);
        // cv::waitKey(1);

        std::lock_guard<std::mutex> _position_detector_guard(_position_detector_mutex);
        _animal_positions.push(pos);
        REPORT_PERFORMANCE();
    }
}

SpatialInfo PositionTrackingProcessor::detectPosition(cv::Mat& frame, int timestamp)
{
    SpatialInfo pos;
    pos.timestamp_ = timestamp;
    if (_rgb_mode)
    {
        std::array<cv::Mat, 3> frame_channels;
        cv::split(frame, frame_channels);

        pos = detectPositionRGB(frame_channels[_first_led_channel], 
                             frame_channels[_second_led_channel], 
                             _binary_threshold_1, 
                             _binary_threshold_2, 
                             timestamp);
    }
    else
    {
        pos = detectPositionGreyscale(frame, _binary_threshold_1, timestamp);
        /*
        cv::Mat first_led_frame;
        cv::Mat second_led_frame;
        cv::threshold(frame, first_led_frame, _binary_threshold_1, 255, cv::THRESH_BINARY);
        // cv::threshold(frame, second_led_frame, _binary_threshold_2, 255, cv::THRESH_BINARY);
        cv::inRange(frame, cv::Scalar(_binary_threshold_2), cv::Scalar(1.5 * _binary_threshold_2), second_led_frame);
        pos = detectPosition(first_led_frame, 
                             second_led_frame, 
                             _binary_threshold_1, 
                             _binary_threshold_2, 
                             timestamp);
                             */
    }
    return pos;
}

#ifdef PROFILE_POS_TRACKING
void PositionTrackingProcessor::reportPerformance()
{
    const auto now = std::chrono::high_resolution_clock::now();
    const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - _old_time).count();
    _old_time = now;

    _time_diffs.push_back(diff);

    if (_perf_proc_counter > TEST_PROC_STEPS)
    {
        std::cout << "Processing time: " << std::endl <<
            " max: " << *std::max_element(_time_diffs.begin()+1, _time_diffs.end()) <<
            " min: " << *std::min_element(_time_diffs.begin()+1, _time_diffs.end()) <<
            " avg: " << std::accumulate(_time_diffs.begin()+1, _time_diffs.end(), 0.0) / double(_time_diffs.size()-1) << std::endl;

        exit(0);
    }

    ++_perf_proc_counter;
}
#endif

namespace
{
    const cv::Mat dilate_kernel = cv::getStructuringElement(cv::MORPH_RECT, {3, 3});
    const cv::Mat erode_kernel = cv::getStructuringElement(cv::MORPH_RECT, {3, 3});

    // auxiliary vars used for processing
    std::vector<cv::Point2f> centroids;
    std::vector<std::vector<cv::Point>> contours;
    const cv::Point2f unknown_pos(-1, -1);

    cv::Mat& preprocess(cv::Mat& frame, int bin_thr)
    {
        cv::threshold(frame, frame, bin_thr, 255, cv::THRESH_BINARY);
        cv::erode(frame, frame, erode_kernel);
        cv::dilate(frame, frame, dilate_kernel);
        return frame;
    }

    void findContours(std::vector<std::vector<cv::Point>>& contours, const cv::Mat& frame)
    {
        contours.clear();
        cv::findContours(frame, contours, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);
        std::sort(contours.begin(), contours.end(), 
                [](const auto& c1, const auto& c2) { return cv::contourArea(c1) > cv::contourArea(c2); });
                 // sort contours in descending order by size
    }

    void findCentroids(std::vector<cv::Point2f>& centroids, const std::vector<std::vector<cv::Point>>& contours)
    {
        centroids.clear();
        cv::Moments m;

        for (const auto& c: contours)
        {
            m = cv::moments(c, true);
            centroids.emplace_back(float(m.m10) / float(m.m00), float(m.m01) / float(m.m00));
        }
    }

    SpatialInfo getPositionFromCentroids(const cv::Point2f& centroid_first_led, const cv::Point2f& centroid_second_led, int timestamp)
    {
        SpatialInfo pos;
        pos.timestamp_ = timestamp;
        pos.Init(centroid_second_led.x, centroid_second_led.y, 
                 centroid_first_led.x, centroid_first_led.y, 
                 timestamp);
        pos.valid = true;

        return pos;
    }

    int getLEDChannel(std::string&& channel)
    {
        switch (std::tolower(channel[0]))
        { // because of BGR format in opencv
            case 'r':
                return 0;
            case 'g':
                return 1;
            case 'b':
                return 2;
            default:
                return -1;
        }
    }

    void getCentroids(std::vector<cv::Point2f>& centroids, cv::Mat& frame, int bin_thr)
    {
        preprocess(frame, bin_thr);
        findContours(contours, frame);
        findCentroids(centroids, contours);
    }

    SpatialInfo detectPositionRGB(cv::Mat& first_led_frame, cv::Mat& second_led_frame, int bin_thr_1, int bin_thr_2, int timestamp)
    {
        // cv::imshow("frame_1", first_led_frame);
        getCentroids(centroids, first_led_frame, bin_thr_1);
        // cv::imshow("preprocessed_frame_1", first_led_frame);
        const auto c1 = centroids.empty() ? unknown_pos : centroids[0];

        // cv::imshow("frame_1", second_led_frame);
        getCentroids(centroids, second_led_frame, bin_thr_2);
        // cv::imshow("preprocessed_frame_2", second_led_frame);
        const auto c2 = centroids.empty() ?  unknown_pos : centroids[0];

        return getPositionFromCentroids(c1, c2, timestamp);
    }

    SpatialInfo detectPositionGreyscale(cv::Mat& frame, int bin_thr, int timestamp)
    {
        getCentroids(centroids, frame, bin_thr);
        // cv::imshow("preprocessed_frame", frame);
        const auto c1 = centroids.empty() ? unknown_pos : centroids[0];
        const auto c2 = centroids.size() < 2 ?  unknown_pos : centroids[1];

        return getPositionFromCentroids(c1, c2, timestamp);
    }
}
