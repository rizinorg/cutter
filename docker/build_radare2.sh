#!/bin/sh

check_r2() {
	r2 -v >/dev/null 2>&1
	if [ $? = 0 ]; then
		R2COMMIT=$(r2 -v | tail -n1 | sed "s,commit: \\(.*\\) build.*,\\1,")
		SUBMODULE=$(git submodule | grep "radare2" | awk '{print $1}')
		if [ "$R2COMMIT" = "$SUBMODULE" ]; then
			return 0
		fi
	fi
	return 1
}

# Build radare2
check_r2
if [ $? -eq 1 ]; then
    printf "A (new?) version of radare2 will be installed. Do you agree? [Y/n] "
    read -r answer
    if [ -z "$answer" ] || [ "$answer" = "Y" ] || [ "$answer" = "y" ]; then
        R2PREFIX=${1:-"/usr"}
        git submodule init && git submodule update
        cd radare2 || exit 1
        ./sys/install.sh "$R2PREFIX"
        cd ..
    else
        echo "Sorry but this script won't work otherwise. Read the README."
        exit 1
    fi
else
    echo "Correct radare2 version found, skipping..."
fi
