# Makefile for Objective 2: Slotted Page Testing

CC = gcc
CFLAGS = -Wall -g -I.
LDFLAGS = 

# Data directory (relative to parent directory)
DATA_DIR = ../data 

# Source files
SLOTTED_SRCS = slotted_page.c student_file.c
SLOTTED_OBJS = $(SLOTTED_SRCS:.c=.o)

# Test programs
TEST_PROGS = test_objective2_final

.PHONY: all clean test

all: $(TEST_PROGS)

# Comprehensive test with results table and performance analysis
test_objective2_final: test_objective2_final.o $(SLOTTED_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Object file rules
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
slotted_page.o: slotted_page.c slotted_page.h
student_file.o: student_file.c student_file.h slotted_page.h
test_objective2_final.o: test_objective2_final.c student_file.h slotted_page.h

# Test target - runs with ALL records from data/student.txt
test: test_objective2_final
	@echo "Running Objective 2 comprehensive tests with ALL records..."
	@echo ""
	./test_objective2_final
	@echo ""
	@echo "Test completed. Database files: test_slotted.db, test_static_*.db"

# Test with limited records
test-10k: test_objective2_final
	@echo "Running test with 10,000 records..."
	./test_objective2_final 10000

# Quick test with fewer records
test-quick: test_objective2_final
	@echo "Running quick test with 1000 records..."
	./test_objective2_final 1000

# Clean build artifacts only (keep test databases)
clean:
	rm -f $(TEST_PROGS) *.o

# Clean everything including test database files
clean-all: clean
	rm -f test_*.db *.db
	rm -f results_*.txt objective2_results.txt
	@echo "Cleaned all build artifacts and test database files"

# Clean only database files
clean-db:
	rm -f test_*.db *.db
	@echo "Cleaned all test database files"

# Help target
help:
	@echo "Objective 2 Makefile - Slotted Page Testing"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build all test programs"
	@echo "  test             - Run comprehensive tests with ALL records"
	@echo "  test-10k         - Run tests with 10,000 records"
	@echo "  test-quick       - Run quick test with 1,000 records"
	@echo "  clean            - Remove build artifacts (keep databases)"
	@echo "  clean-all        - Remove everything (builds + databases + results)"
	@echo "  clean-db         - Remove only test database files"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Build everything"
	@echo "  make test         # Run tests with ALL records (~17,814)"
	@echo "  make test-10k     # Run tests with 10,000 records"
	@echo "  make test-quick   # Run quick test with 1,000 records"
	@echo "  make clean-db     # Remove test .db files"
	@echo "  make clean-all    # Remove all generated files"
	@echo ""
	@echo "Custom record count:"
	@echo "  ./test_objective2_final 5000   # Run with 5000 records"
	@echo "  ./test_objective2_final        # Run with ALL records"
	@echo ""
	@echo "Note: Tests use student data from ../data/student.txt (~17,814 records)"
