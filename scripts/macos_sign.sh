#!/bin/bash
#
# Script to be run manually on a maintainer's machine to re-sign
# and notarize Cutter dmgs created by the CI with Developer ID.
#
# https://developer.apple.com/forums/thread/701514

set -e
shopt -s extglob

usage() {
	echo "macos_sign.sh [command] ..."
	echo ""
	echo "Commands:"
	echo "  sign_bundle [Cutter.app]	 Sign the given bundle in-place"
	echo "  notarize_bundle [Cutter.app] Notarize the given bundle after it has been signed and staple the ticket"
	echo "  resign_dmg [Cutter.dmg]	  Sign and notarize the Cutter.app in the given dmg into a new dmg"
	exit 1
}


TARGET=Cutter.app
IDENT="Developer ID Application: Florian Märkl (7C89959B9X)"
ENTITLEMENTS="$(dirname "${BASH_SOURCE[0]}")/../dist/macos/Entitlements.plist"

ee() {
	echo "$@"
	"$@"
}

echo_step() {
	echo ""
	printf "\033[0;32m----------- $@ -----------\033[0m\n"
}

sign_files () {
	local ARGS="$1"
	shift 1
	for f in $@; do
		ee codesign -s "$IDENT" $ARGS --timestamp  -f "$f"
	done
}

sign_bundle() {
	# Sign "from the inside out" with Developer ID
	# Libs need only a signature
	# Executables need -o runtime for notarization
	# Debugging executables also need entitlements

	local TARGET=$1
	echo_step "Signing ${TARGET}"

	local PYTHON_PREFIX=${TARGET}/Contents/Frameworks/Python.framework/Versions/*.*
	sign_files "" \
		${PYTHON_PREFIX}/lib/**/**/*.so \
		${PYTHON_PREFIX}/lib/*.dylib \
		${PYTHON_PREFIX}/lib/python*/site-packages/PySide6/*.so \
		${PYTHON_PREFIX}/lib/python*/site-packages/shiboken6/*.so
	sign_files "-o runtime" \
		${TARGET}/Contents/Resources/bin/!(rizin) \
		${PYTHON_PREFIX}/bin/python*.!(*-config)

	sign_files "" \
		${TARGET}/Contents/Frameworks/*.framework \
		${TARGET}/Contents/Frameworks/*.dylib \
		${TARGET}/Contents/PlugIns/**/*.dylib \
		${TARGET}/Contents/Resources/lib/*.dylib \
		${TARGET}/Contents/Resources/lib/rizin/plugins/*.dylib \
		${TARGET}/Contents/Resources/plugins/native/*.so \

	sign_files "-o runtime" \
		${TARGET}/Contents/Resources/bin/!(rizin)

	sign_files "--entitlements ${ENTITLEMENTS} -o runtime" \
		${TARGET}/Contents/Resources/bin/rizin \
		${TARGET}
}

notarize_bundle() {
	# Save credentials with:
	# xcrun notarytool store-credentials --apple-id 'apple@rizin.re' --team-id 7C89959B9X notarytool-7C89959B9X
	local AUTH="--keychain-profile notarytool-7C89959B9X"
	local TARGET=$1

	echo_step "Notarizing ${TARGET}"

	ee ditto -c -k --keepParent ${TARGET} Cutter-notarize-submit.zip
	ee xcrun notarytool submit --wait --timeout 30m ${AUTH} Cutter-notarize-submit.zip
	# TODO: if possible, fail the script here if the notarization failed. Unfortunately notarytool does not
	# return a non-zero exit code by default on failure, so it does not work automatically yet.
	# However, the staple below will still fail if the submission was not notarized, so we still detect it.
	ee xcrun stapler staple ${TARGET}
}

resign_dmg() {
	local TARGET="$1"
	echo_step "Mounting temporary rw variant of ${TARGET} to Cutter-rw/"
	rm -f Cutter-rw.dmg
	ee hdiutil convert -format UDRW -o Cutter-rw.dmg "${TARGET}"
	mkdir -p Cutter-rw
	ee hdiutil attach Cutter-rw.dmg -mount required -mountpoint Cutter-rw
	unmount() {
		ee hdiutil detach Cutter-rw
	}
	trap unmount EXIT
	sign_bundle Cutter-rw/Cutter.app
	notarize_bundle Cutter-rw/Cutter.app
	unmount
	trap - EXIT
	OUTPUT="${1%.*}-signed.dmg"
	echo_step "Creating final read-only ${OUTPUT}"
	ee hdiutil convert -format UDZO -o "${OUTPUT}" Cutter-rw.dmg
}

case "$1" in
	sign_bundle)
		if [ "$#" -ne 2 ]; then
			usage
		fi
		sign_bundle "$2"
	;;
	notarize_bundle)
		if [ "$#" -ne 2 ]; then
			usage
		fi
		notarize_bundle "$2"
	;;
	resign_dmg)
		if [ "$#" -ne 2 ]; then
			usage
		fi
		resign_dmg "$2"
	;;
	*)
		usage
	;;
esac

