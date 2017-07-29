#!/usr/bin/env bash

# source GkideEnvSetting.sh

# Logging Level:
# LOGALL(0), TRACE(1), DEBUG(2), STATE(3), ALERT(4), ERROR(5), FATAL(6) or LOGOFF(7)
export NVIM_LOGGING_LEVEL=3
export SNAIL_LOGGING_LEVEL=1

log_level_msg=""

function FUNC_LOG_MSG()
{
    if [ $1 -eq 0 ]; then log_level_msg="LOGALL(0)"; fi
    if [ $1 -eq 1 ]; then log_level_msg="TRACE(1)"; fi
    if [ $1 -eq 2 ]; then log_level_msg="DEBUG(2)"; fi
    if [ $1 -eq 3 ]; then log_level_msg="STATE(3)"; fi
    if [ $1 -eq 4 ]; then log_level_msg="ALERT(4)"; fi
    if [ $1 -eq 5 ]; then log_level_msg="ERROR(5)"; fi
    if [ $1 -eq 6 ]; then log_level_msg="FATAL(6)"; fi
    if [ $1 -eq 7 ]; then log_level_msg="LOGOFF(7)"; fi
}

export NVIM_LOGGING_FILE=${PWD}/build/log.nvim
export SNAIL_LOGGING_FILE=${PWD}/build/log.snail

FUNC_LOG_MSG ${NVIM_LOGGING_LEVEL}
printf "nvim log level : %s\n" ${log_level_msg}
printf "nvim log file  : %s\n" ${NVIM_LOGGING_FILE}

FUNC_LOG_MSG ${SNAIL_LOGGING_LEVEL}
printf "snail log level: %s\n" ${log_level_msg}
printf "snail log file : %s\n" ${SNAIL_LOGGING_FILE}
