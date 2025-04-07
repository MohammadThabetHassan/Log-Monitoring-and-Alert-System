#!/bin/bash
# send_email.sh: Sends a detailed alert email using swaks.
# Usage: ./send_email.sh -r recipient_email -f error_log.txt
# The script calls the Geminai API to generate detailed information about an error.

# Parse command-line options.
while getopts "r:f:" opt; do
  case "$opt" in
    r) recipient="$OPTARG" ;;
    f) error_file="$OPTARG" ;;
    *)
      echo "Usage: $0 -r recipient_email -f error_log.txt"
      exit 1
      ;;
  esac
done

if [ -z "$recipient" ] || [ -z "$error_file" ]; then
  echo "Usage: $0 -r recipient_email -f error_log.txt"
  exit 1
fi

# Read the error log.
error_log=$(cat "$error_file")

# Call the Geminai API to get a detailed explanation and recommendations.
# Ensure GEMINAI_API_KEY is available in the environment (e.g., via GitHub Actions secrets).
detailed_response=$(curl -s -X POST "https://api.geminai.com/analyze_error" \
  -H "Content-Type: application/json" \
  -d "{\"api_key\": \"${GEMINAI_API_KEY}\", \"error_log\": \"$(echo "$error_log" | sed 's/\"/\\"/g')\"}")

# Extract the detailed explanation from the API response.
detailed_explanation=$(echo "$detailed_response" | jq -r '.detailed_explanation')

# Fallback message if API does not return a value.
if [ -z "$detailed_explanation" ] || [ "$detailed_explanation" == "null" ]; then
  detailed_explanation="No detailed explanation available. Please review the error log for more information."
fi

# Create the full email body.
email_body="Error encountered:
$error_log

Detailed Explanation & Recommended Fixes:
$detailed_explanation

For additional details, please refer to the system logs and monitoring dashboard."

# Send the email using swaks with TLS enabled.
swaks --to "$recipient" \
      --from "alert@example.com" \
      --server smtp.gmail.com:587 \
      --tls \
      --auth LOGIN \
      --auth-user "test.java.153@gmail.com" \
      --auth-password "your_actual_app_password" \
      --header "Subject: Enhanced Log Monitor Alert" \
      --body "$email_body"
