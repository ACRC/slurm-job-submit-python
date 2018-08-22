#! /bin/bash
set -euo pipefail
IFS=$'\n\t'

cd ~

git clone --depth 1 --branch "$SLURM_GIT_TAG" https://github.com/SchedMD/slurm.git
cd slurm
./configure
cd -

cp -r /mnt/slurm-job-submit-python .
cd slurm-job-submit-python
make install SLURM_SRC_DIR=/root/slurm

echo "JobSubmitPlugins=python" >> /etc/slurm/slurm.conf
supervisorctl restart slurmctld

function run_test()
{
    echo -e "###############################"
    echo -e "Running $1"
    echo -e '###############################\n'
    sleep 2 # to allow log to catch up
    LAST=$(tail -n1 /var/log/slurm/slurmctld.log)

    bash "$1"

    RC=$?

    echo "/var/log/slurm/slurmctld.log:"
    grep -A1000 -F "${LAST}" /var/log/slurm/slurmctld.log

    echo ""
    return ${RC}
}

PASSED=0
FAILED=0
for test in $(ls tests | grep -E '.*[[:digit:]]+\-.*\.sh')
do
    if run_test "tests/$test"; then
        echo -e '\e[32mTest passed ✔\e[0m\n'
        PASSED=$((PASSED+1))
    else
        echo -e '\e[31mTest failed ✖\e[0m\n'
        FAILED=$((FAILED+1))
    fi
done

if [[ $FAILED -gt 0 ]]; then echo -ne '\e[31m'; else echo -e '\e[32m'; fi
echo "${PASSED} passed, ${FAILED} failed"
echo -e '\e[0m'

exit ${FAILED}
