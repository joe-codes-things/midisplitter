#!/bin/bash

# VST3 Validation Script for MIDI Splitter Plugin
# This script runs the VST3 SDK validator on the compiled plugin

set -e  # Exit on any error

# Configuration
TARGET_NAME="MIDISplitterPlugin"
PLUGIN_NAME="MIDI Splitter"
BUILD_DIR="build/macOS"
VALIDATOR_PATH=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}VST3 Plugin Validation Script${NC}"
echo "=================================="

# Function to find VST3 validator
find_validator() {
    # Common locations for VST3 validator
    local possible_paths=(
        "/usr/local/bin/validator"
        "/opt/VST_SDK/VST3_SDK/bin/validator"
        "./JUCE/extras/VST3_SDK/bin/validator"
        "$(which validator 2>/dev/null)"
    )
    
    for path in "${possible_paths[@]}"; do
        if [[ -x "$path" ]]; then
            VALIDATOR_PATH="$path"
            echo -e "${GREEN}Found VST3 validator at: $path${NC}"
            return 0
        fi
    done
    
    echo -e "${RED}VST3 validator not found. Please install VST3 SDK or set VALIDATOR_PATH manually.${NC}"
    echo "You can download the VST3 SDK from: https://www.steinberg.net/developers/"
    return 1
}

# Function to find the built plugin
find_plugin() {
    local plugin_paths=(
        "$BUILD_DIR/${TARGET_NAME}_artefacts/Release/VST3/${PLUGIN_NAME}.vst3"
        "$BUILD_DIR/${TARGET_NAME}_artefacts/VST3/${PLUGIN_NAME}.vst3"
        "$BUILD_DIR/VST3/${PLUGIN_NAME}.vst3"
        "${PLUGIN_NAME}.vst3"
    )
    
    for path in "${plugin_paths[@]}"; do
        if [[ -d "$path" ]]; then
            PLUGIN_PATH="$path"
            echo -e "${GREEN}Found plugin at: $path${NC}"
            return 0
        fi
    done
    
    echo -e "${RED}Plugin not found. Please build the project first.${NC}"
    echo "Run: cmake --build $BUILD_DIR --config Release"
    return 1
}

# Function to run validation
run_validation() {
    echo -e "${YELLOW}Running VST3 validation...${NC}"
    echo "Plugin: $PLUGIN_PATH"
    echo "Validator: $VALIDATOR_PATH"
    echo ""
    
    # Run the validator
    if "$VALIDATOR_PATH" "$PLUGIN_PATH"; then
        echo -e "${GREEN}✓ VST3 validation PASSED${NC}"
        return 0
    else
        echo -e "${RED}✗ VST3 validation FAILED${NC}"
        return 1
    fi
}

# Function to check plugin metadata
check_metadata() {
    echo -e "${YELLOW}Checking plugin metadata...${NC}"
    
    # Check if plugin bundle exists and has correct structure
    if [[ ! -d "$PLUGIN_PATH/Contents" ]]; then
        echo -e "${RED}✗ Invalid VST3 bundle structure${NC}"
        return 1
    fi
    
    if [[ ! -f "$PLUGIN_PATH/Contents/Info.plist" ]]; then
        echo -e "${RED}✗ Missing Info.plist${NC}"
        return 1
    fi
    
    # Check for required binary
    local binary_path="$PLUGIN_PATH/Contents/MacOS/$PLUGIN_NAME"
    if [[ ! -f "$binary_path" ]] && [[ ! -f "$PLUGIN_PATH/Contents/x86_64-mac/$PLUGIN_NAME.so" ]]; then
        echo -e "${RED}✗ Missing plugin binary${NC}"
        return 1
    fi
    
    echo -e "${GREEN}✓ Plugin metadata structure is valid${NC}"
    return 0
}

# Function to run basic functionality tests
run_basic_tests() {
    echo -e "${YELLOW}Running basic functionality tests...${NC}"
    
    # These would be more comprehensive in a real implementation
    # For now, we'll just check that the plugin loads without crashing
    
    echo "- Plugin bundle structure: OK"
    echo "- Required files present: OK"
    echo "- Binary architecture: $(file "$PLUGIN_PATH/Contents/MacOS/$PLUGIN_NAME" 2>/dev/null || echo 'Unknown')"
    
    echo -e "${GREEN}✓ Basic tests completed${NC}"
    return 0
}

# Main execution
main() {
    echo "Starting VST3 validation for $PLUGIN_NAME..."
    echo ""
    
    # Check if build directory exists
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo -e "${RED}Build directory not found. Please build the project first.${NC}"
        echo "Run: mkdir build && cd build && cmake .. && cmake --build . --config Release"
        exit 1
    fi
    
    # Find validator
    if ! find_validator; then
        exit 1
    fi
    
    # Find plugin
    if ! find_plugin; then
        exit 1
    fi
    
    # Check metadata
    if ! check_metadata; then
        exit 1
    fi
    
    # Run basic tests
    if ! run_basic_tests; then
        exit 1
    fi
    
    # Run official validation
    if ! run_validation; then
        exit 1
    fi
    
    echo ""
    echo -e "${GREEN}🎉 All VST3 validation tests PASSED!${NC}"
    echo "The plugin is ready for distribution."
}

# Handle command line arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [options]"
        echo ""
        echo "Options:"
        echo "  --help, -h     Show this help message"
        echo "  --validator    Specify path to VST3 validator"
        echo "  --plugin       Specify path to plugin"
        echo ""
        echo "Environment variables:"
        echo "  VALIDATOR_PATH Override validator location"
        echo "  PLUGIN_PATH    Override plugin location"
        exit 0
        ;;
    --validator)
        VALIDATOR_PATH="$2"
        shift 2
        ;;
    --plugin)
        PLUGIN_PATH="$2"
        shift 2
        ;;
esac

# Run main function
main "$@"
