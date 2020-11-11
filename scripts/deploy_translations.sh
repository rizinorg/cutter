#!/bin/sh
#########
#### Push new translation files to cutter-translations
#### so Crowdin can fetch them

log() {
    echo "[TRANSLATIONS] $1"
}

log "Script started"

# Update submodule
log "Updating translations..."
cd ${TRAVIS_BUILD_DIR}/src
git submodule update translations
cd translations
git pull origin master

# Generate Crowdin single translation file from cutter_fr.ts
log "Generating single translation file"
lupdate ../Cutter.pro
cp ./fr/cutter_fr_FR.ts ./Translations.ts

# Push it so Crowdin can find new strings, and later push updated translations
log "Committing..."
git add Translations.ts
git commit -m "Updated translations"
log "Pushing..."
export GIT_SSH_COMMAND="/usr/bin/ssh -i $TRAVIS_BUILD_DIR/scripts/deploy_translations_rsa"
git push "git@github.com:radareorg/cutter-translations.git" HEAD:refs/heads/master

log "Script done!"
