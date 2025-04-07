#!/bin/bash
# monitor.sh - Enhanced automation script for log processing, rotation, and interactive menu

CONFIG_FILE="./config.txt"
LOG_DIR="./logs"
ARCHIVE_DIR="./archive"
ALERT_FILE="alerts.txt"
CRON_LOG="monitor_cron.log"

# Ensure required directories exist.
mkdir -p "$LOG_DIR" "$ARCHIVE_DIR"
touch "$ALERT_FILE" "$CRON_LOG"

archive_logs() {
    find "$LOG_DIR" -type f -mtime +1 -exec mv {} "$ARCHIVE_DIR" \;
}

rotate_alerts() {
    if [ -f "$ALERT_FILE" ]; then
        mv "$ALERT_FILE" "${ALERT_FILE}_$(date +%Y%m%d)"
        touch "$ALERT_FILE"
    fi
}

run_parser() {
    # Run the log parser. If it fails, retry up to 3 times.
    for i in {1..3}; do
        ./logParser "$CONFIG_FILE" "$LOG_DIR/fake_syslog.log" && break
        echo "Attempt $i failed, retrying..." >> "$CRON_LOG"
        sleep 2
    done
}

view_alerts() {
    echo "Latest alerts from $ALERT_FILE:"
    tail -n 10 "$ALERT_FILE"
}

generate_fake() {
    ./generate_fake_logs.sh
}

clean_logs() {
    echo "Cleaning and archiving logs..."
    archive_logs
    rotate_alerts
}

menu() {
    PS3="Select an option: "
    options=("Run Log Parser" "View Latest Alerts" "Clean/Archive Logs" "Generate Fake Logs" "Exit")
    select opt in "${options[@]}"; do
        case $REPLY in
            1) run_parser; break ;;
            2) view_alerts; break ;;
            3) clean_logs; break ;;
            4) generate_fake; break ;;
            5) exit 0 ;;
            *) echo "Invalid option";;
        esac
    done
}

# Log runtime and status.
echo "$(date +'%Y-%m-%d %H:%M:%S') - monitor.sh execution started" >> "$CRON_LOG"

# If run with no arguments, show menu; otherwise, run parser.
if [ "$1" == "--menu" ]; then
    menu
else
    run_parser
    archive_logs
    rotate_alerts
fi

echo "$(date +'%Y-%m-%d %H:%M:%S') - monitor.sh execution finished" >> "$CRON_LOG"