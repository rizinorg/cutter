#!/bin/bash

if ! [[ $# -eq 3 ]]; then
    echo "Usage: $0 [App name] [background.png] [Volume name]"
    exit 1
fi

APP_NAME=$1
DMG_BACKGROUND_IMG=$2
VOL_NAME=$3

if [[ ! -f ${DMG_BACKGROUND_IMG} ]]; then
    echo "background not found"
    exit 1
fi

echo "Adding link to '/Applications'..."
pushd /Volumes/"${VOL_NAME}" || exit 1
ln -s /Applications " " || exit 1
popd || exit 1

echo "Package background set up..."
mkdir /Volumes/"${VOL_NAME}"/.background || exit 1

cp "${DMG_BACKGROUND_IMG}" /Volumes/"${VOL_NAME}"/.background/ || exit 1

DMG_BACKGROUND_IMG_BASENAME=$(basename "${DMG_BACKGROUND_IMG}")

# tell the Finder to resize the window, set the background,
#  change the icon size, place the icons in the right position, etc.
echo '
   tell application "Finder"
     tell disk "'${VOL_NAME}'"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {400, 100, 925, 508}
           set viewOptions to the icon view options of container window
           set arrangement of viewOptions to not arranged
           set icon size of viewOptions to 122
           set background picture of viewOptions to file ".background:'${DMG_BACKGROUND_IMG_BASENAME}'"
           set position of item "'${APP_NAME}'.app" of container window to {117, 193}
           set position of item " " of container window to {411, 193}
           set position of item ".background" of container window to {1000, 1000}
           set position of item ".fseventsd" of container window to {1010, 1010}
           close
           open
           update without registering applications
           delay 2
     end tell
   end tell
' | osascript || exit 1

echo "Syncing..."
sync || exit 1
