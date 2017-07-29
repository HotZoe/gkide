#!/usr/bin/env bash

# replace '.git/hooks/commit-msg' contents with the following lines
# -----------------------------------------------------------------
# !/bin/sh
#
# see 'scripts/gitcheck/commit-msg' for details
# $(pwd)/scripts/gitcheck/commit-msg/init.sh $1
# -----------------------------------------------------------------

# script to check the commit log message.
#
# The hook is allowed to edit the commit message file.
# Exit with non-zero status after issuing an appropriate message if it wants to stop the commit.

function backup_msg()
{
    msg=$(cat ${1})

    if [ -z "${msg}" ] ; then
        return 0
    fi

    tmp_msg_file=$(pwd)/tmp/tmp-commit-msg
    if [ ! -d $(pwd)/tmp ]; then mkdir $(pwd)/tmp; fi
    if [ -f ${tmp_msg_file} ]; then rm ${tmp_msg_file}; fi
    cat "${1}" >> ${tmp_msg_file}

    usr_msg=$(grep "^[^#]" "${tmp_msg_file}")

    if [ "${usr_msg}" != "" ]; then
        echo "${usr_msg}" > ${tmp_msg_file}
    fi

    head_1_msg=$(head -n1 ${tmp_msg_file})
    tail_1_msg=$(tail -n1 ${tmp_msg_file})

    if [ "${head_1_msg}" = "${tail_1_msg}" ]; then
        usr_msg=$(grep "^Signed-off-by: .*" "${tmp_msg_file}")

        # check if we forget signed-off-by, but input commit message, keep it
        if [ "${usr_msg}" != "" ]; then
            # only signed off line, just remove it
            rm ${tmp_msg_file}
            return 1
        fi
    fi

    echo -e "The input commit message save to: \033[33m${tmp_msg_file}\033[0m"
}

GITCHECK_COMMIT_MSG_DIR=$(pwd)/scripts/gitcheck/commit-msg

# check the signed off
${GITCHECK_COMMIT_MSG_DIR}/check-signed-off-by.sh $1
if [ ! $? -eq 0 ] ; then backup_msg ${1}; exit 1; fi

# check the commit message format
${GITCHECK_COMMIT_MSG_DIR}/check-msg-fmt.sh $1
if [ ! $? -eq 0 ] ; then backup_msg ${1}; exit 1; fi

# check the commit message spell
# ${GITCHECK_COMMIT_MSG_DIR}/check-spell.sh $1
# if [ ! $? -eq 0 ] ; then backup_msg ${1}; exit 1; fi

exit $?

# debug usage
echo "[GitHookDebug] commit-msg => pass"
exit 1
