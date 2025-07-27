#!/bin/bash

# CADS VSCode Extension Packager
# Creates a .vsix package for distribution

set -e

EXTENSION_NAME="cads-syntax"
VERSION="1.0.0"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "📦 CADS VSCode Extension Packager"
echo "================================="

# Check if vsce is installed
if ! command -v vsce &> /dev/null; then
    echo "❌ vsce (Visual Studio Code Extension manager) not found"
    echo "📥 Install with: npm install -g vsce"
    echo "🔗 More info: https://code.visualstudio.com/api/working-with-extensions/publishing-extension"
    exit 1
fi

cd "$SCRIPT_DIR"

echo "🔨 Building extension package..."
vsce package

PACKAGE_FILE="$EXTENSION_NAME-$VERSION.vsix"

if [ -f "$PACKAGE_FILE" ]; then
    echo "✅ Package created: $PACKAGE_FILE"
    echo ""
    echo "Installation options:"
    echo "1. Manual install: code --install-extension $PACKAGE_FILE"
    echo "2. VSCode GUI: Extensions view → ... → Install from VSIX"
    echo "3. Direct install: ./install.sh"
else
    echo "❌ Package creation failed"
    exit 1
fi