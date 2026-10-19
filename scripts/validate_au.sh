#!/bin/bash

# Audio Unit Validation Script for MIDI Splitter Plugin
# This script runs the macOS auval tool on the compiled plugin

set -e  # Exit on any error

# Configuration
TARGET_NAME="MIDISplitterPlugin"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
source "$SCRIPT_DIR/branch_config.sh"
GIT_BRANCH="$(midisplitter_branch_name "$PROJECT_ROOT")"
PLUGIN_NAME="$(midisplitter_product_name "$GIT_BRANCH")"
BUILD_DIR="$(midisplitter_build_dir "$PROJECT_ROOT" "$GIT_BRANCH" macOS)"
AU_TYPE="aumf"  # MIDI Effect
MANUFACTURER_CODE="Nliv"
PLUGIN_CODE="Mdsp"
if [[ "$GIT_BRANCH" != "main" ]]; then
    PLUGIN_CODE="MdDv"
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Audio Unit Validation Script${NC}"
echo "===================================="

# Function to check if we're on macOS
check_macos() {
    if [[ "$(uname)" != "Darwin" ]]; then
        echo -e "${RED}Audio Unit validation is only available on macOS${NC}"
        exit 1
    fi
}

# Function to find the built AU plugin
find_au_plugin() {
    local plugin_paths=(
        "$BUILD_DIR/${TARGET_NAME}_artefacts/Release/AU/${PLUGIN_NAME}.component"
        "$BUILD_DIR/${TARGET_NAME}_artefacts/AU/${PLUGIN_NAME}.component"
        "$BUILD_DIR/AU/${PLUGIN_NAME}.component"
        "${PLUGIN_NAME}.component"
        "~/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component"
        "/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component"
    )
    
    for path in "${plugin_paths[@]}"; do
        # Expand tilde
        expanded_path="${path/#\~/$HOME}"
        if [[ -d "$expanded_path" ]]; then
            AU_PLUGIN_PATH="$expanded_path"
            echo -e "${GREEN}Found AU plugin at: $expanded_path${NC}"
            return 0
        fi
    done
    
    echo -e "${RED}AU plugin not found. Please build the project first.${NC}"
    echo "Run: cmake --build $BUILD_DIR --config Release"
    return 1
}

# Function to install plugin temporarily for validation
install_plugin_temp() {
    local temp_install_path="$HOME/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component"
    
    if [[ "$AU_PLUGIN_PATH" != "$temp_install_path" ]]; then
        echo -e "${YELLOW}Installing plugin temporarily for validation...${NC}"
        
        # Create directory if it doesn't exist
        mkdir -p "$(dirname "$temp_install_path")"
        
        # Copy plugin to system location
        cp -R "$AU_PLUGIN_PATH" "$temp_install_path"
        
        # Update path
        AU_PLUGIN_PATH="$temp_install_path"
        TEMP_INSTALLED=true
        
        echo -e "${GREEN}Plugin installed temporarily at: $temp_install_path${NC}"
    fi
}

# Function to remove temporary installation
cleanup_temp_install() {
    if [[ "$TEMP_INSTALLED" == "true" ]]; then
        echo -e "${YELLOW}Cleaning up temporary installation...${NC}"
        rm -rf "$HOME/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component"
        echo -e "${GREEN}Temporary installation cleaned up${NC}"
    fi
}

# Function to check AU plugin structure
check_au_structure() {
    echo -e "${YELLOW}Checking AU plugin structure...${NC}"
    
    # Check if plugin bundle exists
    if [[ ! -d "$AU_PLUGIN_PATH" ]]; then
        echo -e "${RED}✗ AU plugin bundle not found${NC}"
        return 1
    fi
    
    # Check for Info.plist
    if [[ ! -f "$AU_PLUGIN_PATH/Contents/Info.plist" ]]; then
        echo -e "${RED}✗ Missing Info.plist${NC}"
        return 1
    fi
    
    # Check for binary
    local binary_path="$AU_PLUGIN_PATH/Contents/MacOS/$PLUGIN_NAME"
    if [[ ! -f "$binary_path" ]]; then
        echo -e "${RED}✗ Missing plugin binary at $binary_path${NC}"
        return 1
    fi
    
    # Check Info.plist content
    local bundle_type=$(plutil -extract CFBundlePackageType raw "$AU_PLUGIN_PATH/Contents/Info.plist" 2>/dev/null || echo "")
    if [[ "$bundle_type" != "BNDL" ]]; then
        echo -e "${YELLOW}⚠ Bundle type is '$bundle_type', expected 'BNDL'${NC}"
    fi
    
    echo -e "${GREEN}✓ AU plugin structure is valid${NC}"
    return 0
}

# Function to run auval validation
run_auval() {
    echo -e "${YELLOW}Running auval validation...${NC}"
    echo "Plugin Type: $AU_TYPE"
    echo "Manufacturer: $MANUFACTURER_CODE"
    echo "Plugin Code: $PLUGIN_CODE"
    echo ""
    
    # First, try to find the plugin
    echo -e "${BLUE}Searching for plugin...${NC}"
    if ! auval -s "$AU_TYPE" "$PLUGIN_CODE" "$MANUFACTURER_CODE"; then
        echo -e "${RED}✗ Plugin not found by auval${NC}"
        echo "Make sure the plugin is properly installed and the codes match."
        return 1
    fi
    
    echo ""
    echo -e "${BLUE}Running comprehensive validation...${NC}"
    
    # Run full validation
    if auval -v "$AU_TYPE" "$PLUGIN_CODE" "$MANUFACTURER_CODE"; then
        echo -e "${GREEN}✓ auval validation PASSED${NC}"
        return 0
    else
        echo -e "${RED}✗ auval validation FAILED${NC}"
        return 1
    fi
}

# Function to run basic AU tests
run_basic_au_tests() {
    echo -e "${YELLOW}Running basic AU functionality tests...${NC}"
    
    # Test plugin loading
    echo "- Plugin bundle structure: OK"
    echo "- Info.plist present: OK"
    echo "- Binary present: OK"
    
    # Check binary architecture
    local binary_path="$AU_PLUGIN_PATH/Contents/MacOS/$PLUGIN_NAME"
    if [[ -f "$binary_path" ]]; then
        local arch_info=$(file "$binary_path")
        echo "- Binary architecture: $arch_info"
    fi
    
    # Check code signing (if present)
    if codesign -v "$AU_PLUGIN_PATH" 2>/dev/null; then
        echo "- Code signing: Valid"
    else
        echo "- Code signing: Not signed (OK for development)"
    fi
    
    echo -e "${GREEN}✓ Basic AU tests completed${NC}"
    return 0
}

# Function to show plugin info
show_plugin_info() {
    echo -e "${YELLOW}Plugin Information:${NC}"
    echo "=================="
    
    local info_plist="$AU_PLUGIN_PATH/Contents/Info.plist"
    
    if [[ -f "$info_plist" ]]; then
        echo "Bundle Name: $(plutil -extract CFBundleName raw "$info_plist" 2>/dev/null || echo 'Unknown')"
        echo "Bundle Version: $(plutil -extract CFBundleVersion raw "$info_plist" 2>/dev/null || echo 'Unknown')"
        echo "Bundle Identifier: $(plutil -extract CFBundleIdentifier raw "$info_plist" 2>/dev/null || echo 'Unknown')"
        echo "Manufacturer: $(plutil -extract AudioComponents.0.manufacturer raw "$info_plist" 2>/dev/null || echo 'Unknown')"
        echo "Type: $(plutil -extract AudioComponents.0.type raw "$info_plist" 2>/dev/null || echo 'Unknown')"
        echo "Subtype: $(plutil -extract AudioComponents.0.subtype raw "$info_plist" 2>/dev/null || echo 'Unknown')"
    else
        echo "Info.plist not found"
    fi
    
    echo ""
}

# Trap to ensure cleanup
trap cleanup_temp_install EXIT

# Main execution
main() {
    echo "Starting AU validation for $PLUGIN_NAME..."
    echo ""
    
    # Check if we're on macOS
    check_macos
    
    # Check if build directory exists
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo -e "${RED}Build directory not found. Please build the project first.${NC}"
        echo "Run: mkdir build && cd build && cmake .. && cmake --build . --config Release"
        exit 1
    fi
    
    # Find AU plugin
    if ! find_au_plugin; then
        exit 1
    fi
    
    # Show plugin info
    show_plugin_info
    
    # Check AU structure
    if ! check_au_structure; then
        exit 1
    fi
    
    # Install plugin temporarily if needed
    install_plugin_temp
    
    # Run basic tests
    if ! run_basic_au_tests; then
        exit 1
    fi
    
    # Run auval validation
    if ! run_auval; then
        exit 1
    fi
    
    echo ""
    echo -e "${GREEN}🎉 All AU validation tests PASSED!${NC}"
    echo "The plugin is ready for distribution on macOS."
}

# Handle command line arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [options]"
        echo ""
        echo "Options:"
        echo "  --help, -h     Show this help message"
        echo "  --plugin       Specify path to AU plugin"
        echo "  --type         Override AU type (default: aumf)"
        echo "  --manu         Override manufacturer code (default: Mdar)"
        echo "  --code         Override plugin code (default: Mdar)"
        echo ""
        echo "Environment variables:"
        echo "  AU_PLUGIN_PATH Override plugin location"
        exit 0
        ;;
    --plugin)
        AU_PLUGIN_PATH="$2"
        shift 2
        ;;
    --type)
        AU_TYPE="$2"
        shift 2
        ;;
    --manu)
        MANUFACTURER_CODE="$2"
        shift 2
        ;;
    --code)
        PLUGIN_CODE="$2"
        shift 2
        ;;
esac

# Run main function
main "$@"
