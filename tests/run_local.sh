#! /bin/bash

export SLURM_DOCKER_TAG=17.11.9
export SLURM_GIT_TAG=slurm-17-11-9-2

docker pull giovtorres/docker-centos7-slurm:${SLURM_DOCKER_TAG}

docker run -v "$(git rev-parse --show-toplevel)":/mnt/slurm-job-submit-python:ro -e SLURM_GIT_TAG -h ernie --rm "giovtorres/docker-centos7-slurm:${SLURM_DOCKER_TAG}" /bin/bash /mnt/slurm-job-submit-python/tests/run_tests.sh
