#!/bin/bash
# Check driver include patterns and coding standards

set -e

check_driver_file() {
    local file="$1"
    echo "Checking driver file: $file"
    if ! head -5 "$file" | grep -q "SPDX-License-Identifier:"; then
        echo "Error: Missing SPDX license identifier in $file"
        return 1
    fi
    if [[ "$file" == *.c ]] && [[ "$file" =~ drivers/ ]]; then
        if ! grep -q "DT_DRV_COMPAT" "$file"; then
            echo "Warning: Driver $file may be missing DT_DRV_COMPAT definition"
        fi
        if ! grep -q "LOG_MODULE_REGISTER" "$file"; then
            echo "Warning: Driver $file may be missing LOG_MODULE_REGISTER"
        fi
    fi
    echo "✓ $file checked"
}

exit_code=0
for file in "$@"; do
    if ! check_driver_file "$file"; then
        exit_code=1
    fi
done

exit $exit_code

