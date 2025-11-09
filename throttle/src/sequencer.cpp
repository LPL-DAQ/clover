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

constexpr uint64_t NSEC_PER_CONTROL_TICK = 1'000'000; // 1 ms

static constexpr int MAX_BREAKPOINTS = 20;

K_MUTEX_DEFINE(sequence_lock);
K_SEM_DEFINE(control_sem, 0, 1); // Taken by sequence thread and granted when last count iter is reached.

static int gap_millis;
static std::vector<float> breakpoints;

volatile int step_count = 0;
volatile int count_to = 0;
uint64_t start_clock = 0;

struct data_row {
    float time;
    float motor_pos;
    float pt203;
    float pt204;
    float ptf401;
};
static std::array<data_row, 4000> data_buffer;

static void step_control_loop(k_work *) {
    int next_millis = step_count + 1;

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

    uint64_t since_start = k_cycle_get_64() - start_clock;
    uint64_t ns_since_start = k_cyc_to_ns_floor64(since_start);
    data_buffer[next_millis] = data_row{
            .time = static_cast<float>(ns_since_start) / 1e9f, // to us lossy, then to sec
            .motor_pos = static_cast<float>(curr_degrees),
            .pt203 = static_cast<float>(readings.pt203),
            .pt204 = static_cast<float>(readings.pt204),
            .ptf401 = static_cast<float>(readings.ptf401)
    };

    // move to target
    throttle_valve_move(target);
}

K_WORK_DEFINE(control_loop, step_control_loop);

static void control_loop_schedule(k_timer *timer) {
    if (step_count >= count_to) {
        k_timer_stop(timer);
        k_sem_give(&control_sem); // Return control to start_trace
        return;
    }
    k_work_submit(&control_loop);
    step_count += 1;
}

K_TIMER_DEFINE(control_loop_schedule_timer, control_loop_schedule, nullptr);

int sequencer_start_trace(int sock) {
    if (breakpoints.empty()) {
        LOG_ERR("No breakpoints specified.");
        return 1;
    }
    if (breakpoints.size() > MAX_BREAKPOINTS) {
        LOG_ERR("Too many breakpoints: %u", breakpoints.size());
        return 1;
    }
    if (gap_millis < 1) {
        LOG_ERR("Breakpoint gap_millis is too short: %d ms", gap_millis);
    }

    // Replace first breakpoint with current position
    breakpoints.front() = throttle_valve_get_pos();
    LOG_INF("Got breakpoints:");
    for (int i = 0; i < std::ssize(breakpoints); ++i) {
        LOG_INF("t=%d ms, bp=%f", i * gap_millis, static_cast<double>(breakpoints[i]));
    }

    k_mutex_lock(&sequence_lock, K_NO_WAIT);

    step_count = 0;
    count_to = std::min((std::ssize(breakpoints) - 1) * gap_millis, 4000);

    start_clock = k_cycle_get_64();

    // Start control iterations
    k_timer_start(&control_loop_schedule_timer, K_NO_WAIT, K_NSEC(NSEC_PER_CONTROL_TICK));

    // Await control iters, sem is granted by final tick of control timer.
    k_sem_take(&control_sem, K_FOREVER);

    // Halt valve
    throttle_valve_stop();

    // Print header
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

    // Print all datapoints
    for (int i = 0; i < count_to; ++i) {
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

int sequencer_prepare(int gap, std::vector<float> bps) {
    if (gap <= 0 || bps.empty()) {
        return 1;
    }

    gap_millis = gap;
    breakpoints = bps;
    return 0;
}
