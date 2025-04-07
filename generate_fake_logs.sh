#!/bin/bash
# generate_fake_logs.sh - Enhanced fake log generator with CLI options.

usage() {
    echo "Usage: $0 [--output FILE] [--count N] [--delay MS] [--start-time \"YYYY-MM-DD HH:MM:SS\"] [--end-time \"YYYY-MM-DD HH:MM:SS\"] [--severity {ERROR|WARNING|INFO}] [--ipv6]"
    exit 1
}

# Default values
OUTPUT_LOG="./logs/fake_syslog.log"
COUNT=100
DELAY=50  # milliseconds
START_TIME=$(date +"%Y-%m-%d 00:00:00")
END_TIME=$(date +"%Y-%m-%d 23:59:59")
SEVERITY=""
IPV6=false

# Parse CLI arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output)
            OUTPUT_LOG="$2"
            shift 2 ;;
        --count)
            COUNT="$2"
            shift 2 ;;
        --delay)
            DELAY="$2"
            shift 2 ;;
        --start-time)
            START_TIME="$2"
            shift 2 ;;
        --end-time)
            END_TIME="$2"
            shift 2 ;;
        --severity)
            SEVERITY="$2"
            shift 2 ;;
        --ipv6)
            IPV6=true
            shift 1 ;;
        *) usage ;;
    esac
done

mkdir -p "$(dirname "$OUTPUT_LOG")"

# Function to generate IPv4 or IPv6 address.
rand_ip() {
    if [ "$IPV6" = true ]; then
        printf '%x:%x:%x:%x:%x:%x:%x:%x\n' $((RANDOM % 0xffff)) $((RANDOM % 0xffff)) $((RANDOM % 0xffff)) $((RANDOM % 0xffff)) $((RANDOM % 0xffff)) $((RANDOM % 0xffff)) $((RANDOM % 0xffff)) $((RANDOM % 0xffff))
    else
        echo "$((RANDOM % 256)).$((RANDOM % 256)).$((RANDOM % 256)).$((RANDOM % 256))"
    fi
}

# Convert delay from ms to seconds fraction.
delay_sec=$(echo "scale=2; $DELAY/1000" | bc)

for (( i=0; i<COUNT; i++ )); do
    TIMESTAMP=$(date +"%Y-%m-%d %H:%M:%S")
    RAND=$((RANDOM % 10))
    IP=$(rand_ip)
    case $RAND in
        [0-2])
            echo "$TIMESTAMP ERROR auth: Failed login from $IP" >> "$OUTPUT_LOG" ;;
        [3-4])
            echo "$TIMESTAMP ERROR app: Error: unexpected condition at module X" >> "$OUTPUT_LOG" ;;
        [5-6])
            echo "$TIMESTAMP WARNING security: Unauthorized access attempt from $IP" >> "$OUTPUT_LOG" ;;
        7)
            echo "$TIMESTAMP INFO network: Port scan detected from $IP" >> "$OUTPUT_LOG" ;;
        8)
            echo "$TIMESTAMP INFO auth: login burst from $IP" >> "$OUTPUT_LOG" ;;
        *)
            if [ -n "$SEVERITY" ]; then
                echo "$TIMESTAMP $SEVERITY system: Routine operation with severity $SEVERITY from $IP" >> "$OUTPUT_LOG"
            else
                echo "$TIMESTAMP INFO system: Routine operation completed successfully from $IP" >> "$OUTPUT_LOG"
            fi
            ;;
    esac
    sleep "$delay_sec"
done

echo "Generated $COUNT fake log entries in $OUTPUT_LOG"