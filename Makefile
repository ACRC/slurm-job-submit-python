SLURM_INCLUDE_DIR ?= /usr/include/slurm
SLURM_SRC_DIR ?= /mnt/shared/apps/slurm/BUILD/slurm-17.11.8
PYTHON_CONFIG ?= python3-config
PYTHON_INCLUDE_FLAGS=$(shell $(PYTHON_CONFIG) --includes)
PYTHON_LIBRARY_FLAGS=$(shell $(PYTHON_CONFIG) --libs)

SLURM_PLUGIN_INSTALL_DIR=/usr/lib64/slurm/
SLURM_SCRIPT_DIR=/etc/slurm

CC=gcc
CFLAGS=-shared -fPIC -Wall -std=c99 -O3 -Wfatal-errors -DDEFAULT_SCRIPT_DIR=\"$(SLURM_SCRIPT_DIR)\"

SOURCES=job_submit_python.c
OUTPUT_LIBRARY=job_submit_python.so

all: $(SOURCES) $(OUTPUT_LIBRARY)

$(OUTPUT_LIBRARY): $(SOURCES)
	$(CC) $(SOURCES) -o $@ -I $(SLURM_INCLUDE_DIR) -I $(SLURM_SRC_DIR) $(PYTHON_INCLUDE_FLAGS) $(PYTHON_LIBRARY_FLAGS) $(CFLAGS)

clean:
	rm $(OUTPUT_LIBRARY)

install: all
	install $(OUTPUT_LIBRARY) $(SLURM_PLUGIN_INSTALL_DIR)

test:
	tests/run_local.sh
