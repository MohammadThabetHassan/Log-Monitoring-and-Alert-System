#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_LINE 2048

// Alert severity levels.
typedef enum { SEV_INFO, SEV_WARNING, SEV_CRITICAL, SEV_ALERT } Severity;

// Structure for an alert record.
typedef struct {
    char timestamp_utc[64];
    char timestamp_local[64];
    Severity severity;
    char source[64];
    char message[512];
} AlertRecord;

// Structure for configuration.
typedef struct {
    char log_path[256];            // can be a file or wildcard (for now, a single file)
    char alert_output_txt[256];    // text alerts file
    char alert_output_json[256];   // JSON alerts file
    char external_command[256];    // optional external command (e.g., send_email.sh)
    int failed_login_threshold;
    int error_repeat_threshold;
    int port_scan_threshold;       // e.g., number of distinct ports in period
    int suspicious_time_threshold; // for login outside work hours (e.g., before 7 or after 19)
    int burst_threshold;           // sudden bursts (number)
    int dedup_interval;            // seconds to suppress duplicates
    bool enable_failed_login;
    bool enable_error;
    bool enable_unauth;
    bool enable_port_scan;
    bool enable_suspicious_time;
    bool enable_burst;
} Config;

// Filtering/runtime options.
typedef struct {
    char keyword[64];
    struct tm start_time;
    struct tm end_time;
    char level[16];
    bool start_time_set;
    bool end_time_set;
    bool level_set;
    bool realtime;      // --realtime option
    bool json_output;   // --json-output option
} FilterOptions;

// Alert history for deduplication per type.
typedef struct {
    time_t last_failed;
    time_t last_error;
    time_t last_unauth;
    time_t last_port;
    time_t last_burst;
} AlertHistory;

// Global summary counters.
typedef struct {
    int total_lines;
    int matching_lines;
    int alerts_failed;
    int alerts_error;
    int alerts_unauth;
    int alerts_port;
    int alerts_burst;
} Summary;

//
// Utility: Get current timestamps (UTC and local) in provided buffers.
void get_timestamps(char *utc_buf, size_t utc_size, char *local_buf, size_t local_size) {
    time_t now = time(NULL);
    struct tm *tm_utc = gmtime(&now);
    struct tm *tm_local = localtime(&now);
    strftime(utc_buf, utc_size, "%Y-%m-%d %H:%M:%S", tm_utc);
    strftime(local_buf, local_size, "%Y-%m-%d %H:%M:%S", tm_local);
}

//
// Utility: Write an alert to both text and JSON files.
void write_alert(const Config *config, const AlertRecord *alert) {
    FILE *ftxt = fopen(config->alert_output_txt, "a");
    if (ftxt) {
        fprintf(ftxt, "[%s] %s: [%s] %s\n", alert->timestamp_local,
                (alert->severity == SEV_CRITICAL) ? "CRITICAL" :
                (alert->severity == SEV_WARNING) ? "WARNING" : "ALERT",
                alert->source, alert->message);
        fclose(ftxt);
    }
    FILE *fjson = fopen(config->alert_output_json, "a");
    if (fjson) {
        fprintf(fjson,
                "{\"timestamp_utc\":\"%s\", \"timestamp_local\":\"%s\", \"severity\":\"%s\", \"source\":\"%s\", \"message\":\"%s\"}\n",
                alert->timestamp_utc,
                alert->timestamp_local,
                (alert->severity == SEV_CRITICAL) ? "CRITICAL" :
                (alert->severity == SEV_WARNING) ? "WARNING" : "ALERT",
                alert->source,
                alert->message);
        fclose(fjson);
    }
    // Trigger external command if configured.
    if (strlen(config->external_command) > 0) {
        char command[1024];
        snprintf(command, sizeof(command), "%s \"%s\"", config->external_command, alert->message);
        int ret = system(command);
        (void) ret;
    }
}

//
// Log an alert if deduplication allows.
void process_alert(const Config *config, AlertHistory *history, Summary *sum, Severity sev, const char *source, const char *fmt, ...) {
    time_t now = time(NULL);
    bool send = true;
    if (sev == SEV_CRITICAL && (now - history->last_failed < config->dedup_interval)) send = false;
    if (sev == SEV_WARNING && (now - history->last_error < config->dedup_interval)) send = false;
    if (strcmp(source, "unauth") == 0 && (now - history->last_unauth < config->dedup_interval)) send = false;
    if (strcmp(source, "port") == 0 && (now - history->last_port < config->dedup_interval)) send = false;
    if (strcmp(source, "burst") == 0 && (now - history->last_burst < config->dedup_interval)) send = false;

    if (!send) return;

    AlertRecord alert;
    get_timestamps(alert.timestamp_utc, sizeof(alert.timestamp_utc), alert.timestamp_local, sizeof(alert.timestamp_local));
    alert.severity = sev;
    strncpy(alert.source, source, sizeof(alert.source) - 1);

    va_list args;
    va_start(args, fmt);
    vsnprintf(alert.message, sizeof(alert.message), fmt, args);
    va_end(args);

    write_alert(config, &alert);

    if (sev == SEV_CRITICAL) { history->last_failed = now; sum->alerts_failed++; }
    else if (sev == SEV_WARNING) { history->last_error = now; sum->alerts_error++; }
    else if (strcmp(source, "unauth") == 0) { history->last_unauth = now; sum->alerts_unauth++; }
    else if (strcmp(source, "port") == 0) { history->last_port = now; sum->alerts_port++; }
    else if (strcmp(source, "burst") == 0) { history->last_burst = now; sum->alerts_burst++; }
}

//
// Dummy function: Check whether the current time is suspicious (e.g., outside 7 AM to 7 PM).
bool is_suspicious_time() {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    return (local->tm_hour < 7 || local->tm_hour > 19);
}

//
// Parse a log line into parts: timestamp, level, source, and message.
// Expected format: "YYYY-MM-DD HH:MM:SS LEVEL source: message"
bool parse_log_line(const char *line, char *timestamp, char *level, char *source, char *message) {
    return (sscanf(line, "%63s %*s %15s %63[^:]: %511[^\n]", timestamp, level, source, message) >= 4);
}

//
// Check filtering conditions.
bool should_filter(const char *log_timestamp, const char *log_level, const char *message, const FilterOptions *filter) {
    if (strlen(filter->keyword) > 0 && strstr(message, filter->keyword) == NULL)
        return true;
    if (filter->level_set && strcasecmp(log_level, filter->level) != 0)
        return true;
    return false;
}

//
// Real-time monitoring mode: re-read file periodically.
void realtime_monitor(const Config *config, const FilterOptions *filter) {
    FILE *fp = fopen(config->log_path, "r");
    if (!fp) {
        perror("fopen realtime file");
        return;
    }
    fseek(fp, 0, SEEK_END);

    Summary sum = {0};
    AlertHistory history = {0};
    char line[MAX_LINE];
    while (1) {
        if (fgets(line, sizeof(line), fp) != NULL) {
            sum.total_lines++;
            char timestamp[64], level[16], source[64], message[512];
            if (!parse_log_line(line, timestamp, level, source, message))
                continue;
            if (filter != NULL && should_filter(timestamp, level, message, filter))
                continue;
            sum.matching_lines++;
            if (strstr(message, "Failed login") || strstr(message, "failed login")) {
                static int count = 0;
                count++;
                if (count >= config->failed_login_threshold) {
                    process_alert(config, &history, &sum, SEV_CRITICAL, "auth", "Multiple failed login attempts from %s (%d attempts)", source, count);
                    count = 0;
                }
            }
            if (strstr(message, "Error") || strstr(message, "error")) {
                static int count = 0;
                count++;
                if (count >= config->error_repeat_threshold) {
                    process_alert(config, &history, &sum, SEV_WARNING, "app", "Repeated errors from %s (%d errors)", source, count);
                    count = 0;
                }
            }
            if (strstr(message, "Unauthorized access") || strstr(message, "unauthorized")) {
                process_alert(config, &history, &sum, SEV_CRITICAL, "unauth", "Unauthorized access detected from %s", source);
            }
            if (strstr(message, "port") && strstr(message, "scan")) {
                process_alert(config, &history, &sum, SEV_WARNING, "port", "Port scanning activity from %s", source);
            }
            if (is_suspicious_time()) {
                process_alert(config, &history, &sum, SEV_WARNING, "auth", "Suspicious login time from %s", source);
            }
            if (strstr(message, "burst")) {
                process_alert(config, &history, &sum, SEV_CRITICAL, "burst", "Burst of logins detected from %s", source);
            }
        } else {
            usleep(500000);
        }
    }
    fclose(fp);
}

//
// Main processing function: read from file or stdin.
void process_log(const char *input_path, const Config *config, const FilterOptions *filter) {
    FILE *fp = NULL;
    if (strcmp(input_path, "-") == 0) {
        fp = stdin;
    } else {
        fp = fopen(input_path, "r");
        if (!fp) {
            perror("fopen log file");
            return;
        }
    }

    Summary sum = {0};
    AlertHistory history = {0};
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) {
        sum.total_lines++;
        char timestamp[64], level[16], source[64], message[512];
        if (!parse_log_line(line, timestamp, level, source, message))
            continue;
        if (filter != NULL && should_filter(timestamp, level, message, filter))
            continue;
        sum.matching_lines++;
        if (strstr(message, "Failed login") || strstr(message, "failed login")) {
            static int count = 0;
            count++;
            if (count >= config->failed_login_threshold) {
                process_alert(config, &history, &sum, SEV_CRITICAL, "auth", "Multiple failed login attempts from %s (%d attempts)", source, count);
                count = 0;
            }
        }
        if (strstr(message, "Error") || strstr(message, "error")) {
            static int count = 0;
            count++;
            if (count >= config->error_repeat_threshold) {
                process_alert(config, &history, &sum, SEV_WARNING, "app", "Repeated errors from %s (%d errors)", source, count);
                count = 0;
            }
        }
        if (strstr(message, "Unauthorized access") || strstr(message, "unauthorized")) {
            process_alert(config, &history, &sum, SEV_CRITICAL, "unauth", "Unauthorized access detected from %s", source);
        }
        if (strstr(message, "port") && strstr(message, "scan")) {
            process_alert(config, &history, &sum, SEV_WARNING, "port", "Port scanning activity detected from %s", source);
        }
        if (is_suspicious_time()) {
            process_alert(config, &history, &sum, SEV_WARNING, "auth", "Suspicious login time from %s", source);
        }
        if (strstr(message, "burst")) {
            process_alert(config, &history, &sum, SEV_CRITICAL, "burst", "Burst of logins detected from %s", source);
        }
    }

    if (fp != stdin)
        fclose(fp);

    fprintf(stderr, "\nSummary:\n");
    fprintf(stderr, "  Total lines processed: %d\n", sum.total_lines);
    fprintf(stderr, "  Matching lines: %d\n", sum.matching_lines);
    fprintf(stderr, "  Failed login alerts: %d\n", sum.alerts_failed);
    fprintf(stderr, "  Error alerts: %d\n", sum.alerts_error);
    fprintf(stderr, "  Unauthorized alerts: %d\n", sum.alerts_unauth);
    fprintf(stderr, "  Port scan alerts: %d\n", sum.alerts_port);
    fprintf(stderr, "  Burst alerts: %d\n", sum.alerts_burst);
}

//
// Main entry point.
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <config-file> <log-file or -> [options]\n", argv[0]);
        fprintf(stderr, "Options: --keyword, --start, --end, --level, --realtime, --json-output\n");
        return EXIT_FAILURE;
    }

    Config config = {0};
    FILE *cf = fopen(argv[1], "r");
    if (!cf) {
        perror("fopen config file");
        return EXIT_FAILURE;
    }
    char cline[MAX_LINE];
    while (fgets(cline, sizeof(cline), cf)) {
        if (cline[0] == '#' || cline[0] == '[') continue;
        char key[128], value[256];
        if (sscanf(cline, "%127[^=]=%255[^\n]", key, value) == 2) {
            if (strcmp(key, "log_path") == 0)
                strncpy(config.log_path, value, sizeof(config.log_path));
            else if (strcmp(key, "alert_output_path") == 0)
                strncpy(config.alert_output_txt, value, sizeof(config.alert_output_txt));
            else if (strcmp(key, "alert_output_json") == 0)
                strncpy(config.alert_output_json, value, sizeof(config.alert_output_json));
            else if (strcmp(key, "external_alert_command") == 0)
                strncpy(config.external_command, value, sizeof(config.external_command));
            else if (strcmp(key, "failed_login_threshold") == 0)
                config.failed_login_threshold = atoi(value);
            else if (strcmp(key, "error_repeat_threshold") == 0)
                config.error_repeat_threshold = atoi(value);
            else if (strcmp(key, "port_scan_threshold") == 0)
                config.port_scan_threshold = atoi(value);
            else if (strcmp(key, "suspicious_time_threshold") == 0)
                config.suspicious_time_threshold = atoi(value);
            else if (strcmp(key, "burst_threshold") == 0)
                config.burst_threshold = atoi(value);
            else if (strcmp(key, "dedup_interval_seconds") == 0)
                config.dedup_interval = atoi(value);
            else if (strcmp(key, "enable_failed_login") == 0)
                config.enable_failed_login = (atoi(value) == 1);
            else if (strcmp(key, "enable_error") == 0)
                config.enable_error = (atoi(value) == 1);
            else if (strcmp(key, "enable_unauth") == 0)
                config.enable_unauth = (atoi(value) == 1);
            else if (strcmp(key, "enable_port_scan") == 0)
                config.enable_port_scan = (atoi(value) == 1);
            else if (strcmp(key, "enable_suspicious_time") == 0)
                config.enable_suspicious_time = (atoi(value) == 1);
            else if (strcmp(key, "enable_burst") == 0)
                config.enable_burst = (atoi(value) == 1);
        }
    }
    fclose(cf);

    if (strlen(config.alert_output_txt) == 0)
        strcpy(config.alert_output_txt, "alerts.txt");
    if (strlen(config.alert_output_json) == 0)
        strcpy(config.alert_output_json, "alerts.json");
    if (strlen(config.log_path) == 0)
        strcpy(config.log_path, "logs/fake_syslog.log");

    FilterOptions filter = {0};
    filter.realtime = false;
    filter.json_output = false;
    static struct option long_options[] = {
            {"keyword", required_argument, 0, 'k'},
            {"start", required_argument, 0, 's'},
            {"end", required_argument, 0, 'e'},
            {"level", required_argument, 0, 'l'},
            {"realtime", no_argument, 0, 'r'},
            {"json-output", no_argument, 0, 'j'},
            {0, 0, 0, 0}
    };
    int opt, option_index = 0;
    while ((opt = getopt_long(argc - 2, argv + 2, "k:s:e:l:rj", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'k':
                strncpy(filter.keyword, optarg, sizeof(filter.keyword) - 1);
                break;
            case 's':
                strptime(optarg, "%Y-%m-%d %H:%M:%S", &filter.start_time);
                filter.start_time_set = true;
                break;
            case 'e':
                strptime(optarg, "%Y-%m-%d %H:%M:%S", &filter.end_time);
                filter.end_time_set = true;
                break;
            case 'l':
                strncpy(filter.level, optarg, sizeof(filter.level) - 1);
                filter.level_set = true;
                break;
            case 'r':
                filter.realtime = true;
                break;
            case 'j':
                filter.json_output = true;
                break;
            default:
                break;
        }
    }

    if (filter.realtime)
        realtime_monitor(&config, &filter);
    else
        process_log(strcmp(argv[2], "-") == 0 ? "-" : argv[2], &config, &filter);

    return EXIT_SUCCESS;
}