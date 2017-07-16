#!/bin/sh
#
# check if we have non-ASCII filenames, if it does then no commit

# If you want to allow non-ASCII filenames set this variable to true.
allownonascii=$(git config --bool hooks.allownonascii)

# Redirect output to stderr.
exec 1>&2

# Cross platform projects tend to avoid non-ASCII filenames; prevent
# them from being added to the repository. We exploit the fact that the
# printable range starts at the space character and ends with tilde.
if [ "$allownonascii" != "true" ] &&
	# Note that the use of brackets around a tr range is ok here, (it's
	# even required, for portability to Solaris 10's /usr/bin/tr), since
	# the square bracket bytes happen to fall in the designated range.
	test $(git diff --cached --name-only --diff-filter=A -z HEAD |
	  LC_ALL=C tr -d '[ -~]\0' | wc -c) != 0
then
    err_msg="
\033[0;31mError\033[0m: Attempt to add a non-ASCII file name.\n\n
This can cause problems if you want to work with people on other platforms.\n\n
To be portable it is advisable to rename the file.\n\n
If you know what you are doing you can disable this check using:\n\n
$ \033[0;33mgit config hooks.allownonascii true\033[0m"
	echo $err_msg
    exit 1
fi

# If there are whitespace errors, print the offending file names and fail.
# Check for introduced trailing whitespace or an indent that uses a space before a tab.
exec git diff-index --check --cached HEAD --
