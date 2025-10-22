#!/bin/bash
# AuxPoW Test Runner Script for Classic2
# Usage: ./test_auxpow.sh [options]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Classic2 AuxPoW Test Suite ===${NC}"
echo ""

# Check if test binary exists
if [ ! -f "src/test/test_bitcoin" ]; then
    echo -e "${YELLOW}Test binary not found. Building...${NC}"
    make -j$(nproc) || {
        echo -e "${RED}Build failed!${NC}"
        exit 1
    }
fi

# Parse command line arguments
VERBOSE=0
SPECIFIC_TEST=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -t|--test)
            SPECIFIC_TEST="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  -v, --verbose       Run tests with verbose output"
            echo "  -t, --test NAME     Run specific test (e.g., check_auxpow)"
            echo "  -h, --help          Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                  # Run all auxpow tests"
            echo "  $0 -v               # Run with verbose output"
            echo "  $0 -t check_auxpow  # Run specific test"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Build log level argument
if [ $VERBOSE -eq 1 ]; then
    LOG_LEVEL="--log_level=all"
else
    LOG_LEVEL="--log_level=test_suite"
fi

# Build test filter
if [ -n "$SPECIFIC_TEST" ]; then
    TEST_FILTER="--run_test=auxpow_tests/$SPECIFIC_TEST"
    echo -e "${YELLOW}Running specific test: auxpow_tests/$SPECIFIC_TEST${NC}"
else
    TEST_FILTER="--run_test=auxpow_tests"
    echo -e "${YELLOW}Running all auxpow tests...${NC}"
fi

echo ""

# Run the tests
./src/test/test_bitcoin $LOG_LEVEL $TEST_FILTER

# Check result
if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo ""
    echo "AuxPoW implementation is working correctly."
    echo ""
    echo "Next steps:"
    echo "  1. Test on regtest: bitcoind -regtest"
    echo "  2. Test RPC methods: bitcoin-cli createauxblock <address>"
    echo "  3. Review AUXPOW_FIXES.md for deployment checklist"
    exit 0
else
    echo ""
    echo -e "${RED}✗ Tests failed!${NC}"
    echo ""
    echo "Please review the error output above."
    echo "For verbose output, run: $0 -v"
    exit 1
fi
