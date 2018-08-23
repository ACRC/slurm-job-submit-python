#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

cat << EOF > /etc/slurm/job_submit.py
import slurm
def job_submit(job_desc, submit_uid):
    job_desc['environment'] = "this is a string, not a dict"
    return 0
EOF

LAST=$(tail -n1 /var/log/slurm/slurmctld.log)

sbatch <<EOF
#! /bin/bash
EOF

LOG=$(grep -A1000 -F "${LAST}" /var/log/slurm/slurmctld.log)

scancel -u root

if [[ $LOG != *"error: job_submit_python: Environment field expected a mapping, instead found a"* ]]; then echo "Error message not found"; exit 1; fi
