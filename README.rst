.. image:: https://travis-ci.com/ACRC/slurm-job-submit-python.svg?branch=master
    :target: https://travis-ci.com/ACRC/slurm-job-submit-python

Python Job Submit plugin for Slurm
==================================

Similar the now built-in plugin for Lua,
this plugin allows the use of Python scripts to control job submission.

For example, a plugin file called ``/etc/slurm/job_submit.py`` with:

.. code-block:: python

   def job_submit(job_desc, submit_uid):
       job_desc.partition = "debug"
       return 0

will set the partition of all jobs to be ``debug``.

Or, a plugin like:

.. code-block:: python

   def job_submit(job_desc, submit_uid):
       job_desc.environment["NEW_ENV_VAR"] = "a new env var"
       return 0

will set an environment variable called ``NEW_ENV_VAR`` with the value
``a new env var`` in all jobs.
