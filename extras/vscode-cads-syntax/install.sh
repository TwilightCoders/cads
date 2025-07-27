#!/bin/bash

# CADS VSCode Extension Installer
# Installs the CADS syntax highlighting extension to VSCode

set -e

EXTENSION_NAME="cads-syntax"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Determine VSCode extensions directory
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    EXTENSIONS_DIR="$HOME/.vscode/extensions"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    # Windows
    EXTENSIONS_DIR="$USERPROFILE/.vscode/extensions"
else
    # Linux
    EXTENSIONS_DIR="$HOME/.vscode/extensions"
fi

TARGET_DIR="$EXTENSIONS_DIR/twilightcoders.$EXTENSION_NAME-1.0.0"

echo "🔍 CADS VSCode Extension Installer"
echo "=================================="
echo "Installing to: $TARGET_DIR"

# Create extensions directory if it doesn't exist
mkdir -p "$EXTENSIONS_DIR"

# Remove existing installation
if [ -d "$TARGET_DIR" ]; then
    echo "🗑️  Removing existing installation..."
    rm -rf "$TARGET_DIR"
fi

# Copy extension files
echo "📦 Installing extension..."
cp -r "$SCRIPT_DIR" "$TARGET_DIR"

# Remove installer script from target
rm -f "$TARGET_DIR/install.sh"

echo "✅ Installation complete!"
echo ""
echo "Next steps:"
echo "1. Restart VSCode"
echo "2. Open any .cads file to see syntax highlighting"
echo ""
echo "Example .cads files available in:"
echo "  - examples/mxt275_quick_verify.cads"
echo "  - examples/gmrs_fast_discovery.cads"