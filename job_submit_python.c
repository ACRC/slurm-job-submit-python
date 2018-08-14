/*****************************************************************************\
 *  job_submit_python.c - Set defaults in job submit request specifications.
 *****************************************************************************
 *  Copyright (C) 2018 Matt Williams <matt.williams@bristol.ac.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
\*****************************************************************************/

#include <Python.h>

#include "slurm/slurm.h"
#include "slurm/slurm_errno.h"

#include "src/common/read_config.h"
#include "src/common/xstring.h"
#include "src/common/xmalloc.h"
#include "src/slurmctld/slurmctld.h"

#include <stdbool.h>

const char plugin_name[]="Job submit Python plugin";
const char plugin_type[] = "job_submit/python";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;

int init(void)
{
	Py_Initialize();

	// Append the script_directory to the Python path
	char script_directory[] = "/usr/lib64/slurm/";
	PyObject* sysPath = PySys_GetObject((char*)"path");
	PyObject* programName = PyUnicode_FromString(script_directory);
	PyList_Append(sysPath, programName);
	Py_DECREF(programName);

	//verbose("job_submit_python: initialised");

	return SLURM_SUCCESS;
}

int fini(void)
{
	Py_Finalize();
	return SLURM_SUCCESS;
}

void print_python_error()
{
	if (PyErr_Occurred())
	{
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);
		//PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

		PyObject *pTracebackModuleName = PyUnicode_FromString("traceback");
		PyObject *pTracebackModule = PyImport_Import(pTracebackModuleName);
		Py_DECREF(pTracebackModuleName);

		PyObject* pFormatTbFn = PyObject_GetAttrString(pTracebackModule, "format_tb");
		Py_DECREF(pTracebackModule);

		PyObject* pFormattedTb = PyObject_CallFunctionObjArgs(pFormatTbFn, ptraceback, NULL);
		Py_DECREF(pFormatTbFn);
		Py_XDECREF(ptraceback);

		PyObject* pFormattedTbStr = PyObject_Str(pFormattedTb);

		error("job_submit_python: %s", PyUnicode_AsUTF8(pFormattedTbStr));
		error("job_submit_python: %s: %s", PyUnicode_AsUTF8(ptype), PyUnicode_AsUTF8(pvalue));

		Py_DECREF(pFormattedTbStr);
		Py_XDECREF(pvalue);
		Py_DECREF(ptype);

		PyErr_Clear();
	}
}

void insert_object(PyObject* dict, char* name, PyObject* obj)
{
	if (obj != NULL)
	{
		PyDict_SetItemString(dict, name, obj);
		Py_DECREF(obj);
	}
	else
	{
		error("Could not convert job description entry %s", name);
		print_python_error();
	}
}

PyObject* char_star_star_to_python(uint32_t num_strings, char** str_list)
{
	PyObject* list = PyList_New(num_strings);

	for (int i=0; i < num_strings; ++i)
	{
		PyObject* str = PyUnicode_FromString(str_list[i]);
		PyList_SetItem(list, i, str);
	}

	return list;
}

PyObject* char_star_star_to_python_dict(uint32_t num_strings, char** str_list)
{
	PyObject* dict = PyDict_New();

	for (int i=0; i < num_strings; ++i)
	{
		char* eq = xstrchr(str_list[i], '=');
		size_t eq_position = (size_t)(eq - str_list[i]);
		PyObject* str_val = PyUnicode_FromString(str_list[i] + eq_position + 1);
		PyDict_SetItemString(dict, xstrndup(str_list[i], eq_position), str_val);
	}

	return dict;
}

#define insert_char_star(job_desc, dict, name) do { if(job_desc->name != NULL) insert_object(dict, #name, PyUnicode_FromString(job_desc->name)); else insert_object(dict, #name, Py_None); } while (0)
#define insert_char_star_star(job_desc, dict, name, count) do { if(job_desc->name != NULL) insert_object(dict, #name, char_star_star_to_python(job_desc->count, job_desc->name)); else insert_object(dict, #name, Py_None); } while (0)
#define insert_environment_dict(job_desc, dict, name, count) do { if(job_desc->name != NULL) insert_object(dict, #name, char_star_star_to_python_dict(job_desc->count, job_desc->name)); else insert_object(dict, #name, Py_None); } while (0)
#define insert_uint8_t(job_desc, dict, name) do { if(job_desc->name != NO_VAL8) insert_object(dict, #name, PyLong_FromUnsignedLong(job_desc->name)); else insert_object(dict, #name, Py_None); } while (0)
#define insert_uint16_t(job_desc, dict, name) do { if(job_desc->name != NO_VAL16) insert_object(dict, #name, PyLong_FromUnsignedLong(job_desc->name)); else insert_object(dict, #name, Py_None); } while (0)
#define insert_uint32_t(job_desc, dict, name) do { if(job_desc->name != NO_VAL) insert_object(dict, #name, PyLong_FromUnsignedLong(job_desc->name)); else insert_object(dict, #name, Py_None); } while (0)
#define insert_uint64_t(job_desc, dict, name) do { if(job_desc->name != NO_VAL64) insert_object(dict, #name, PyLong_FromUnsignedLongLong(job_desc->name)); else insert_object(dict, #name, Py_None); } while (0)
#define insert_time_t(job_desc, dict, name) do { insert_object(dict, #name, PyLong_FromUnsignedLong(job_desc->name)); } while (0)
#define insert_uint8_t_to_bool(job_desc, dict, name) do { if(job_desc->name != NO_VAL8) {insert_object(dict, #name, PyBool_FromLong(job_desc->name));} else insert_object(dict, #name, Py_None); } while (0)
#define insert_uint16_t_to_bool(job_desc, dict, name) do { if(job_desc->name != NO_VAL16) {insert_object(dict, #name, PyBool_FromLong(job_desc->name));} else insert_object(dict, #name, Py_None); } while (0)

PyObject* create_job_desc_dict(struct job_descriptor *job_desc)
{
	PyObject* pJobDesc = PyDict_New();

	insert_char_star(job_desc, pJobDesc, account);
	insert_char_star(job_desc, pJobDesc, acctg_freq);
	insert_char_star(job_desc, pJobDesc, admin_comment);
	insert_char_star(job_desc, pJobDesc, alloc_node);
	insert_uint16_t(job_desc, pJobDesc, alloc_resp_port);
	insert_uint32_t(job_desc, pJobDesc, alloc_sid);
	insert_char_star_star(job_desc, pJobDesc, argv, argc);
	insert_char_star(job_desc, pJobDesc, array_inx);
	//insert_char_star(job_desc, pJobDesc, array_bitmap); // void*
	insert_time_t(job_desc, pJobDesc, begin_time);
	insert_uint32_t(job_desc, pJobDesc, bitflags);
	insert_char_star(job_desc, pJobDesc, burst_buffer);
	insert_uint16_t(job_desc, pJobDesc, ckpt_interval);
	insert_char_star(job_desc, pJobDesc, ckpt_dir);
	insert_char_star(job_desc, pJobDesc, clusters);
	insert_char_star(job_desc, pJobDesc, cluster_features);
	insert_char_star(job_desc, pJobDesc, comment);
	insert_uint16_t_to_bool(job_desc, pJobDesc, contiguous);
	insert_uint16_t(job_desc, pJobDesc, core_spec);
	insert_char_star(job_desc, pJobDesc, cpu_bind);
	insert_uint16_t(job_desc, pJobDesc, cpu_bind_type);
	insert_uint32_t(job_desc, pJobDesc, cpu_freq_min);
	insert_uint32_t(job_desc, pJobDesc, cpu_freq_max);
	insert_uint32_t(job_desc, pJobDesc, cpu_freq_gov);
	insert_time_t(job_desc, pJobDesc, deadline);
	insert_uint32_t(job_desc, pJobDesc, delay_boot);
	insert_char_star(job_desc, pJobDesc, dependency);
	insert_time_t(job_desc, pJobDesc, end_time);
	insert_environment_dict(job_desc, pJobDesc, environment, env_size);
	insert_uint32_t(job_desc, pJobDesc, env_size);
	insert_char_star(job_desc, pJobDesc, extra);
	insert_char_star(job_desc, pJobDesc, exc_nodes);
	insert_char_star(job_desc, pJobDesc, features);
	insert_uint64_t(job_desc, pJobDesc, fed_siblings_active);
	insert_uint64_t(job_desc, pJobDesc, fed_siblings_viable);
	insert_char_star(job_desc, pJobDesc, gres);
	insert_uint32_t(job_desc, pJobDesc, group_id);
	insert_uint16_t_to_bool(job_desc, pJobDesc, immediate);
	insert_uint32_t(job_desc, pJobDesc, job_id);
	insert_char_star(job_desc, pJobDesc, job_id_str);
	insert_uint16_t_to_bool(job_desc, pJobDesc, kill_on_node_fail);
	insert_char_star(job_desc, pJobDesc, licenses);
	insert_uint16_t(job_desc, pJobDesc, mail_type);
	insert_char_star(job_desc, pJobDesc, mail_user);
	insert_char_star(job_desc, pJobDesc, mcs_label);
	insert_char_star(job_desc, pJobDesc, mem_bind);
	insert_uint16_t(job_desc, pJobDesc, mem_bind_type);
	insert_char_star(job_desc, pJobDesc, name);
	insert_char_star(job_desc, pJobDesc, network);
	insert_uint32_t(job_desc, pJobDesc, nice);
	insert_uint32_t(job_desc, pJobDesc, num_tasks);
	insert_uint8_t(job_desc, pJobDesc, open_mode);
	insert_char_star(job_desc, pJobDesc, origin_cluster);
	insert_uint16_t(job_desc, pJobDesc, other_port);
	insert_uint8_t_to_bool(job_desc, pJobDesc, overcommit);
	insert_uint32_t(job_desc, pJobDesc, pack_job_offset);
	insert_char_star(job_desc, pJobDesc, partition);
	insert_uint16_t(job_desc, pJobDesc, plane_size);
	insert_uint8_t(job_desc, pJobDesc, power_flags);
	insert_uint32_t(job_desc, pJobDesc, priority);
	insert_uint32_t(job_desc, pJobDesc, profile);
	insert_char_star(job_desc, pJobDesc, qos);
	insert_uint16_t_to_bool(job_desc, pJobDesc, reboot);
	insert_char_star(job_desc, pJobDesc, resp_host);
	insert_uint16_t(job_desc, pJobDesc, restart_cnt);
	insert_char_star(job_desc, pJobDesc, req_nodes);
	insert_uint16_t_to_bool(job_desc, pJobDesc, requeue);
	insert_char_star(job_desc, pJobDesc, reservation);
	insert_char_star(job_desc, pJobDesc, script);
	insert_uint16_t(job_desc, pJobDesc, shared);
	insert_char_star_star(job_desc, pJobDesc, spank_job_env, spank_job_env_size);
	insert_uint32_t(job_desc, pJobDesc, task_dist);
	insert_uint32_t(job_desc, pJobDesc, time_limit);
	insert_uint32_t(job_desc, pJobDesc, time_min);
	insert_uint32_t(job_desc, pJobDesc, user_id);
	insert_uint16_t_to_bool(job_desc, pJobDesc, wait_all_nodes);
	insert_uint16_t(job_desc, pJobDesc, warn_flags);
	insert_uint16_t(job_desc, pJobDesc, warn_signal);
	insert_uint16_t(job_desc, pJobDesc, warn_time);
	insert_char_star(job_desc, pJobDesc, work_dir);
	insert_uint16_t(job_desc, pJobDesc, cpus_per_task);
	insert_uint32_t(job_desc, pJobDesc, min_cpus);
	insert_uint32_t(job_desc, pJobDesc, max_cpus);
	insert_uint32_t(job_desc, pJobDesc, min_nodes);
	insert_uint32_t(job_desc, pJobDesc, max_nodes);
	insert_uint16_t(job_desc, pJobDesc, boards_per_node);
	insert_uint16_t(job_desc, pJobDesc, sockets_per_board);
	insert_uint16_t(job_desc, pJobDesc, sockets_per_node);
	insert_uint16_t(job_desc, pJobDesc, cores_per_socket);
	insert_uint16_t(job_desc, pJobDesc, threads_per_core);
	insert_uint16_t(job_desc, pJobDesc, ntasks_per_node);
	insert_uint16_t(job_desc, pJobDesc, ntasks_per_socket);
	insert_uint16_t(job_desc, pJobDesc, ntasks_per_core);
	insert_uint16_t(job_desc, pJobDesc, ntasks_per_board);
	insert_uint16_t(job_desc, pJobDesc, pn_min_cpus);
	insert_uint64_t(job_desc, pJobDesc, pn_min_memory);
	insert_uint32_t(job_desc, pJobDesc, pn_min_tmp_disk);
	insert_uint32_t(job_desc, pJobDesc, req_switch);
	//select_jobinfo
	insert_char_star(job_desc, pJobDesc, std_err);
	insert_char_star(job_desc, pJobDesc, std_in);
	insert_char_star(job_desc, pJobDesc, std_out);
	//insert_uint64_t_star(job_desc, pJobDesc, tres_req_cnt);
	insert_uint32_t(job_desc, pJobDesc, wait4switch);
	insert_char_star(job_desc, pJobDesc, wckey);
	insert_uint16_t(job_desc, pJobDesc, x11);
	insert_char_star(job_desc, pJobDesc, x11_magic_cookie);
	insert_uint16_t(job_desc, pJobDesc, x11_target_port);

	return pJobDesc;
}

void clear_char_star_star(uint32_t* num_strings_p, char*** str_list_p)
{
	for (int i = 0; i < *num_strings_p; ++i)
	{
		xfree((*str_list_p)[i]);
	}
	xfree(*str_list_p);
	*num_strings_p = 0;
}


// Given an array of strings, move any NULL strings to the end
void defragment_array(uint32_t num_strings, char** str_list)
{
	for (int empty_finder = 0; empty_finder < num_strings; ++empty_finder)
	{
		if (str_list[empty_finder] == NULL)
		{
			for (int back_pointer = num_strings-1; back_pointer > empty_finder; --back_pointer)
			{
				if (str_list[back_pointer] != NULL)
				{
					info("Doing a swap");
					str_list[empty_finder] = str_list[back_pointer];
					str_list[back_pointer] = NULL;
					break;
				}
			}
		}
	}
}

void python_dict_to_environment(PyObject* obj, uint32_t* num_strings_p, char*** str_list_p)
{
	if (obj == Py_None)
	{
		clear_char_star_star(num_strings_p, str_list_p);
                return;
	}

	if (!PyMapping_Check(obj))
	{
		//TODO environment is not a mapping
	}

	uint32_t p_length = PyMapping_Length(obj);

	for (int i = 0; i < (*num_strings_p); ++i)
	{
		char* eq = xstrchr((*str_list_p)[i], '=');
		size_t eq_position = (size_t)(eq - (*str_list_p)[i]);
		char* key = xstrndup((*str_list_p)[i], eq_position);
		char* value = (*str_list_p)[i] + eq_position + 1;
		if (PyMapping_HasKeyString(obj, key))
		{
			// The key is still there but we must check if the value has been changed
			PyObject* p_value = PyMapping_GetItemString(obj, key);
			PyMapping_DelItemString(obj, key);
			PyObject* p_str = PyObject_Str(p_value);
			Py_DECREF(p_value);
			bool value_changed = PyUnicode_CompareWithASCIIString(p_str, value) != 0;
			if (value_changed)
			{
				xfree((*str_list_p)[i]);
				(*str_list_p)[i] = xstrdup_printf("%s=%s", key, PyUnicode_AsUTF8(p_str));
			}
			Py_DECREF(p_str);
		}
		else
		{
			// Key has been removed by the user so remove it from the job descriptor
			xfree((*str_list_p)[i]);
			(*str_list_p)[i] = NULL;
		}
	}

	// If we have removed entries from str_list_p, we must defragment it to move the gaps to the end
	defragment_array(*num_strings_p, *str_list_p);

	// resize str_list_p to the original size of the dict with realloc
	*num_strings_p = p_length;
	*str_list_p = xrealloc(*str_list_p, (*num_strings_p)*sizeof(char*));

	// Slot in the remaining elements of the dict into the string list
	PyObject* remaining_items = PyMapping_Items(obj);
	for (int i = 0; i < PyMapping_Length(remaining_items); ++i)
	{
		PyObject* item = PyList_GetItem(remaining_items, i);
		char* key = PyUnicode_AsUTF8(PyTuple_GetItem(item, 0));
		PyObject* p_value = PyTuple_GetItem(item, 1);
		PyObject* p_str = PyObject_Str(p_value);
		Py_DECREF(p_value);

		const int new_index = (*num_strings_p) - PyMapping_Length(obj) + i;

		(*str_list_p)[new_index] = xstrdup_printf("%s=%s", key, PyUnicode_AsUTF8(p_str));
	}
	Py_DECREF(remaining_items);
}

void python_to_char_star_star(PyObject* obj, uint32_t* num_strings_p, char*** str_list_p)
{
	if (obj == Py_None)
	{
		clear_char_star_star(num_strings_p, str_list_p);
                return;
	}

	PyObject *list = PySequence_Fast(obj, "attribute is not a sequence");

	if (list == NULL)
	{
		print_python_error();
		clear_char_star_star(num_strings_p, str_list_p);
		return;
	}

	int python_count = PySequence_Fast_GET_SIZE(list);

	if (python_count == 0)
	{
		clear_char_star_star(num_strings_p, str_list_p);
		return;
	}

	if (python_count < (*num_strings_p))
	{
		// If some entries have been removed the we should clear the memory that we will not need
		for (int i = python_count; i < *num_strings_p; ++i)
		{
			xfree((*str_list_p)[i]);
		}
	}

	if (python_count != (*num_strings_p))
	{
		*num_strings_p = python_count;
		*str_list_p = xrealloc(*str_list_p, sizeof(char*) * (*num_strings_p));
	}

	for (int i = 0; i < python_count; ++i)
	{
		PyObject* obj = PySequence_Fast_GET_ITEM(list, i);
		PyObject* str = PyObject_Str(obj);
		char* s = PyUnicode_AsUTF8(str);

		(*str_list_p)[i] = xstrdup(s);

		Py_DECREF(str);
	}

	Py_DECREF(list);
	print_python_error(); // If there was one
}

#define retrieve_char_star(job_desc, dict, name) \
do { \
	PyObject* o = PyDict_GetItemString(dict, #name); \
	if (o != NULL) { \
		if (o == Py_None) { \
			job_desc->name = NULL; \
		} else { \
			char* s = PyUnicode_AsUTF8(o); \
			if (job_desc->name == NULL || strcmp(s, job_desc->name) != 0) { \
				xfree(job_desc->name); \
				job_desc->name = xstrdup(s); \
			} \
		} \
		PyDict_DelItemString(dict, #name); \
	} \
} while (0)
#define retrieve_char_star_star(job_desc, dict, name, count) \
do { \
	PyObject* o = PyDict_GetItemString(dict, #name); \
	if (o != NULL) { \
		python_to_char_star_star(o, &job_desc->count, &job_desc->name); \
		PyDict_DelItemString(dict, #name); \
	} \
} while (0)
#define retrieve_environment_dict(job_desc, dict, name, count) \
do { \
	PyObject* o = PyDict_GetItemString(dict, #name); \
	if (o != NULL) { \
		python_dict_to_environment(o, &job_desc->count, &job_desc->name); \
		PyDict_DelItemString(dict, #name); \
	} \
} while (0)
#define retrieve_int(job_desc, dict, name, noval) \
do { \
	PyObject* o = PyDict_GetItemString(dict, #name); \
	if (o != NULL) { \
		if (o == Py_None) { \
			job_desc->name = noval; \
		} else { \
			job_desc->name = PyLong_AsUnsignedLong(o); \
		} \
		PyDict_DelItemString(dict, #name); \
	} \
} while (0)
#define retrieve_uint8_t(job_desc, dict, name) retrieve_int(job_desc, dict, name, NO_VAL8)
#define retrieve_uint16_t(job_desc, dict, name) retrieve_int(job_desc, dict, name, NO_VAL16)
#define retrieve_uint32_t(job_desc, dict, name) retrieve_int(job_desc, dict, name, NO_VAL)
#define retrieve_uint64_t(job_desc, dict, name) retrieve_int(job_desc, dict, name, NO_VAL64)
#define retrieve_int_as_bool(job_desc, dict, name, noval) \
do { \
	PyObject* o = PyDict_GetItemString(dict, #name); \
	if (o != NULL) { \
		if (o == Py_None) { \
			job_desc->name = noval; \
		} else { \
			job_desc->name = PyLong_AsUnsignedLong(o); \
		} \
		PyDict_DelItemString(dict, #name); \
	} \
} while (0)
#define retrieve_uint8_t_as_bool(job_desc, dict, name) retrieve_int_as_bool(job_desc, dict, name, NO_VAL8)
#define retrieve_uint16_t_as_bool(job_desc, dict, name) retrieve_int_as_bool(job_desc, dict, name, NO_VAL16)
#define retrieve_time_t(job_desc, dict, name) \
do { \
	PyObject* o = PyDict_GetItemString(dict, #name); \
	if (o != NULL) { \
		job_desc->name = PyLong_AsUnsignedLong(o); \
		PyDict_DelItemString(dict, #name); \
	} \
} while (0)

void retrieve_job_desc_dict(struct job_descriptor *job_desc, PyObject* pJobDesc)
{
	retrieve_char_star(job_desc, pJobDesc, account);
	retrieve_char_star(job_desc, pJobDesc, acctg_freq);
	retrieve_char_star(job_desc, pJobDesc, admin_comment);
	retrieve_char_star(job_desc, pJobDesc, alloc_node);
	retrieve_uint16_t(job_desc, pJobDesc, alloc_resp_port);
	retrieve_uint32_t(job_desc, pJobDesc, alloc_sid);
	retrieve_char_star_star(job_desc, pJobDesc, argv, argc);
	retrieve_char_star(job_desc, pJobDesc, array_inx);
	//retrieve_char_star(job_desc, pJobDesc, array_bitmap); // void*
	retrieve_time_t(job_desc, pJobDesc, begin_time);
	retrieve_uint32_t(job_desc, pJobDesc, bitflags);
	retrieve_char_star(job_desc, pJobDesc, burst_buffer);
	retrieve_uint16_t(job_desc, pJobDesc, ckpt_interval);
	retrieve_char_star(job_desc, pJobDesc, ckpt_dir);
	retrieve_char_star(job_desc, pJobDesc, clusters);
	retrieve_char_star(job_desc, pJobDesc, cluster_features);
	retrieve_char_star(job_desc, pJobDesc, comment);
	retrieve_uint16_t_as_bool(job_desc, pJobDesc, contiguous);
	retrieve_uint16_t(job_desc, pJobDesc, core_spec);
	retrieve_char_star(job_desc, pJobDesc, cpu_bind);
	retrieve_uint16_t(job_desc, pJobDesc, cpu_bind_type);
	retrieve_uint32_t(job_desc, pJobDesc, cpu_freq_min);
	retrieve_uint32_t(job_desc, pJobDesc, cpu_freq_max);
	retrieve_uint32_t(job_desc, pJobDesc, cpu_freq_gov);
	retrieve_time_t(job_desc, pJobDesc, deadline);
	retrieve_uint32_t(job_desc, pJobDesc, delay_boot);
	retrieve_char_star(job_desc, pJobDesc, dependency);
	retrieve_time_t(job_desc, pJobDesc, end_time);
	retrieve_environment_dict(job_desc, pJobDesc, environment, env_size);
	retrieve_char_star(job_desc, pJobDesc, extra);
	retrieve_char_star(job_desc, pJobDesc, exc_nodes);
	retrieve_char_star(job_desc, pJobDesc, features);
	retrieve_uint64_t(job_desc, pJobDesc, fed_siblings_active);
	retrieve_uint64_t(job_desc, pJobDesc, fed_siblings_viable);
	retrieve_char_star(job_desc, pJobDesc, gres);
	retrieve_uint32_t(job_desc, pJobDesc, group_id);
	retrieve_uint16_t_as_bool(job_desc, pJobDesc, immediate);
	retrieve_uint32_t(job_desc, pJobDesc, job_id);
	retrieve_char_star(job_desc, pJobDesc, job_id_str);
	retrieve_uint16_t_as_bool(job_desc, pJobDesc, kill_on_node_fail);
	retrieve_char_star(job_desc, pJobDesc, licenses);
	retrieve_uint16_t(job_desc, pJobDesc, mail_type);
	retrieve_char_star(job_desc, pJobDesc, mail_user);
	retrieve_char_star(job_desc, pJobDesc, mcs_label);
	retrieve_char_star(job_desc, pJobDesc, mem_bind);
	retrieve_uint16_t(job_desc, pJobDesc, mem_bind_type);
	retrieve_char_star(job_desc, pJobDesc, name);
	retrieve_char_star(job_desc, pJobDesc, network);
	retrieve_uint32_t(job_desc, pJobDesc, nice);
	retrieve_uint32_t(job_desc, pJobDesc, num_tasks);
	retrieve_uint8_t(job_desc, pJobDesc, open_mode);
	retrieve_char_star(job_desc, pJobDesc, origin_cluster);
	retrieve_uint16_t(job_desc, pJobDesc, other_port);
	retrieve_uint8_t_as_bool(job_desc, pJobDesc, overcommit);
	retrieve_uint32_t(job_desc, pJobDesc, pack_job_offset);
	retrieve_char_star(job_desc, pJobDesc, partition);
	retrieve_uint16_t(job_desc, pJobDesc, plane_size);
	retrieve_uint8_t(job_desc, pJobDesc, power_flags);
	retrieve_uint32_t(job_desc, pJobDesc, priority);
	retrieve_uint32_t(job_desc, pJobDesc, profile);
	retrieve_char_star(job_desc, pJobDesc, qos);
	retrieve_uint16_t_as_bool(job_desc, pJobDesc, reboot);
	retrieve_char_star(job_desc, pJobDesc, resp_host);
	retrieve_uint16_t(job_desc, pJobDesc, restart_cnt);
	retrieve_char_star(job_desc, pJobDesc, req_nodes);
	retrieve_uint16_t_as_bool(job_desc, pJobDesc, requeue);
	retrieve_char_star(job_desc, pJobDesc, reservation);
	retrieve_char_star(job_desc, pJobDesc, script);
	retrieve_uint16_t(job_desc, pJobDesc, shared);
	retrieve_char_star_star(job_desc, pJobDesc, spank_job_env, spank_job_env_size);
	retrieve_uint32_t(job_desc, pJobDesc, task_dist);
	retrieve_uint32_t(job_desc, pJobDesc, time_limit);
	retrieve_uint32_t(job_desc, pJobDesc, time_min);
	retrieve_uint32_t(job_desc, pJobDesc, user_id);
	retrieve_uint16_t_as_bool(job_desc, pJobDesc, wait_all_nodes);
	retrieve_uint16_t(job_desc, pJobDesc, warn_flags);
	retrieve_uint16_t(job_desc, pJobDesc, warn_signal);
	retrieve_uint16_t(job_desc, pJobDesc, warn_time);
	retrieve_char_star(job_desc, pJobDesc, work_dir);
	retrieve_uint16_t(job_desc, pJobDesc, cpus_per_task);
	retrieve_uint32_t(job_desc, pJobDesc, min_cpus);
	retrieve_uint32_t(job_desc, pJobDesc, max_cpus);
	retrieve_uint32_t(job_desc, pJobDesc, min_nodes);
	retrieve_uint32_t(job_desc, pJobDesc, max_nodes);
	retrieve_uint16_t(job_desc, pJobDesc, boards_per_node);
	retrieve_uint16_t(job_desc, pJobDesc, sockets_per_board);
	retrieve_uint16_t(job_desc, pJobDesc, sockets_per_node);
	retrieve_uint16_t(job_desc, pJobDesc, cores_per_socket);
	retrieve_uint16_t(job_desc, pJobDesc, threads_per_core);
	retrieve_uint16_t(job_desc, pJobDesc, ntasks_per_node);
	retrieve_uint16_t(job_desc, pJobDesc, ntasks_per_socket);
	retrieve_uint16_t(job_desc, pJobDesc, ntasks_per_core);
	retrieve_uint16_t(job_desc, pJobDesc, ntasks_per_board);
	retrieve_uint16_t(job_desc, pJobDesc, pn_min_cpus);
	retrieve_uint64_t(job_desc, pJobDesc, pn_min_memory);
	retrieve_uint32_t(job_desc, pJobDesc, pn_min_tmp_disk);
	retrieve_uint32_t(job_desc, pJobDesc, req_switch);
	//select_jobinfo
	retrieve_char_star(job_desc, pJobDesc, std_err);
	retrieve_char_star(job_desc, pJobDesc, std_in);
	retrieve_char_star(job_desc, pJobDesc, std_out);
	//retrieve_uint64_t_star(job_desc, pJobDesc, tres_req_cnt);
	retrieve_uint32_t(job_desc, pJobDesc, wait4switch);
	retrieve_char_star(job_desc, pJobDesc, wckey);
	retrieve_uint16_t(job_desc, pJobDesc, x11);
	retrieve_char_star(job_desc, pJobDesc, x11_magic_cookie);
	retrieve_uint16_t(job_desc, pJobDesc, x11_target_port);
}

PyObject* load_script()
{
	char script_name[] = "job_submit";

	// Import the job_submit module
	PyObject *pName = PyUnicode_FromString(script_name);
	PyObject *pModuleInitial = PyImport_Import(pName);
	Py_DECREF(pName);

	if (pModuleInitial != NULL)
	{
		verbose("job_submit_python: Loaded \"%s\"", script_name);

		// Reload the module to ensure live updating the script works
		PyObject* pModule = PyImport_ReloadModule(pModuleInitial);
		Py_DECREF(pModuleInitial);

		return pModule;
	}

	error("job_submit_python: Failed to load \"%s\"", script_name);
	print_python_error();

	return pModuleInitial;
}

extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid, char **err_msg)
{
	PyObject* pModule = load_script();
	if (pModule != NULL)
	{
		PyObject* pFunc = PyObject_GetAttrString(pModule, "job_submit");
		if (pFunc && PyCallable_Check(pFunc))
		{
			PyObject* pJobDesc = create_job_desc_dict(job_desc);

			PyObject* pRc = PyObject_CallFunctionObjArgs(pFunc, pJobDesc, NULL);

			if (pRc != NULL)
			{
				// TODO check the return type is an int
				long rc = PyLong_AsLong(pRc);
				Py_DECREF(pRc);

				retrieve_job_desc_dict(job_desc, pJobDesc);
				Py_DECREF(pJobDesc);

				if (rc != SLURM_SUCCESS)
				{
					Py_DECREF(pFunc);
					Py_DECREF(pModule);
					return rc;
				}
			}
			else
			{
				Py_DECREF(pJobDesc);
				Py_DECREF(pFunc);
				Py_DECREF(pModule);

				error("job_submit_python: Call failed");
				print_python_error();

				return SLURM_ERROR;
			}
		}
		else
		{
			error("Cannot find function \"%s\"", "job_submit");
			print_python_error();

			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
			return SLURM_ERROR;
		}
		Py_XDECREF(pFunc);
		Py_DECREF(pModule);
	}
	else
	{
		return SLURM_ERROR;
	}
	
	return SLURM_SUCCESS;
}

extern int job_modify(struct job_descriptor *job_desc, struct job_record *job_ptr, uint32_t submit_uid)
{
	return SLURM_SUCCESS;
}

