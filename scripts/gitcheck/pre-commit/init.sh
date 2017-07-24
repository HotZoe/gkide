#!/bin/sh

# replace '.git/hooks/pre-commit' contents with the following lines
# -----------------------------------------------------------------
# !/bin/sh
#
# see 'scripts/gitcheck/pre-commit' for details
# $(pwd)/scripts/gitcheck/pre-commit/init.sh
# -----------------------------------------------------------------

if git rev-parse --verify HEAD >/dev/null 2>&1; then
    against=HEAD
else
    # Initial commit: diff against an empty tree object
    against=4b825dc642cb6eb9a060e54bf8d69288fbee4904
fi

if [ "${against}" != "HEAD" ]; then
    echo -e "\033[0;33mHEAD\033[0m is detached, do not commit."
    exit 1
fi

GITCHECK_PRE_COMMIT_DIR=$(pwd)/scripts/gitcheck/pre-commit

# check non-ASCII filenames & whitespace errors
${GITCHECK_PRE_COMMIT_DIR}/check-filename.sh
if [ ! $? -eq 0 ] ; then exit 1; fi

# check the changed files contents
${GITCHECK_PRE_COMMIT_DIR}/check-filecontents.sh
if [ ! $? -eq 0 ] ; then exit 1; fi

exit $?

# debug usage
echo "[GitHookDebug] pre-commit => pass"
exit 1

# README
#
# Before Git invokes a hook, it changes its working directory to either the root of the working tree
# in a non-bare repository, or to the `$GIT_DIR` in a bare repository.
#
# printf "Work Directory: %s\n" $(pwd)
# set | egrep GIT

# The pre-commit hook is run first, before you even type in a commit message. If exiting non-zero
# from this hook aborts the commit, although you can bypass it with 'git commit --no-verify'.
#
# It’s used to inspect the snapshot that’s about to be committed
# - to see if you’ve forgotten something
# - to make sure tests run
# - to check for code style, run lint or something equivalent
# - to check for trailing whitespace (the default hook does exactly this)
# - to check for appropriate documentation on new methods
# - to examine whatever you need to inspect in the code

# if error, it may be the difference between 'bash' & 'dash'
# - $ ls -al $(which sh)
# - $ ls -al $(which bash)
# - $ ls -al $(which dash)
