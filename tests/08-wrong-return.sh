#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

cat << EOF > /etc/slurm/job_submit.py
import slurm
def job_submit(job_desc, submit_uid):
    return None
EOF

LAST=$(tail -n1 /var/log/slurm/slurmctld.log)

if (
sbatch <<EOF
#! /bin/bash
EOF
); then echo "Job didn't fail to submit when it should: $?"; else echo "Job failed correctly" exit 1; fi

LOG=$(grep -A1000 -F "${LAST}" /var/log/slurm/slurmctld.log)

scancel -u root

if [[ $LOG != *"error: job_submit_python: return value of function must be an integer, not "* ]]; then echo "Error message not found"; exit 1; fi
