# 🔍 Log Monitoring and Alert System

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://example.com)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Made With C+Bash](https://img.shields.io/badge/made%20with-C%20%2B%20Bash-blue.svg)](https://example.com)

## Overview

The **Log Monitoring and Alert System** is a flexible, high-performance tool designed for DevOps engineers, sysadmins, and SREs. It continuously scans log files to detect anomalies and security events, triggering alerts via various output formats and integrated email notifications.  
 
Developed primarily in **C** and **Bash**, the system leverages the efficiency and low-overhead of C for high-speed log parsing while utilizing Bash for orchestration, automation, and external integrations such as email alerting. This dual-language approach ensures that both real-time and scheduled log monitoring environments are catered for, making it ideal for both production environments and testing scenarios.


## Dependencies
- GCC (tested with gcc 9+)
- `swaks` (for email sending)
- Bash 4.0+
- cron (optional, for scheduled runs)

## Table of Contents
- [Features](#features)
- [Architecture Diagram](#architecture-diagram)
- [Configuration Files](#configuration-files)
- [Script and Binary Documentation](#script-and-binary-documentation)
- [Usage Examples](#usage-examples)
- [Setup, Compilation, and Makefile Instructions](#setup-compilation-and-makefile-instructions)
- [Troubleshooting & Debugging](#troubleshooting--debugging)
- [Contribution, License, and Contact](#contribution-license-and-contact)

## Features

- **Real-time Log Monitoring:** Constantly scans log files and processes new entries as they are appended.
- **Configurable Alert Rules:** Use simple INI and JSON configuration files to set thresholds, deduplication periods, and alert criteria.
- **Dual-format Alert Output:** Simultaneously outputs alerts in human-readable text and structured JSON format.
- **Interactive Bash Menu:** Provides an automation interface via an interactive menu (monitor.sh) for on-demand operations.
- **Email Alerting:** Alerts can be sent via an external script (`send_email.sh`) using Gmail SMTP with TLS encryption.
- **Fake Log Generation:** Includes a script (`generate_fake_logs.sh`) for generating synthetic logs for testing.
- **Cron Integration:** Easily schedule the log parser using cron to run at defined intervals.
- **Log Rotation and Deduplication:** Supports log rotation and suppresses repeated alerts within a configurable deduplication interval.

## Architecture Diagram

Below is a simplified architecture diagram using Mermaid syntax:

```mermaid
flowchart TD
    A[Log Files] -->|Appended Logs| B[logParser.c]
    B --> C{Alert Rules}
    C -- Failed Login --> D[process_alert]
    C -- Unauthorized Access --> D
    C -- Error --> D
    D --> E[Output Files: alerts.txt / alerts.json]
    D --> F[External Commands]
    F --> G[send_email.sh]
    G --> H[Gmail SMTP]
    B --> I[monitor.sh (Interactive/Auto)]
    I --> A
```

## Configuration Files

### INI Configuration (`config.txt`)

```ini
# Log file path (supports wildcards if needed)
log_path=logs/fake_syslog.log

# Output alert files
alert_output_path=alerts.txt
alert_output_json=alerts.json

# External command for alerts (e.g., email script)
external_alert_command=./send_email.sh -r admin@example.com

# Alert thresholds
failed_login_threshold=3
error_repeat_threshold=5
port_scan_threshold=10
suspicious_time_threshold=7
burst_threshold=5
dedup_interval_seconds=60

# Enable or disable specific rules (1 to enable, 0 to disable)
enable_failed_login=1
enable_error=1
enable_unauth=1
enable_port_scan=1
enable_suspicious_time=1
enable_burst=1
```

### JSON Configuration (`config.json`)

```json
{
  "log_path": "logs/fake_syslog.log",
  "alert_output_txt": "alerts.txt",
  "alert_output_json": "alerts.json",
  "external_alert_command": "./send_email.sh -r admin@example.com",
  "failed_login_threshold": 3,
  "error_repeat_threshold": 5,
  "port_scan_threshold": 10,
  "suspicious_time_threshold": 7,
  "burst_threshold": 5,
  "dedup_interval_seconds": 60,
  "enable_failed_login": true,
  "enable_error": true,
  "enable_unauth": true,
  "enable_port_scan": true,
  "enable_suspicious_time": true,
  "enable_burst": true
}
```

**Explanation of Keys:**

- **log_path:** Path to the system log file or directory containing logs.
- **alert_output_path / alert_output_json:** Files where alert messages are stored in text and JSON formats.
- **external_alert_command:** Command to run when an alert is triggered. Use `-r` to specify the recipient email.
- **Threshold keys:** Numeric values determining when to trigger specific alerts.
- **enable_*:** Boolean flags to activate or deactivate alert rules.

## Script and Binary Documentation

### logParser.c

**Description:**  
A high-performance C program that scans log files and processes entries based on defined alert rules. Supports both file input and real-time monitoring.

**CLI Usage:**

```bash
./logParser config.txt logs/fake_syslog.log [options]
```

**Options:**

- `--keyword "pattern"`: Only process log lines containing the specified keyword.
- `--start "YYYY-MM-DD HH:MM:SS"`: Start time for filtering logs.
- `--end "YYYY-MM-DD HH:MM:SS"`: End time for filtering logs.
- `--level "LEVEL"`: Filter log lines by a specific severity level (e.g., ERROR, INFO).
- `--realtime`: Start in real-time monitoring mode.
- `--json-output`: Output alerts in JSON format.

**Examples:**

```bash
./logParser config.txt logs/fake_syslog.log --keyword "login" --level "ERROR"
```

### monitor.sh

**Description:**  
A Bash script that orchestrates the log monitoring system. Runs in auto mode (scheduled via cron) or interactive mode, offering a menu-driven interface.

**Modes:**

- **Auto Mode:** Execute non-interactively via cron.
- **Interactive Menu Mode:** Presents options for running alerts, refreshing logs, or manually triggering tests.

### generate_fake_logs.sh

**Description:**  
Generates synthetic log entries to simulate potential security or system events.

**CLI Flags:**

- `-n`: Number of fake log lines to generate.
- `-o`: Output file for generated logs.
- `--interval`: Time interval between log entries.

**Example:**

```bash
./generate_fake_logs.sh -n 500 -o logs/fake_syslog.log --interval 1
```

### send_email.sh

**Description:**  
Sends an alert email using Gmail SMTP via the swaks tool. Supports TLS encryption and allows customization of the recipient email.

**SMTP Settings:**

- **Gmail SMTP:** Uses smtp.gmail.com on port 587.
- **Credentials:** Uses the configured email and app password.
- **Recipient Logic:** Accepts the recipient email via the `-r` option or the `RECIPIENT` environment variable.

**Example:**

```bash
./send_email.sh -r admin@example.com "Suspicious login detected from auth"
```

## Usage Examples

**Sample Log Line:**

```
2025-04-07 06:25:30 ERROR auth: Failed login attempt detected
```

**Sample Alert Output (Text):**

```
[2025-04-07 06:28:42] CRITICAL: [auth] Multiple failed login attempts from auth (3 attempts)
```

**Sample Alert Output (JSON):**

```json
{
  "timestamp_utc": "2025-04-07 06:28:42",
  "timestamp_local": "2025-04-07 06:28:42",
  "severity": "CRITICAL",
  "source": "auth",
  "message": "Multiple failed login attempts from auth (3 attempts)"
}
```

**Real CLI Command for Log Parsing with Email Alerting:**

```bash
./logParser config.txt logs/fake_syslog.log --keyword "login" --level "ERROR"
```

**Cron Job Example:**

Add the following line to your crontab to run the parser every 5 minutes:

```cron
*/5 * * * * /path/to/logParser config.txt logs/fake_syslog.log --keyword "login" --level "ERROR" >> /var/log/logparser.log 2>&1
```

## Setup, Compilation, and Makefile Instructions

1. **Compiling the C Parser:**

   Ensure you have `gcc` installed. Simply run:

   ```bash
   make
   ```

   This command builds `logParser` from `logParser.c`.

2. **Folder Structure:**

   ```
   /project-root
   ├── config.txt
   ├── config.json
   ├── logParser.c
   ├── monitor.sh
   ├── generate_fake_logs.sh
   ├── send_email.sh
   ├── Makefile
   └── logs/
       └── fake_syslog.log
   ```

3. **File Permissions:**

   Ensure all shell scripts are executable:

   ```bash
   chmod +x monitor.sh generate_fake_logs.sh send_email.sh
   ```

## Troubleshooting & Debugging

- **Email Alerts Not Sending:**
  - Confirm that `swaks` is installed (`sudo apt install swaks`).
  - Verify your SMTP credentials and ensure "Less Secure App Access" is enabled in your Gmail account if required.
  
- **Compilation Issues:**
  - Ensure you are using the required macros (_XOPEN_SOURCE, _DEFAULT_SOURCE) to access functions like `usleep`.
  - Use `gcc -Wall -O2 -o logParser logParser.c` to compile, and fix any warnings accordingly.

- **Verbose Mode:**
  - Check log files (e.g., `/var/log/logparser.log` if using cron) for detailed error messages.

## Contribution, License, and Contact

Contributions, feedback, and bug reports are welcome! Please submit pull requests or open issues on our GitHub repository.

Licensed under the MIT License.  
Maintained by [Mohammad Thabet Hassan](https://github.com/MohammadThabetHassan) – feel free to reach out via [email](mailto:mohammad_thabet@hotmail.com).

---

### Sample Alert JSON

```json
{
  "timestamp_utc": "2025-04-07 06:28:42",
  "timestamp_local": "2025-04-07 06:28:42",
  "severity": "CRITICAL",
  "source": "auth",
  "message": "Multiple failed login attempts from auth (3 attempts)"
}
```
