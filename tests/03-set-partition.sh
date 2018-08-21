#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

cat << EOF > /etc/slurm/job_submit.py
def job_submit(job_desc, submit_uid):
    job_desc["partition"] = "debug"
    return 0
EOF

JID=$(
sbatch --parsable <<EOF
#! /bin/bash
hostname
EOF
)

PARTITION=$(squeue --states all -j $JID --Format partition --noheader | xargs)

if [[ $PARTITION != "debug" ]]; then echo "Parition should be \"debug\" but is \"$PARTITION\""; exit 1; fi
