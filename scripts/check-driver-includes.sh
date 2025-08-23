#!/bin/bash
# Check driver include patterns and coding standards

set -e

check_driver_file() {
    local file="$1"
    
    echo "Checking driver file: $file"
    
    # Check for proper SPDX license header
    if ! head -5 "$file" | grep -q "SPDX-License-Identifier:"; then
        echo "Error: Missing SPDX license identifier in $file"
        return 1
    fi
    
    # Check for DT_DRV_COMPAT definition in driver .c files
    if [[ "$file" == *.c ]] && [[ "$file" =~ drivers/ ]]; then
        if ! grep -q "DT_DRV_COMPAT" "$file"; then
            echo "Warning: Driver $file may be missing DT_DRV_COMPAT definition"
        fi
    fi
    
    # Check for LOG_MODULE_REGISTER in driver files
    if [[ "$file" == *.c ]] && [[ "$file" =~ drivers/ ]]; then
        if ! grep -q "LOG_MODULE_REGISTER" "$file"; then
            echo "Warning: Driver $file may be missing LOG_MODULE_REGISTER"
        fi
    fi
    
    # Check for proper include ordering (Zephyr convention)
    if [[ "$file" == *.c ]]; then
        # System includes should come before local includes
        local_includes_started=false
        while IFS= read -r line; do
            if [[ "$line" =~ ^#include\ \<zephyr/ ]]; then
                if $local_includes_started; then
                    echo "Warning: Zephyr includes should come before local includes in $file"
                fi
            elif [[ "$line" =~ ^#include\ \" ]]; then
                local_includes_started=true
            fi
        done < "$file"
    fi
    
    echo "✓ $file checked"
    return 0
}

exit_code=0
for file in "$@"; do
    if ! check_driver_file "$file"; then
        exit_code=1
    fi
done

exit $exit_code