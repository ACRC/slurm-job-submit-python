#!/bin/bash

cat << EOF > /etc/slurm/job_submit.py
def job_submit(job_desc, submit_uid):
    return -1
EOF

sbatch <<EOF
#! /bin/bash
hostname
EOF

if [[ "$?" -ne 1 ]]; then echo "sbatch returned wrong value"; exit 1; fi
