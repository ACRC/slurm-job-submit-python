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
make clean
make install SLURM_SRC_DIR=/root/slurm

echo "JobSubmitPlugins=python" >> /etc/slurm/slurm.conf
supervisorctl restart slurmctld
echo ""

function run_test()
{
    echo -e "Running $1"
    sleep 2 # to allow log to catch up
    LAST=$(tail -n1 /var/log/slurm/slurmctld.log)

    OUTPUT=$(bash "$1" 2>&1)

    RC=$?

    if [[ $RC -ne 0 ]]; then
        echo "$OUTPUT"
        echo ""

        echo "/var/log/slurm/slurmctld.log:"
        grep -A1000 -F "${LAST}" /var/log/slurm/slurmctld.log
        echo ""
    fi

    return ${RC}
}

PASSED=0
FAILED=0
for test in $(find tests -regex '.*/?[0-9]+-.+\.sh' | sort)
do
    if run_test "$test"; then
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
