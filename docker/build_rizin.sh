#!/bin/sh

check_rizin() {
	rizin -v >/dev/null 2>&1
	if [ $? = 0 ]; then
		RZCOMMIT=$(rizin -v | tail -n1 | sed "s,commit: \\(.*\\) build.*,\\1,")
		SUBMODULE=$(git submodule | grep "rizin" | awk '{print $1}')
		if [ "$RZCOMMIT" = "$SUBMODULE" ]; then
			return 0
		fi
	fi
	return 1
}

# Build rizin
check_rizin
if [ $? -eq 1 ]; then
    printf "A (new?) version of rizin will be installed. Do you agree? [Y/n] "
    read -r answer
    if [ -z "$answer" ] || [ "$answer" = "Y" ] || [ "$answer" = "y" ]; then
        RZPREFIX=${1:-"/usr"}
        git submodule init && git submodule update
        cd rizin || exit 1
        ./sys/install.sh "$RZPREFIX"
        cd ..
    else
        echo "Sorry but this script won't work otherwise. Read the README."
        exit 1
    fi
else
    echo "Correct rizin version found, skipping..."
fi
