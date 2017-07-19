#!/bin/sh

# replace '.git/hooks/prepare-commit-msg' contents with the following lines
# -----------------------------------------------------------------
# !/bin/sh
#
# see 'scripts/gitcheck/prepare-commit-msg' for details
# $(pwd)/scripts/gitcheck/prepare-commit-msg/init.sh $1 $2 $3
# -----------------------------------------------------------------

# for debug
# printf "Work Directory: %s\n" $(pwd)
# set | egrep GIT
# echo "pos-arg-1=$1"
# echo "pos-arg-2=$2"
# echo "pos-arg-3=$3"

GITCHECK_PREPARE_COMMIT_MSG_DIR=$(pwd)/scripts/gitcheck/prepare-commit-msg
GITCHECK_COMMIT_MSG_DIR=$(pwd)/scripts/gitcheck/commit-msg


case "$2,$3" in
    merge,)
        # Comments out the "Conflicts:" part of a merge commit.
        /usr/bin/perl -i.bak -ne 's/^/# /, s/^# #/#/ if /^Conflicts/ .. /#/; print' "$1"
        ;;

    commit,*)
        # git commit --amend
        #echo "\033[31mcommit\033[0m, todo some thing"
        #exit 1
        ;;

    message,*)
        ${GITCHECK_COMMIT_MSG_DIR}/check-msg-fmt.sh $1
        if [ ! $? -eq 0 ] ; then
            echo -e "See: \033[33mcontrib/GitConfig.md\033[0m"
            echo -e "See: \033[33mcontrib/GitCommitStyle.md\033[0m"
            exit 1
        fi
        ;;

    template,)
        #echo "See \033[33mtemplate\033[0m, todo some thing"
        #exit 1
        ;;

    *)
        ;;
esac

# add a Signed-off-by line to the message
# Doing this in a hook is a bad idea in general, but the prepare-commit-msg hook is more suited to it.
#
# Do not do it here, use -s/-S instead
#
# SOB=$(git var GIT_AUTHOR_IDENT | sed -n 's/^\(.*>\).*$/Signed-off-by: \1/p')
# grep -qs "^$SOB" "$1" || echo "$SOB" >> "$1"

if [ ! $? -eq 0 ] ; then exit 1; fi
exit $?

# for debug
echo "[GitHookDebug] prepare-commit-msg => pass"
exit 1
