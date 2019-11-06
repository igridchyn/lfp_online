#include "PositionTrackingProcessor.h"
#include <iostream>
#include <filesystem>
#include <future>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>

namespace fs = std::filesystem;

constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
constexpr int DEFAULT_LEFT_POS = 0;
constexpr int DEFAULT_TOP_POS = 0;


namespace
{
    const cv::Mat dilate_kernel = cv::getStructuringElement(cv::MORPH_RECT, {5, 5});
    const cv::Mat erode_kernel = cv::getStructuringElement(cv::MORPH_RECT, {7, 7});

    cv::Mat& preprocess(cv::Mat& frame, int bin_thr);
    void find_contours(std::vector<std::vector<cv::Point>>& contours, std::vector<cv::Vec4i>& hierarchy, const cv::Mat& frame);
    void find_centroids(std::vector<cv::Point2f>& centroids, const std::vector<std::vector<cv::Point>>& contours);
    SpatialInfo get_position_from_centroids(const std::vector<cv::Point2f>& centroids, const unsigned long long timestamp);
}

#ifdef PROFILE_POS_TRACKING
constexpr int TEST_PROC_STEPS = 100;
#define REPORT_PERFORMANCE(num_detected) reportPerformance(num_detected)
#else
#define REPORT_PERFORMANCE
#endif

PositionTrackingProcessor::PositionTrackingProcessor(LFPBuffer *buf):
                    LFPProcessor(buf),
                    _camera(buffer->config_->getInt("pos_track.im_width", DEFAULT_WIDTH), 
                            buffer->config_->getInt("pos_track.im_height", DEFAULT_HEIGHT), 
                            buffer->config_->getInt("pos_track.im_pos_left", DEFAULT_LEFT_POS), 
                            buffer->config_->getInt("pos_track.im_pos_top", DEFAULT_TOP_POS)),
                    _binary_threshold(buffer->config_->getInt("pos_track.bin_thr", 20))
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

            memcpy(reinterpret_cast<void*>(buffer->positions_buf_ + buffer->pos_buf_pos_), &pos, sizeof(SpatialInfo));
            buffer->AdvancePositionBufferPointer();
            std::cout << pos << std::endl;

            _animal_positions.pop();
        }
    }
}

void PositionTrackingProcessor::detect_positions()
{
    std::vector<cv::Point2f> centroids;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    while(_detection_on)
    {
        centroids.clear();
        const auto raw_frame = _camera.captureFrame();

        cv::Mat frame(raw_frame->size[1], raw_frame->size[0],  // height, width
                      CV_8U, raw_frame->image,
                      raw_frame->stride);

        preprocess(frame, _binary_threshold);
        find_contours(contours, hierarchy, frame);
        find_centroids(centroids, contours);
        const auto pos = get_position_from_centroids(centroids, raw_frame->timestamp);

        // std::cout << pos << std::endl;
        // cv::imshow("Live", frame);
        // cv::waitKey(1);

        REPORT_PERFORMANCE(contours.size());
        std::lock_guard<std::mutex> _position_detector_guard(_position_detector_mutex);
        _animal_positions.push(pos);
    }
}

#ifdef PROFILE_POS_TRACKING
void PositionTrackingProcessor::reportPerformance(unsigned int detected_spots)
{
    const auto now = std::chrono::high_resolution_clock::now();
    const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - _old_time).count();
    _old_time = now;

    _num_spots_detected.push_back(detected_spots);
    _time_diffs.push_back(diff);

    if (_perf_proc_counter > TEST_PROC_STEPS)
    {
        std::cout << "Processing time: " << std::endl <<
            " max: " << *std::max_element(_time_diffs.begin()+1, _time_diffs.end()) <<
            " min: " << *std::min_element(_time_diffs.begin()+1, _time_diffs.end()) <<
            " avg: " << std::accumulate(_time_diffs.begin()+1, _time_diffs.end(), 0.0) / double(_time_diffs.size()-1) << std::endl;

        std::cout << "Spots detected: " << std::endl <<
            " max: " << *std::max_element(_num_spots_detected.begin(), _num_spots_detected.end()) << 
            " min: " << *std::min_element(_num_spots_detected.begin(), _num_spots_detected.end()) << 
            " avg: " << std::accumulate(_num_spots_detected.begin(), _num_spots_detected.end(), 0) / double(_num_spots_detected.size()) << std::endl;

        exit(0);
    }

    ++_perf_proc_counter;
}
#endif

namespace
{
    cv::Mat& preprocess(cv::Mat& frame, int bin_thr)
    {
        cv::threshold(frame, frame, bin_thr, 255, cv::THRESH_BINARY);
        cv::dilate(frame, frame, dilate_kernel);
        cv::erode(frame, frame, erode_kernel);
        cv::dilate(frame, frame, dilate_kernel);
        cv::erode(frame, frame, erode_kernel);
        return frame;
    }

    void find_contours(std::vector<std::vector<cv::Point>>& contours, std::vector<cv::Vec4i>& hierarchy, const cv::Mat& frame)
    {
        contours.clear();
        hierarchy.clear();
        cv::findContours(frame, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);
        std::sort(contours.begin(), contours.end(), 
                [](const auto& c1, const auto& c2) { return c1.size() > c2.size(); });
                 // sort contours in descending order by size
    }

    void find_centroids(std::vector<cv::Point2f>& centroids, const std::vector<std::vector<cv::Point>>& contours)
    {
        centroids.clear();
        cv::Moments m;

        for (const auto& c: contours)
        {
            m = cv::moments(c, true);
            centroids.emplace_back(float(m.m10) / float(m.m00), float(m.m01) / float(m.m00));
        }
    }

    SpatialInfo get_position_from_centroids(const std::vector<cv::Point2f>& centroids, const unsigned long long timestamp)
    {
        SpatialInfo pos;
        pos.timestamp_ = timestamp;

        if (!centroids.empty())
        {
            const bool two_spots = centroids.size() >= 2;

            const auto& big_x = centroids[0].x;
            const auto& big_y = centroids[0].y;
            const auto& small_x = two_spots ? centroids[1].x : -1.;
            const auto& small_y = two_spots ? centroids[1].y : -1.;

            pos.Init(big_x, big_y, small_x, small_y, timestamp);
            pos.valid = true;
        }

        return pos;
    }
}
