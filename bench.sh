#! /usr/bin/env bash
# Repeats $2 command n times

run_count="${1:-1000}"
command="$2"

for (( i=0; i<run_count; i++ ))
do
    eval "$command"
done
