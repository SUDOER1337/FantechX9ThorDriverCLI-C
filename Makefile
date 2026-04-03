# Fantech X9 Thor Driver CLI - C Implementation Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lusb-1.0 -lm -lpthread

# Directories
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/fantech-driver

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Link the final executable
$(TARGET): $(OBJECTS)
	@echo "Linking $@"
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJDIR) $(BINDIR)

# Clean and install system-wide
clean-install: clean directories install
	@echo "Clean install completed!"

# Install to system
install: directories $(TARGET)
	@echo "Installing fantech-driver to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod 755 /usr/local/bin/fantech-driver

# Uninstall from system
uninstall:
	@echo "Removing fantech-driver from /usr/local/bin..."
	sudo rm -f /usr/local/bin/fantech-driver

# Test compilation (don't create final binary)
test-compile: directories
	@echo "Testing compilation..."
	$(MAKE) $(OBJECTS)
	@echo "Compilation test successful!"

# Run with test config
test: $(TARGET)
	@echo "Testing with find command..."
	./$(TARGET) find

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean all

# Release build
release: CFLAGS += -DNDEBUG -s
release: clean all

# Check for required dependencies
check-deps:
	@echo "Checking for required dependencies..."
	@pkg-config --exists libusb-1.0 && echo "✓ libusb-1.0 found" || echo "✗ libusb-1.0 not found"
	@which gcc > /dev/null && echo "✓ gcc found" || echo "✗ gcc not found"

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build project (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  clean-install- Clean and install system-wide"
	@echo "  install      - Install to /usr/local/bin"
	@echo "  uninstall    - Remove from /usr/local/bin"
	@echo "  test-compile - Test compilation without linking"
	@echo "  test         - Run basic test"
	@echo "  debug        - Build with debug symbols"
	@echo "  release      - Build optimized release version"
	@echo "  check-deps   - Check for required dependencies"
	@echo "  help         - Show this help"

.PHONY: all clean install uninstall clean-install test-compile test debug release check-deps help directories
