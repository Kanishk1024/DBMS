#!/bin/bash

# Run Objective 2 Tests and Generate Results

echo "============================================"
echo "    OBJECTIVE 2: Slotted Page Testing     "
echo "============================================"
echo ""

# Compile
echo "Compiling..."
make -f Makefile.objective2 clean > /dev/null 2>&1
make -f Makefile.objective2 all > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "Error: Compilation failed"
    exit 1
fi

echo "Compilation successful!"
echo ""

# Test with different record counts
RECORD_COUNTS=(1000 5000 10000 15000)

for COUNT in "${RECORD_COUNTS[@]}"; do
    echo "================================================"
    echo "   Testing with $COUNT records"
    echo "================================================"
    ./test_objective2 $COUNT 2>&1 | tee results_${COUNT}.txt
    echo ""
done

echo ""
echo "============================================"
echo "All tests completed!"
echo "Results saved in results_*.txt files"
echo "============================================"
