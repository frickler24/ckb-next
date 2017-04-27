#!/bin/bash

####
# Script to generate a link file for a given directory name
####

# Check for dirnames to be ignored
IFS=$'\n'
for elem in $(cat ${TRAVIS_BUILD_DIR}/documentation/dirs_to_ignore) ; do
	if [ x$1 == x${elem} ] ; then
		echo "Ignoring directory ${elem}"
		exit 0
	fi
done

# dirnames are in ./directoy but we need directory only
DIRNAME=$(echo $1 | cut -d/ -f2)
echo "Generating link for ${DIRNAME}"

# Assume that content file was cleared before the first call.
sed s/BRANCH/$DIRNAME/g snippet-file >> content
sed s/BRANCH/$DIRNAME/g snippet-file-manpages >> content-manpages

# Create a tarfile with man pages
echo "searching for man files in $1/all"
cd $1/all
tar cfz ckb-next-man.tar.gz man
rm -rf man/
# Clean up all other man directories
cd ..
find . -name man -type d -exec rm -rf {} \;
find . -name latex -type d -exec sh -c "cd {} ; mv refman.pdf ../ ; rm -rf ./* ; mv ../refman.pdf ." \;
exit 0

