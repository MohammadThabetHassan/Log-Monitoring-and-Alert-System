CC = gcc
CFLAGS = -Wall -O2
TARGET = logParser

# Default target builds the logParser binary.
all: $(TARGET)

# Build the logParser binary from logParser.c and add execution permission.
$(TARGET): logParser.c
	$(CC) $(CFLAGS) -o $(TARGET) logParser.c
	chmod +x $(TARGET)

# The test target:
# 1. Generates fake logs using generate_fake_logs.sh.
# 2. Ensures both logParser and send_email.sh are executable.
# 3. Sets the EMAIL environment variable and runs logParser.
test: $(TARGET)
	@echo "Generating fake logs using generate_fake_logs.sh..."
	@chmod +x ./generate_fake_logs.sh
	./generate_fake_logs.sh
	@echo "Ensuring logParser is executable..."
	@chmod +x ./$(TARGET)
	@echo "Ensuring send_email.sh is executable..."
	@chmod +x ./send_email.sh
	@echo "Running logParser test with EMAIL set to mohd20vm@gmail.com..."
	EMAIL="mohd20vm@gmail.com" ./$(TARGET) config.txt logs/fake_syslog.log --keyword "login" --level "ERROR"

# Clean target removes compiled binaries and generated logs.
clean:
	rm -f $(TARGET) *.o
	rm -rf logs

.PHONY: all test clean
