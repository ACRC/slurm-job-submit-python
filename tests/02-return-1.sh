#!/bin/bash

cat << EOF > /etc/slurm/job_submit.py
def job_submit(job_desc, submit_uid):
    return -1
EOF

sbatch <<EOF
#! /bin/bash
hostname
EOF

RC=$?

if [[ "$RC" -ne 1 ]]; then echo "sbatch returned wrong value: $RC instead of 1"; exit 1; fi
