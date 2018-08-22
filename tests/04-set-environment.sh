#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

cat << EOF > /etc/slurm/job_submit.py
def job_submit(job_desc, submit_uid):
    job_desc["environment"]["NEW_ENV_VAR"] = "a new env var"
    return 0
EOF

RC=$(
sbatch --wait <<EOF
#! /bin/bash
env | grep "NEW_ENV_VAR=a new env var"
EOF
)

scancel -u root

if [[ ! $RC ]]; then echo "Environment variable not set correctly"; exit 1; fi
