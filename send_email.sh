#!/bin/bash
# send_email.sh
# This script sends a detailed email alert using swaks.
# Usage: ./send_email.sh -r recipient_email
# It expects the alert message to be piped into it.

# Parse recipient from command-line options.
while getopts "r:" opt; do
  case "$opt" in
    r) recipient="$OPTARG" ;;
    *)
      echo "Usage: $0 -r recipient_email"
      exit 1
      ;;
  esac
done

# Read the alert message from standard input.
alert_message=$(cat)

# Extra details can be appended to the alert message.
# For example, if logParser outputs a summary to stdout, you might capture that in variables
# (or have logParser output additional details to a file that you then read here).
# Below is an example of additional information you might include.
detailed_info="Detailed Alert Summary:
- Date/Time: $(date)
- Total lines processed: 100
- Matching lines: 35
- Failed login alerts: 1
- Error alerts: 0
- Unauthorized alerts: 0
- Port scan alerts: 0
- Burst alerts: 0
For more information, please refer to the logs or the monitoring dashboard."

# Combine the original alert message with the extra details.
email_body="${alert_message}

${detailed_info}"

# Send the email using swaks.
swaks --to "$recipient" \
      --from "alert@example.com" \
      --server smtp.gmail.com:587 \
      --auth LOGIN \
      --auth-user "test.java.153@gmail.com" \
      --auth-password "your_actual_app_password" \
      --header "Subject: Log Monitor Alert" \
      --body "$email_body"
