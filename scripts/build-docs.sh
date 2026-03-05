#!/usr/bin/env bash
#
# build-docs.sh — Build the complete Subvision CV documentation site.
#
# This script generates:
#   1. C++ API documentation (Doxygen → docs/cpp/html)
#   2. .NET API metadata     (DocFX metadata → docs/api)
#   3. Final documentation   (DocFX build → docs/_site)
#
# Prerequisites:
#   - Doxygen  (https://www.doxygen.nl/download.html)
#   - DocFX    (https://dotnet.github.io/docfx/)
#
# Usage:
#   chmod +x scripts/build-docs.sh
#   ./scripts/build-docs.sh
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DOCS_DIR="$PROJECT_ROOT/docs"

echo "========================================"
echo "  Subvision CV — Documentation Builder"
echo "========================================"
echo ""
echo "Project root: $PROJECT_ROOT"
echo "Docs dir:     $DOCS_DIR"
echo ""

# ──────────────────────────────────────────
# Step 1: Generate C++ API docs with Doxygen
# ──────────────────────────────────────────
echo "──────────────────────────────────────"
echo "Step 1/3: Generating C++ API docs (Doxygen)"
echo "──────────────────────────────────────"

if ! command -v doxygen &> /dev/null; then
    echo "ERROR: Doxygen is not installed or not in PATH."
    echo "  Install: https://www.doxygen.nl/download.html"
    exit 1
fi

cd "$DOCS_DIR"
doxygen Doxyfile

echo "✓ C++ docs generated in: $DOCS_DIR/cpp/html"
echo ""

# ──────────────────────────────────────────
# Step 2: Generate .NET API metadata
# ──────────────────────────────────────────
echo "──────────────────────────────────────"
echo "Step 2/3: Generating .NET API metadata (DocFX)"
echo "──────────────────────────────────────"

if ! command -v docfx &> /dev/null; then
    echo "WARNING: DocFX is not installed or not in PATH."
    echo "  Install: dotnet tool install -g docfx"
    echo "  Skipping .NET metadata generation."
    echo ""
else
    cd "$DOCS_DIR"
    docfx metadata docfx.json
    echo "✓ .NET metadata generated"
    echo ""
fi

# ──────────────────────────────────────────
# Step 3: Build the documentation website
# ──────────────────────────────────────────
echo "──────────────────────────────────────"
echo "Step 3/3: Building documentation site (DocFX)"
echo "──────────────────────────────────────"

if ! command -v docfx &> /dev/null; then
    echo "WARNING: DocFX is not installed. Cannot build final site."
    echo "  The C++ docs are still available at: $DOCS_DIR/cpp/html/index.html"
    echo ""
    exit 0
fi

cd "$DOCS_DIR"
docfx build docfx.json

echo ""
echo "========================================"
echo "  ✓ Documentation build complete!"
echo "========================================"
echo ""
echo "Output locations:"
echo "  C++ API docs:  $DOCS_DIR/cpp/html/index.html"
echo "  Full site:     $DOCS_DIR/_site/index.html"
echo ""
echo "To preview locally:"
echo "  docfx serve $DOCS_DIR/_site"
echo ""
