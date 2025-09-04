#!/bin/bash
# Check basic device tree syntax

set -e

check_dt_file() {
    local file="$1"
    echo "Checking device tree file: $file"
    # Warn on common issues
    if grep -n "status = \"ok\"" "$file"; then
        echo "Error: Use 'okay' instead of 'ok' for status property in $file"
        return 1
    fi
    if grep -nE '^\s*[a-zA-Z_-]+\s*=.*[^;{]\s*$' "$file"; then
        echo "Warning: Possible missing semicolons in $file"
    fi
    echo "✓ $file looks good"
}

exit_code=0
for file in "$@"; do
    if ! check_dt_file "$file"; then
        exit_code=1
    fi
done

exit $exit_code

