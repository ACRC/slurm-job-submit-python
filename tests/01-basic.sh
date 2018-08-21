#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

cat << EOF > /etc/slurm/job_submit.py
def job_submit(job_desc, submit_uid):
    return 0
EOF

sbatch <<EOF
#! /bin/bash
hostname
EOF
