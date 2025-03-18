#!/bin/bash

# Set the directory to store coverage information
COVERAGE_DIR="build/coverage"

# Create the coverage directory if it doesn't exist
mkdir -p $COVERAGE_DIR

# Collect coverage data
echo "Collecting coverage data..."
lcov --capture --directory . --output-file $COVERAGE_DIR/coverage.info

# Filter out system and test files
echo "Filtering coverage data..."
lcov --remove $COVERAGE_DIR/coverage.info '/usr/*' '*/unittest/*' '*/gtest/*' --output-file $COVERAGE_DIR/coverage_filtered.info

# Generate HTML report
echo "Generating HTML report..."
genhtml $COVERAGE_DIR/coverage_filtered.info --output-directory $COVERAGE_DIR

echo "Coverage report generated in $COVERAGE_DIR"