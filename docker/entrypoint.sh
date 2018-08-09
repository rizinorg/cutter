#!/bin/bash
USERNAME="r2"

echo "Cutter: Starting with UID:GID $LOCAL_USER_ID:$LOCAL_GROUP_ID"
usermod -u $LOCAL_USER_ID $USERNAME
usermod -g $LOCAL_GROUP_ID $USERNAME
export HOME=/home/$USERNAME

exec gosu $USERNAME "/opt/cutter/build/Cutter" $@
