#!/bin/bash
# A sample send_email.sh script that sends a more detailed email alert.

# Parse recipient from options (e.g. -r recipient_email)
while getopts "r:" opt; do
  case "$opt" in
    r) recipient="$OPTARG" ;;
  esac
done

# Read the alert message from standard input.
alert_message=$(cat)

# You can also add extra details to the email body.
additional_info="Alert generated on: $(date)
Detailed Alert Information:
- Check the log monitoring system for more insights.
- Review the attached logs if needed."

# Combine the alert message with the additional info.
email_body="$alert_message

$additional_info"

# Use swaks to send the email.
swaks --to "$recipient" \
      --from "alert@example.com" \
      --server smtp.gmail.com:587 \
      --auth LOGIN \
      --auth-user "test.java.153@gmail.com" \
      --auth-password "your_actual_app_password" \
      --header "Subject: Log Monitor Alert" \
      --body "$email_body"
