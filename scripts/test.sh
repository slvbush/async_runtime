#!/usr/bin/env bash
set -euo pipefail

PRESET_NAME=$1

if [[ ${PRESET_NAME} == *-Sanitized* ]]; then
  export ASAN_OPTIONS=alloc_dealloc_mismatch=0
fi

if [[ ${PRESET_NAME} == *-Debug && -n $(which gdb) && $(uname -s) != MINGW* ]]; then
  gdb -q -return-child-result --batch \
    -ex 'handle SIGHUP nostop pass' \
    -ex 'handle SIGQUIT nostop pass' \
    -ex 'handle SIGPIPE nostop pass' \
    -ex 'handle SIGALRM nostop pass' \
    -ex 'handle SIGTERM nostop pass' \
    -ex 'handle SIGUSR1 nostop pass' \
    -ex 'handle SIGUSR2 nostop pass' \
    -ex 'handle SIGCHLD nostop pass' \
    -ex 'set style enabled on' \
    -ex 'set print frame-arguments all' \
    -ex 'run' \
    -ex 'thread apply all bt -frame-info source-and-location -full' \
    --args "build/${PRESET_NAME}/tests"
else
  "build/${PRESET_NAME}/tests"
fi
