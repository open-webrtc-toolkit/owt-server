#!/bin/bash -e
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This tool is used to update libvpx source code with the latest git
# repository.
#
# Make sure you run this in a svn checkout of deps/third_party/libvpx!

# Usage:
#
# $ ./update_libvpx.sh [branch]

# Tools required for running this tool:
#
# 1. Linux / Mac
# 2. svn
# 3. git

# Location for the remote git repository.
GIT_REPO="http://git.chromium.org/webm/libvpx.git"

GIT_BRANCH="master"
LIBVPX_SRC_DIR="source/libvpx"
BASE_DIR=`pwd`

if [ -n "$1" ]; then
  GIT_BRANCH="$1"
fi

prev_hash="$(egrep "Commit: [a-Z0-9]" README.chromium | awk '{ print $2 }')"
echo "prev_hash:$prev_hash"

rm -rf $(svn ls $LIBVPX_SRC_DIR)
svn update $LIBVPX_SRC_DIR

cd $LIBVPX_SRC_DIR

# Make sure git doesn't mess up with svn.
echo ".svn" >> .gitignore

# Start a local git repo.
git init
git add .
git commit -a -m "Current libvpx"

# Add the remote repo.
git remote add origin $GIT_REPO
git fetch

add="$(git diff-index --diff-filter=D origin/$GIT_BRANCH | \
tr -s '\t' ' ' | cut -f6 -d\ )"
delete="$(git diff-index --diff-filter=A origin/$GIT_BRANCH | \
tr -s '\t' ' ' | cut -f6 -d\ )"

# Switch the content to the latest git repo.
git checkout -b tot origin/$GIT_BRANCH

# Output the current commit hash.
hash=$(git log -1 --format="%H")
echo "Current HEAD: $hash"

# Output log for upstream from current hash.
found_prev_hash="$(git log | grep $prev_hash)"
if [ -n "$prev_hash" ]; then
  echo "git log from upstream:"
  pretty_git_log="$(git log --pretty="%h %s" $prev_hash..HEAD)"
  echo "$pretty_git_log"
fi

# Git is useless now, remove the local git repo.
rm -rf .git

# Update SVN with the added and deleted files.
echo "$add" | xargs -I {} svn add --parents {}
echo "$delete" | xargs -I {} svn rm {}

# Find empty directories and remove them from SVN.
find . -type d -empty -not -iwholename '*.svn*' -exec svn rm {} \;

chmod 755 build/make/*.sh build/make/*.pl configure

cd $BASE_DIR
