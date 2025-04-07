#!/bin/bash
# send_email.sh - Send an email alert via Gmail SMTP using swaks with TLS support.
#
# You can specify the recipient email address in one of the following ways:
# 1. Set the environment variable RECIPIENT before running this script.
#    Example: RECIPIENT="admin@example.com" ./send_email.sh "Alert message"
# 2. Use the -r option when calling this script:
#    Example: ./send_email.sh -r admin@example.com "Alert message"
#
# If no recipient is specified, it defaults to test.java.153@gmail.com.

# Default configuration.
FROM="test.java.153@gmail.com"
DEFAULT_RECIPIENT="test.java.153@gmail.com"  # Default recipient if not provided via env or option.
SUBJECT="Log Monitor Alert"
SMTP_SERVER="smtp.gmail.com"
SMTP_PORT="587"
USERNAME="test.java.153@gmail.com"
PASSWORD="dxjottaxlhpcpzqi"

# Parse options.
RECIPIENT=""
while getopts "r:" opt; do
  case "$opt" in
    r)
      RECIPIENT="$OPTARG"
      ;;
    *)
      ;;
  esac
done
shift $((OPTIND -1))

# If RECIPIENT is not provided via options, check environment variable.
if [ -z "$RECIPIENT" ]; then
    RECIPIENT="${RECIPIENT:-$DEFAULT_RECIPIENT}"
fi

if [ -z "$1" ]; then
    echo "No alert message provided."
    exit 1
fi

ALERT_MSG="$1"

# Using swaks with TLS.
swaks --to "$RECIPIENT" \
      --from "$FROM" \
      --server "$SMTP_SERVER" \
      --port "$SMTP_PORT" \
      --tls \
      --auth LOGIN \
      --auth-user "$USERNAME" \
      --auth-password "$PASSWORD" \
      --header "Subject: $SUBJECT" \
      --body "$ALERT_MSG"

if [ $? -eq 0 ]; then
    echo "Alert sent to $RECIPIENT"
else
    echo "Failed to send alert."
fi