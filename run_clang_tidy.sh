#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ ! -f "${COMPILE_COMMANDS}" ]; then
    echo -e "${RED}Error: compile_commands.json not found in '${BUILD_DIR}'.${NC}"
    echo "Run cmake first to generate it."
    exit 1
fi

if [ ! -f "${SCRIPT_DIR}/.clang-tidy" ]; then
    echo -e "${RED}Error: .clang-tidy not found at project root.${NC}"
    exit 1
fi

# Directories to exclude from analysis (third-party / generated code)
EXCLUDE_DIRS=("lib" "mxcube")

# Build the exclusion filter for Python
EXCLUDE_PATTERNS=$(printf ",'/%s/'" "${EXCLUDE_DIRS[@]}")
EXCLUDE_PATTERNS="[${EXCLUDE_PATTERNS:1}]"  # strip leading comma, wrap in list

# Collect source files tracked in compile_commands.json, excluding third-party dirs
mapfile -t FILES < <(
    python3 -c "
import json
exclude = ${EXCLUDE_PATTERNS}
with open('${COMPILE_COMMANDS}') as f:
    db = json.load(f)
for entry in db:
    src = entry.get('file', '')
    if not any(pat in src for pat in exclude):
        print(src)
"
)

if [ ${#FILES[@]} -eq 0 ]; then
    echo -e "${YELLOW}No source files found to analyse (after excluding third-party dirs).${NC}"
    exit 0
fi

# Header filter: only report diagnostics from project source/include dirs
HEADER_FILTER="${SCRIPT_DIR}/(src|include)/.*"

# ARM toolchain system include paths (needed because compile_commands.json
# targets arm-none-eabi-gcc whose sysroot is unavailable to host clang)
ARM_GCC_VER=$(arm-none-eabi-gcc -dumpversion 2>/dev/null || true)
ARM_EXTRA_ARGS=(
    "--extra-arg=--target=arm-none-eabi"
    "--extra-arg=-isystem/usr/lib/arm-none-eabi/include"
    "--extra-arg=-isystem/usr/lib/gcc/arm-none-eabi/${ARM_GCC_VER}/include"
    "--extra-arg=-isystem/usr/lib/gcc/arm-none-eabi/${ARM_GCC_VER}/include-fixed"
)

echo -e "${GREEN}Running clang-tidy on ${#FILES[@]} file(s)...${NC}"
echo ""

ERRORS=0
for FILE in "${FILES[@]}"; do
    echo "  Analysing: ${FILE#"${SCRIPT_DIR}/"}"
    if ! clang-tidy \
            --config-file="${SCRIPT_DIR}/.clang-tidy" \
            -p "${BUILD_DIR}" \
            --header-filter="${HEADER_FILTER}" \
            "${ARM_EXTRA_ARGS[@]}" \
            "${FILE}"; then
        ERRORS=$((ERRORS + 1))
    fi
done

echo ""
if [ "${ERRORS}" -eq 0 ]; then
    echo -e "${GREEN}Static analysis passed with no errors.${NC}"
else
    echo -e "${RED}Static analysis finished with ${ERRORS} file(s) reporting issues.${NC}"
    exit 1
fi
