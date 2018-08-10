SLURM_INCLUDE_DIR=/usr/include/slurm
SLURM_SRC_DIR=/mnt/shared/apps/slurm/BUILD/slurm-17.11.8
SLURM_LIBRARY=slurm
PYTHON_INCLUDE_DIR=/usr/include/python3.4m/
PYTHON_LIBRARY=python3.4m

SLURM_PLUGIN_INSTALL_DIR=/usr/lib64/slurm/

CC=gcc
CFLAGS=-shared -fPIC -Wall -std=c99 -O3 -Wfatal-errors

SOURCES=job_submit_python.c
OUTPUT_LIBRARY=job_submit_python.so

all: $(SOURCES) $(OUTPUT_LIBRARY)

$(OUTPUT_LIBRARY): $(SOURCES)
	$(CC) $(SOURCES) -o $@ -I $(SLURM_INCLUDE_DIR) -I $(SLURM_SRC_DIR) -I $(PYTHON_INCLUDE_DIR) -l$(SLURM_LIBRARY) -l$(PYTHON_LIBRARY) $(CFLAGS)

clean:
	rm $(OUTPUT_LIBRARY)

install: all
	install $(OUTPUT_LIBRARY) $(SLURM_PLUGIN_INSTALL_DIR)
