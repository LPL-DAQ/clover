//
// Created by lpl on 10/27/25.
//

#include "sequencer.h"
#include "throttle_valve.h"
#include "pts.h"
#include <vector>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <array>
#include <string>
#include <zephyr/net/socket.h>
#include <sstream>

LOG_MODULE_REGISTER(sequencer, CONFIG_LOG_DEFAULT_LEVEL);

static constexpr int MAX_BREAKPOINTS = 20;

K_MUTEX_DEFINE(sequence_lock);
static int gap_millis;
static std::vector<float> breakpoints;
static int end_millis;

struct data_row {
    float time;
    float motor_pos;
    float pt203;
    float pt204;
    float ptf401;
};
static std::array<data_row, 4000> data_buffer;

int sequencer_start_trace(int sock, int gap, std::vector<float> bps) {
    if (bps.size() > MAX_BREAKPOINTS) {
        LOG_ERR("Too many breakpoints: %u", bps.size());
        return 1;
    }
    if (gap < 1) {
        LOG_ERR("Breakpoint gap is too short: %d ms", gap);
    }

    LOG_INF("Got breakpoints:");
    for (int i = 0; i < std::ssize(bps); ++i) {
        LOG_INF("t=%d ms, bp=%f", i * gap, static_cast<float>(bps[i]));
    }

//    int err = k_mutex_lock(&sequence_lock, K_NO_WAIT);
//    if(err)
//        if(err == -EBUSY) {
//            LOG_WRN("Tried starting trace, but one was already in progress.");
//        }
//        else {
//            LOG_ERR("Unexpected error while locking sequence_lock: err %d", err);
//        }
//        return err;
//    }
    end_millis = (std::ssize(bps) - 1) * gap;
    gap_millis = gap;
    breakpoints = bps;

    uint64_t start_time = k_cycle_get_64();
    for (int next_millis = 0; next_millis < end_millis; ++next_millis) {
        int low_bp_index = MIN(next_millis / gap_millis, std::ssize(breakpoints) - 1);
        int high_bp_index = low_bp_index + 1;
        if (low_bp_index < 0) {
            LOG_WRN("Low index is less than 0, how is that possible? curr: %d, gap: %d", next_millis, gap_millis);
            low_bp_index = 0;
        }
        float target;
        if (high_bp_index >= std::ssize(breakpoints)) {
            target = breakpoints.back();
        } else {
            float tween = static_cast<float>(next_millis - (low_bp_index * gap_millis)) / gap_millis;
            target = breakpoints[low_bp_index] + (breakpoints[high_bp_index] - breakpoints[low_bp_index]) * tween;
        }

        float curr_degrees = throttle_valve_get_pos();
        pt_readings readings = pts_sample();

        uint64_t since_start = k_cycle_get_64() - start_time;
        uint64_t ns_since_start = k_cyc_to_ns_floor64(since_start);
        data_buffer[next_millis] = data_row{
                .time = static_cast<float>(ns_since_start) / 1e9f, // to us lossy, then to sec
                .motor_pos = static_cast<float>(curr_degrees),
                .pt203 = static_cast<float>(readings.pt203),
                .pt204 = static_cast<float>(readings.pt204),
                .ptf401 = static_cast<float>(readings.ptf401)
        };


        // move to target
        throttle_valve_move(target, 0.001);
        k_sleep(K_MSEC(1));
    }
    throttle_valve_stop();

    {
        std::string line = "time,valve_pos,pt203,pt204,ptf401\n";
        int bytes_sent = 0;
        while (bytes_sent < std::ssize(line)) {
            int ret = zsock_send(sock, line.c_str() + bytes_sent, line.size() - bytes_sent, 0);
            if (ret < 0) {
                LOG_ERR("Failed to dump data: err %d", ret);
                return ret;
            }
            bytes_sent += ret;
        }
    }

    for (int i = 0; i < end_millis; ++i) {
        std::stringstream ss;
        ss.precision(8);
        ss << std::fixed << data_buffer[i].time << ',' << data_buffer[i].motor_pos << ',' << data_buffer[i].pt203 << ','
           << data_buffer[i].pt204
           << ',' << data_buffer[i].ptf401 << '\n';
        std::string line = ss.str();
        int bytes_sent = 0;
        while (bytes_sent < std::ssize(line)) {
            int ret = zsock_send(sock, line.c_str() + bytes_sent, line.size() - bytes_sent, 0);
            if (ret < 0) {
                LOG_ERR("Failed to dump data: err %d", ret);
                return ret;
            }
            bytes_sent += ret;
        }
    }

    return 0;
}
