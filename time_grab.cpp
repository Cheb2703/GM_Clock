//
// Created by cheb2 on 11/1/2025.
//

#include "time_grab.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/sntp.h"
#include "hardware/powman.h"
#include <cstdio>
#include <ctime>
#include "pico/util/datetime.h"
#include "pico/time.h"
#include <cstring>
#include <lwip/udp.h>
#include <lwip/ip_addr.h>

#include "lwip/udp.h"
#include "lwip/dns.h"

#define NTP_SERVER "pool.ntp.org"
#define NTP_PORT 123
#define NTP_PKT_LEN 48

static constexpr uint32_t NTP_TO_UNIX_OFFSET = 2208988800UL;

static ip_addr_t ntp_server_ip;
static bool dns_resolved = false;
static struct udp_pcb* udp_client = nullptr;
static absolute_time_t last_ntp_sent = {0};
static bool have_synced = false;

// Store last NTP sync time
static time_t base_unix_time = 0;
static absolute_time_t last_sync_time = {0};

// Base timezone offset (in seconds) from UTC (standard time, no DST)
static int base_timezone_offset_seconds = 0;
static char timezone_abbr_std[8] = "STD";
static char timezone_abbr_dst[8] = "DST";

// ----------------- Helpers -----------------
static void print_ip(const ip_addr_t* a) {
    const char* s = ipaddr_ntoa(a);
    printf("%s\n", s ? s : "unknown");
}

static void dns_callback(const char* name, const ip_addr_t* addr, void* arg) {
    if (addr) {
        ip_addr_copy(ntp_server_ip, *addr);
        dns_resolved = true;
        printf("DNS resolved %s -> ", name);
        print_ip(&ntp_server_ip);
    } else {
        printf("DNS resolution failed for %s\n", name);
    }
}

// ----------------- DST Calculation -----------------
static int nth_sunday_of_month(int year, int month, int nth) {
    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = 1;

    time_t first_day = mktime(&t);
    struct tm* first_tm = gmtime(&first_day);
    int wday = first_tm->tm_wday; // 0=Sun, 6=Sat

    return 1 + ((7 - wday) % 7) + (nth - 1) * 7;
}

static bool is_dst_active(time_t utc_time) {
    struct tm tm_utc = {};
    gmtime_r(&utc_time, &tm_utc);
    int year = tm_utc.tm_year + 1900;

    // DST start: 2nd Sunday in March, 2:00 AM local
    int dst_start_day = nth_sunday_of_month(year, 3, 2);
    struct tm dst_start = {};
    dst_start.tm_year = year - 1900;
    dst_start.tm_mon = 2; // March
    dst_start.tm_mday = dst_start_day;
    dst_start.tm_hour = 2;
    time_t dst_start_utc = mktime(&dst_start) - base_timezone_offset_seconds;

    // DST end: 1st Sunday in November, 2:00 AM local
    int dst_end_day = nth_sunday_of_month(year, 11, 1);
    struct tm dst_end = {};
    dst_end.tm_year = year - 1900;
    dst_end.tm_mon = 10; // November
    dst_end.tm_mday = dst_end_day;
    dst_end.tm_hour = 2;
    time_t dst_end_utc = mktime(&dst_end) - (base_timezone_offset_seconds + 3600);

    return (utc_time >= dst_start_utc && utc_time < dst_end_utc);
}

// ----------------- NTP receive -----------------
static void ntp_recv_cb(void* arg, struct udp_pcb* pcb, struct pbuf* p,
                        const ip_addr_t* addr, u16_t port) {
    if (!p) return;
    if (p->tot_len < NTP_PKT_LEN) { pbuf_free(p); return; }

    auto payload = static_cast<uint8_t*>(p->payload);
    auto tx_secs = (static_cast<uint32_t>(payload[40]) << 24) |
                   (static_cast<uint32_t>(payload[41]) << 16) |
                   (static_cast<uint32_t>(payload[42]) << 8)  |
                   static_cast<uint32_t>(payload[43]);

    if (tx_secs == 0) { pbuf_free(p); return; }

    base_unix_time = tx_secs - NTP_TO_UNIX_OFFSET;
    last_sync_time = get_absolute_time();
    have_synced = true;

    printf("NTP reply received: unix=%lu\n", static_cast<unsigned long>(base_unix_time));
    pbuf_free(p);
}

// ----------------- Send NTP -----------------
static void send_ntp_request_internal() {
    if (!dns_resolved) return; // NOLINT

    if (!udp_client) {
        udp_client = udp_new();
        if (!udp_client) {
            printf("Failed to create UDP PCB\n");
            return;
        }
        udp_recv(udp_client, ntp_recv_cb, nullptr);
    }

    uint8_t pkt[NTP_PKT_LEN] = {0};
    pkt[0] = 0b11100011; // LI=3, Version=4, Mode=3

    struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, NTP_PKT_LEN, PBUF_RAM);
    if (!p) { printf("pbuf_alloc failed\n"); return; }

    memcpy(p->payload, pkt, NTP_PKT_LEN);

    err_t err = udp_sendto(udp_client, p, &ntp_server_ip, NTP_PORT);
    if (err != ERR_OK) {
        printf("udp_sendto failed: %d\n", err);
    } else {
        last_ntp_sent = get_absolute_time();
        printf("NTP request sent to ");
        print_ip(&ntp_server_ip);
    }

    pbuf_free(p);
}

// ----------------- Public API -----------------
void init_ntp_client() {
    dns_resolved = false;
    have_synced = false;
    base_unix_time = 0;
    last_sync_time = {0};

    if (udp_client) {
        udp_remove(udp_client);
        udp_client = nullptr;
    }

    ip_addr_t tmp;
    err_t rc = dns_gethostbyname(NTP_SERVER, &tmp, dns_callback, nullptr);
    if (rc == ERR_OK) {
        ip_addr_copy(ntp_server_ip, tmp);
        dns_resolved = true;
        printf("NTP server cached: ");
        print_ip(&ntp_server_ip);
    } else if (rc == ERR_INPROGRESS) {
        printf("Resolving NTP server %s...\n", NTP_SERVER);
    } else {
        printf("dns_gethostbyname error: %d\n", rc);
    }
}

void poll_ntp() {
    cyw43_arch_poll();
    if (!dns_resolved) return; // NOLINT

    uint64_t interval_us = have_synced ? (10 * 60 * 1000000ULL) : (5 * 1000000ULL);
    if (absolute_time_diff_us(last_ntp_sent, get_absolute_time()) > interval_us) {
        send_ntp_request_internal();
    }
}

time_t get_current_unix_time() {
    if (!have_synced) return 0;

    int64_t elapsed_us = absolute_time_diff_us(last_sync_time, get_absolute_time());
    return base_unix_time + static_cast<time_t>(elapsed_us / 1000000ULL);
}

void get_current_time_str(char* buffer, size_t buffer_len) {
    time_t t = get_current_unix_time();
    if (t == 0) {
        snprintf(buffer, buffer_len, "--:--:--");
        return;
    }

    bool dst = is_dst_active(t);
    int total_offset = base_timezone_offset_seconds + (dst ? 3600 : 0);
    const char* tz_abbr = dst ? timezone_abbr_dst : timezone_abbr_std;

    t += total_offset;

    struct tm tmv = {};
    if (gmtime_r(&t, &tmv) == nullptr) {
        snprintf(buffer, buffer_len, "invalid");
        return;
    }

    char timestr[32];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &tmv);
    snprintf(buffer, buffer_len, "%s %s", timestr, tz_abbr);
}

bool is_time_synced() {
    return have_synced;
}

void set_timezone_offset(int offset_seconds) {
    base_timezone_offset_seconds = offset_seconds;
    printf("Base timezone offset set to %+d seconds (%.1f hours)\n",
           offset_seconds, offset_seconds / 3600.0);

    if (offset_seconds == -8 * 3600) { strcpy(timezone_abbr_std, "PST"); strcpy(timezone_abbr_dst, "PDT"); }
    else if (offset_seconds == -7 * 3600) { strcpy(timezone_abbr_std, "MST"); strcpy(timezone_abbr_dst, "MDT"); }
    else if (offset_seconds == -6 * 3600) { strcpy(timezone_abbr_std, "CST"); strcpy(timezone_abbr_dst, "CDT"); }
    else if (offset_seconds == -5 * 3600) { strcpy(timezone_abbr_std, "EST"); strcpy(timezone_abbr_dst, "EDT"); }
    else { strcpy(timezone_abbr_std, "STD"); strcpy(timezone_abbr_dst, "DST"); }
}