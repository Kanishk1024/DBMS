#!/bin/bash

# View graphs in default image viewer
# Usage: ./view_graphs.sh

cd "$( dirname "${BASH_SOURCE[0]}" )"

if [ ! -d "graphs" ]; then
    echo "Error: graphs/ directory not found"
    echo "Please run: ./run_graph_tests.sh first"
    exit 1
fi

echo "Opening performance graphs..."
echo ""
echo "Graphs available:"
echo "  1. physical_io_comparison.png - Physical I/O patterns"
echo "  2. hit_ratio_comparison.png - Hit ratio LRU vs MRU"
echo "  3. buffer_performance.png - Hits vs Misses breakdown"
echo "  4. total_io_comparison.png - Total I/O efficiency"
echo "  5. summary_table.png - Complete performance summary"
echo ""

# Try different viewers based on platform
if command -v eog &> /dev/null; then
    eog graphs/*.png &
elif command -v xdg-open &> /dev/null; then
    for img in graphs/*.png; do
        xdg-open "$img" &
    done
elif command -v open &> /dev/null; then
    open graphs/*.png
elif command -v display &> /dev/null; then
    for img in graphs/*.png; do
        display "$img" &
    done
else
    echo "No image viewer found. Please open graphs manually:"
    ls -1 graphs/*.png
fi
