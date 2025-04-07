#!/bin/bash
# send_email.sh: Sends a detailed alert email using swaks.
# Usage: ./send_email.sh -r recipient_email < alert_message.txt

# Parse the recipient from command-line options.
while getopts "r:" opt; do
  case "$opt" in
    r) recipient="$OPTARG" ;;
    *)
      echo "Usage: $0 -r recipient_email"
      exit 1
      ;;
  esac
done

# Read the initial alert message from standard input.
alert_message=$(cat)

# Append additional detailed information.
detailed_info="Detailed Alert Summary:
- Date/Time: $(date)
- Total lines processed: 100
- Matching lines: 38
- Failed login alerts: 1
- Error alerts: 0
- Unauthorized alerts: 0
- Port scan alerts: 0
- Burst alerts: 0
For more details, please check the monitoring dashboard or review the log files."

# Combine the initial alert with the detailed summary.
email_body="${alert_message}

${detailed_info}"

# Send the email using swaks with TLS enabled.
swaks --to "$recipient" \
      --from "alert@example.com" \
      --server smtp.gmail.com:587 \
      --tls \
      --auth LOGIN \
      --auth-user "test.java.153@gmail.com" \
      --auth-password "your_actual_app_password" \
      --header "Subject: Log Monitor Alert" \
      --body "$email_body"
