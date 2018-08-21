#! /bin/bash

cd ~
git clone --branch {{SLURM_GIT_TAG}} https://github.com/SchedMD/slurm.git
cd slurm
./configure
cd -
git clone {{ORIGIN}}
cd slurm-job-submit-python
git checkout {{TRAVIS_COMMIT}}
make install SLURM_SRC_DIR=/root/slurm
echo "JobSubmitPlugins=python" >> /etc/slurm/slurm.conf
supervisorctl restart slurmctld

sleep 2 # to allow log to catch up
LAST=$(tail -n1 /var/log/slurm/slurmctld.log)

cat << EOF > /etc/slurm/job_submit.py
def job_submit(job_desc, submit_uid):
    return 0
EOF

sbatch <<EOF
#! /bin/bash
hostname
EOF

RC=$?

echo "/var/log/slurm/slurmctld.log:"
grep -A1000 -F "${LAST}" /var/log/slurm/slurmctld.log

exit ${RC}
