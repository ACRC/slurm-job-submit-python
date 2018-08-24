#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

cat << EOF > /etc/slurm/job_submit.py
import slurm
def job_submit(job_desc, submit_uid):
    slurm.user_msg("special error message")
    slurm.user_msg("error message two")
    return 1
EOF

set +e
MESSAGE=$(
sbatch 2>&1 <<EOF
#! /bin/bash
EOF
)
set -e

scancel -u root

if [[ $MESSAGE != *"special error message"* ]]; then echo "Error message not returned correctly"; exit 1; fi
if [[ $MESSAGE != *"error message two"* ]]; then echo "Error message not returned correctly"; exit 1; fi
