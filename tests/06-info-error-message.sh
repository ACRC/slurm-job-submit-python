#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

cat << EOF > /etc/slurm/job_submit.py
import slurm
def job_submit(job_desc, submit_uid):
    slurm.info("this is the helpful message")
    slurm.error("this is a bad message")
    return 0
EOF

LAST=$(tail -n1 /var/log/slurm/slurmctld.log)

sbatch <<EOF
#! /bin/bash
EOF

LOG=$(grep -A1000 -F "${LAST}" /var/log/slurm/slurmctld.log)

scancel -u root

if [[ $LOG != *"job_submit_python: this is the helpful message"* ]]; then echo "Info message not logged correctly"; exit 1; fi
if [[ $LOG != *"error: job_submit_python: this is a bad message"* ]]; then echo "Error message not logged correctly"; exit 1; fi
