#!/bin/bash

####
# Script to search for all branches with documentation
####

# Clean up to start with new files
rm -f content{,html,pdf,man}

# find all directories and call html-gen2.sh for each of them
# Filtering for unwanted dirs occurs in the script called
find . -maxdepth 1 -type d -exec ./html-gen2.sh {} \;

# echo Now change the result for html and pdf links
sed s/FORMATDEFINITION/html/g content > contenthtml
sed s\!FORMATDEFINITION\!latex/refman.pdf\!g content | sed s/_blank/_self/g > contentpdf
sed s\!FORMATDEFINITION\!ckb-next-man.tar.gz\!g content-manpages | sed s/_blank/_self/g > contentman

# echo replace the searchstring with the new content
awk 'NR==FNR { a[n++]=$0; next }
/PUT_SECTION_HERE/ { for (i=0;i<n;++i) print a[i]; next }
1' contenthtml pres-html.html > /tmp/xx && mv /tmp/xx pres-html.html

awk 'NR==FNR { a[n++]=$0; next }
/PUT_SECTION_HERE/ { for (i=0;i<n;++i) print a[i]; next }
1' contentpdf pres-pdf.html > /tmp/xx && mv /tmp/xx pres-pdf.html

awk 'NR==FNR { a[n++]=$0; next }
/PUT_SECTION_HERE/ { for (i=0;i<n;++i) print a[i]; next }
1' contentman pres-man.html > /tmp/xx && mv /tmp/xx pres-man.html

exit 0
