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

BRANCHNAME="${DIRNAME}"
LINETEXT="${BRANCHNAME} from $(date)"
# Assume that content file was cleared before the first call.
sed s/BRANCH/$BRANCHNAME/g snippet-file | sed s/LINETEXT/$LINETEXT/  >> content
sed s/BRANCH/$BRANCHNAME/g snippet-file-manpages | sed s/LINETEXT/$LINETEXT/ >> content-manpages

# Create a tarfile with man pages
echo "searching for man files in $1/all"
cd $1/all
tar cfz ckb-next-man.tar.gz man
# Clean up all other man directories
cd ..
echo "Clearing other man directories"
rm -rf ckb/man ckb-daemon/man
find . -name latex -type d -exec sh -c "cd {} ; mv refman.pdf ../ ; rm -rf ./* ; mv ../refman.pdf ." \;
exit 0

