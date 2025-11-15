#PUBLICDIR= /usr0/cs564/public/project
SRC= buf.c hash.c pf.c
OBJ= buf.o hash.o pf.o
HDR = pftypes.h pf.h 

# Compiler settings
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm

pflayer.o: $(OBJ)
	ld -r -o pflayer.o $(OBJ)

tests: testhash testpf test_realdata

# Original toydb tests
original_tests: testhash testpf

# Generate graphs with real data
graphs: test_realdata generate_graphs.py
	@echo "========================================"
	@echo "Generating Performance Graphs from Real Data"
	@echo "========================================"
	@rm -f realdata_lru.csv realdata_mru.csv
	@rm -rf graphs
	./test_realdata --csv
	@echo ""
	@echo "Generating graphs..."
	python3 generate_graphs.py realdata_lru.csv realdata_mru.csv
	@echo ""
	@echo "✓ Graphs generated successfully in graphs/ directory"
	@echo "View with: eog graphs/*.png (Linux) or open graphs/*.png (macOS)"

testpf: testpf.o pflayer.o
	cc -o testpf testpf.o pflayer.o

testhash: testhash.o pflayer.o
	cc -o testhash testhash.o pflayer.o

# Enhanced buffer management test with real data
test_realdata: test_realdata.o pflayer.o
	$(CC) $(CFLAGS) -o test_realdata test_realdata.o pflayer.o $(LDFLAGS)

test_realdata.o: test_realdata.c $(HDR)
	$(CC) $(CFLAGS) -c test_realdata.c

$(OBJ): $(HDR)

testhash.o: $(HDR)

testpf.o: $(HDR)

lint: 
	lint $(SRC)

install: pflayer.o

clean:
	rm -f $(OBJ) pflayer.o
	rm -f testpf testpf.o testhash testhash.o
	rm -f test_realdata test_realdata.o
	rm -f file1 file2 *.db
	rm -f *.csv
	rm -rf graphs
	rm -f core

# Clean only graphs and CSV files (keep executables)
clean-results:
	rm -f *.csv
	rm -rf graphs
	rm -f *.db
	@echo "✓ Cleaned previous results (CSV files and graphs)"

.PHONY: tests original_tests graphs clean clean-results
