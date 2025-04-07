CC = gcc
CFLAGS = -Wall -O2
TARGET = logParser

# Default target builds the logParser binary.
all: $(TARGET)

# Build the logParser binary from logParser.c and add execution permission.
$(TARGET): logParser.c
	$(CC) $(CFLAGS) -o $(TARGET) logParser.c
	chmod +x $(TARGET)

# The test target first runs the generate_fake_logs.sh script,
# then explicitly sets the execute permission for logParser before executing it.
test: $(TARGET)
	@echo "Generating fake logs using generate_fake_logs.sh..."
	@chmod +x ./generate_fake_logs.sh
	./generate_fake_logs.sh
	@echo "Ensuring logParser is executable..."
	@chmod +x ./$(TARGET)
	@echo "Running logParser test..."
	./$(TARGET) config.txt logs/fake_syslog.log --keyword "login" --level "ERROR"

# Clean target removes compiled binaries and any generated log files.
clean:
	rm -f $(TARGET) *.o
	rm -rf logs

.PHONY: all test clean
