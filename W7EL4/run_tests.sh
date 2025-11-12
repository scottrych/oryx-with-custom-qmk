#!/usr/bin/env fish

# run_tests.sh — Compile and run Achordion unit tests
# Compatible with fish shell environment

echo "=== Building Achordion Unit Tests ==="
echo ""

# Compile the standalone test file
gcc -std=c99 -Wall -Wextra -g -o test_achordion_standalone test_achordion_standalone.c

# Check if compilation succeeded
if test $status -eq 0
    echo "✓ Compilation successful"
    echo ""
    echo "=== Running Tests ==="
    echo ""
    
    # Run the tests
    ./test_achordion_standalone
    set test_result $status
    
    echo ""
    if test $test_result -eq 0
        echo "🎉 All tests completed successfully!"
    else
        echo "❌ Some tests failed (exit code: $test_result)"
    end
    
    # Clean up
    echo ""
    echo "Cleaning up..."
    rm -f test_achordion_standalone
    
    exit $test_result
else
    echo "❌ Compilation failed!"
    exit 1
end