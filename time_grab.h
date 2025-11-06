//
// Created by cheb2 on 11/1/2025.
//

#ifndef GM_CLOCK_TIME_GRAB_H
#define GM_CLOCK_TIME_GRAB_H

#include <cstdint>
#include <cstddef>
#include <ctime>


void init_ntp_client();

void poll_ntp();

time_t get_current_unix_time();

void get_current_time_str(char* buffer, size_t buffer_len);

bool is_time_synced();

void set_timezone_offset(int offset_seconds);

#endif //GM_CLOCK_TIME_GRAB_H