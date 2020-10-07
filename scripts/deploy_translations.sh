#!/bin/sh

#######################
#### Push new translation files

# Update submodule
cd ${TRAVIS_BUILD_DIR}/src
git submodule update translations
cd translations
git pull origin master

# Generate Crowdin single translation file from cutter_fr.ts
lupdate ../Cutter.pro
cp ./fr/cutter_fr_FR.ts ./Translations.ts

# Push it so Crowdin can find new strings, and later push updated translations
git add Translations.ts
git commit -m "Updated translations"
export GIT_SSH_COMMAND="/usr/bin/ssh -i $TRAVIS_BUILD_DIR/scripts/deploy_translations_rsa"
echo "Pushing new translation strings..."
git push "git@github.com:radareorg/cutter-translations.git" HEAD:refs/heads/master
