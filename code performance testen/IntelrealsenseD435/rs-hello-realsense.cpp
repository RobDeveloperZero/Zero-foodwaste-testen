// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <iostream>             // for cout
#include <vector>
#include <algorithm>
#include <time.h>
#include <array>

#define MAX_RANGE_VL53L0X_IN_MM 1200

bool is_finished = false;
int seconds_for_starting_up = 60;
int seconds_to_measure = 300 + seconds_for_starting_up;
time_t start = time(0);
std::vector<float> measures;
std::array<int, MAX_RANGE_VL53L0X_IN_MM> counted_measures_each_mm;
// Measures that are above 120, out of scope of failed.
int unsuccesfull_measures = 0;

float calculate_SD() {
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < measures.size(); ++i) {
        sum += measures[i];
    }
    mean = sum / measures.size();
    for (i = 0; i < measures.size(); ++i) {
        SD += pow(measures[i] - mean, 2);
    }
    return sqrt(SD / measures.size());
}

float calculate_avarage() {
    float sum = 0;
    for (int i = 0; i < measures.size(); i++) {
        sum += measures[i];
    }
    return sum / measures.size();
}

void print_result() {

    std::cout << "Measure time of " << seconds_to_measure - seconds_for_starting_up << " seconds is done.";
    std::cout << "Amount measures that were taken : " << measures.size() << std::endl;
    std::cout << "Amount of failed measures : " << unsuccesfull_measures << std::endl;
    if (measures.size() > 0) {
        std::cout << "Avarage in meters: " << calculate_avarage() << std::endl;
        std::cout << "Standard deviation in meters: " << calculate_SD() << std::endl;
        std::sort(measures.begin(), measures.end());
        std::cout << "Median in meters: " << measures[(measures.size() - 1) / 2] << std::endl;
        std::cout << "Shortest measure in meters: " << measures[0] << std::endl;
        std::cout << "Longest measure in meters : " << measures[measures.size() - 1] << std::endl;
    }
    
    std::cout << "Other measurements taken: ";
    for (int i = 0; i < MAX_RANGE_VL53L0X_IN_MM; i++) {
        if (counted_measures_each_mm[i] != 0) {
            std::cout << counted_measures_each_mm[i] << " times a measure of " << i << " mm has been measured." << std::endl;
        }
    }

}

/* Determine one single measurement based on the avarage of the x, y pixels from the center of the depth resolution. */
float avarage_from_center(rs2::depth_frame& depth, const float width, const float height, int center_resolution_x, int center_resolution_y) {
    //store a sensor measurement.
    float current_avarage = 0;
    const int amount_of_measures = center_resolution_x * center_resolution_y;

    // Get all the measures in a resolution of 100 x 100 around the center.
    for (int y = 0; y < center_resolution_y; y++) {
        for (int x = 0; x < center_resolution_x; x++) {
            current_avarage += depth.get_distance(((width / 2) - (center_resolution_x / 2)) + x, ((height / 2) - (center_resolution_y / 2)) + y);
        }
    }
    return current_avarage / amount_of_measures;
}

/* Determine one single measurement based on the avarage of the x, y pixels from the center of the depth resolution. */
float median_from_center(rs2::depth_frame& depth, const float width, const float height, int center_resolution_x, int center_resolution_y) {
    //store a sensor measurement.
    std::vector<float> measures_from_center;
    // Get all the measures in a resolution of x x y around the center.
    for (int y = 0; y < center_resolution_y; y++) {
        for (int x = 0; x < center_resolution_x; x++) {
            measures_from_center.push_back(depth.get_distance(((width / 2) - (center_resolution_x / 2)) + x, ((height / 2) - 50) + (center_resolution_y / 2)));
        }
    }
    std::sort(measures_from_center.begin(), measures_from_center.end());
    return measures_from_center[(measures_from_center.size() - 1) / 2];
}

// Hello RealSense example demonstrates the basics of connecting to a RealSense device
// and taking advantage of depth data
int main(int argc, char* argv[]) try
{
    // Create a Pipeline - this serves as a top-level API for streaming and processing frames
    rs2::pipeline p;

    rs2::config cfg;

    cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 15);

    // Configure and start the pipeline
    p.start(cfg);
    
    while (true)
    {
        // Block program until frames arrive
        rs2::frameset frames = p.wait_for_frames();

        // Try to get a frame of a depth image
        rs2::depth_frame depth = frames.get_depth_frame();

        // Get the depth frame's dimensions
        float width = depth.get_width();
        float height = depth.get_height();

        // Check if startup time is over.
        bool is_startup_done = difftime(time(0), start) > seconds_for_starting_up;

        float depth_measurement = avarage_from_center(depth, width, height, 100, 100);

        if (is_startup_done && !is_finished) {
            bool is_time_up = difftime(time(0), start) > seconds_to_measure;

            if (is_time_up) {
                is_finished = true;
                print_result();
            }
            else {
                if (depth_measurement < 1.2) {
                    std::cout << "[" << measures.size() << "] : " << depth_measurement * 1000 << std::endl;
                    //" meters. Time left : " << (seconds_to_measure - difftime(time(0), start)) << " \r";
                    measures.push_back(depth_measurement);
                    // Index represented the distance in mm, the value is the counted measurement of the distance in mm.
                    counted_measures_each_mm[static_cast<int>(depth_measurement * 1000)] += 1;
                }
                else {
                    std::cout << "[" << measures.size() << "] : " << depth_measurement * 1000 << std::endl;
                    unsuccesfull_measures++;
                }
            }
        }
        else {
            if (!is_startup_done) {
                std::cout << "Seconds before starting: " << seconds_for_starting_up - difftime(time(0), start) << std::endl;
                std::cout << "Current avarage measure right now " << depth_measurement << std::endl;
            }
        }

    }

    return EXIT_SUCCESS;
}
catch (const rs2::error& e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    unsuccesfull_measures++;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}