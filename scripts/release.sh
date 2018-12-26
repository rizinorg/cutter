#!/bin/sh
# A small script to go faster when releasing

FILES=".appveyor.yml README.md docs/_config.yml docs/index.md"

# Quick check on arguments
if [ ! "$#" -eq 1 ]; then
    echo "Usage: $0 <release version>"
    exit 1
fi

# Get old version in x.x.x format
old_version=$(git tag | grep v | tail -n1 | cut -c2-)
version="$1"

# Do files modification
git checkout -b "r${version}"
echo "Releasing version '${version}'..."

# Modify version in the global files
for file in ${FILES}; do
    sed -i "s,${old_version},${version},g" "${file}"
done

# Add a row in appdata.xml
n=$(grep -ne 1.7.2 src/org.radare.Cutter.appdata.xml | cut -f1 -d:)
today=$(date +%F)
line="\ \ \ \ <release version=\"${version}\" date=\"${today}\" />"
sed -i "${n}i${line}" ./src/org.radare.Cutter.appdata.xml

# Show difference
git diff

# Commit and push
git add ${FILES} src/org.radare.Cutter.appdata.xml
git commit -m "Release ${version}"
git push origin "r${version}"

# Checkout to master and create tag
# Meanwhile, the branch must be merged into master
sleep 60
git checkout master
git pull
git tag "v${version}"
git push --tags

# Now print changelog...
git log "v${old_version}..HEAD"

