#!/bin/bash
# Check basic device tree syntax

set -e

check_dt_file() {
    local file="$1"
    
    echo "Checking device tree file: $file"
    
    # Check that the file starts with a comment or root node
    if [[ "$file" == *.dts ]] && ! head -1 "$file" | grep -qE '^(/\*|/)'; then
        echo "Error: $file should start with a comment or root node"
        return 1
    fi
    
    # Check for common issues
    if grep -n "status = \"ok\"" "$file"; then
        echo "Error: Use 'okay' instead of 'ok' for status property in $file"
        return 1
    fi
    
    # Check for missing semicolons (simple heuristic)
    if grep -nE '^\s*[a-zA-Z_-]+\s*=.*[^;{]\s*$' "$file"; then
        echo "Warning: Possible missing semicolons in $file"
    fi
    
    echo "✓ $file looks good"
    return 0
}

exit_code=0
for file in "$@"; do
    if ! check_dt_file "$file"; then
        exit_code=1
    fi
done

exit $exit_code