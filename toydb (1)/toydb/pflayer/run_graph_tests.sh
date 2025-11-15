#!/bin/bash

# Script to generate performance graphs for buffer management comparison
# Usage: ./run_graph_tests.sh

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "Buffer Management Performance Analysis"
echo "=========================================="
echo ""

# Check if test_graph_data exists
if [ ! -f "test_graph_data" ]; then
    echo "Building test_graph_data..."
    make test_graph_data
    echo ""
fi

# Check if Python and required packages are available
if ! command -v python3 &> /dev/null; then
    echo "Error: python3 not found. Please install Python 3."
    exit 1
fi

# Check for required Python packages
echo "Checking Python dependencies..."
python3 -c "import pandas, matplotlib" 2>/dev/null || {
    echo ""
    echo "Installing required Python packages..."
    pip3 install pandas matplotlib --user
    echo ""
}

# Clean up old results
echo "Cleaning up old results..."
rm -f lru_results.csv mru_results.csv
rm -rf graphs

echo ""
echo "=========================================="
echo "Running LRU Test..."
echo "=========================================="
./test_graph_data LRU lru_results.csv

echo ""
echo "=========================================="
echo "Running MRU Test..."
echo "=========================================="
./test_graph_data MRU mru_results.csv

echo ""
echo "=========================================="
echo "Generating Graphs..."
echo "=========================================="
python3 generate_graphs.py lru_results.csv mru_results.csv

echo ""
echo "=========================================="
echo "Analysis Complete!"
echo "=========================================="
echo ""
echo "Results:"
echo "  - CSV data: lru_results.csv, mru_results.csv"
echo "  - Graphs: graphs/*.png"
echo ""
echo "View graphs with:"
echo "  eog graphs/*.png    (Linux)"
echo "  open graphs/*.png   (macOS)"
echo ""
