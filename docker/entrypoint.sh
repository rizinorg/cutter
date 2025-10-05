#!/bin/bash
set -e

USERNAME="cutter"

# Security: Validate UID/GID are within acceptable range
if [ -n "$LOCAL_USER_ID" ] && [ -n "$LOCAL_GROUP_ID" ]; then
    # Ensure UID/GID are numeric and within acceptable range (1000-65535)
    if ! [[ "$LOCAL_USER_ID" =~ ^[0-9]+$ ]] || [ "$LOCAL_USER_ID" -lt 1000 ] || [ "$LOCAL_USER_ID" -gt 65535 ]; then
        echo "ERROR: Invalid LOCAL_USER_ID. Must be numeric between 1000-65535"
        exit 1
    fi
    if ! [[ "$LOCAL_GROUP_ID" =~ ^[0-9]+$ ]] || [ "$LOCAL_GROUP_ID" -lt 1000 ] || [ "$LOCAL_GROUP_ID" -gt 65535 ]; then
        echo "ERROR: Invalid LOCAL_GROUP_ID. Must be numeric between 1000-65535"
        exit 1
    fi
    
    echo "Cutter: Starting with UID:GID $LOCAL_USER_ID:$LOCAL_GROUP_ID"
    usermod -u $LOCAL_USER_ID $USERNAME 2>/dev/null || true
    groupmod -g $LOCAL_GROUP_ID $USERNAME 2>/dev/null || true
else
    echo "Cutter: Starting with default user configuration"
fi

export HOME=/home/$USERNAME

# Security: Drop privileges before executing
exec su-exec $USERNAME "/opt/cutter/build/cutter" "$@"
