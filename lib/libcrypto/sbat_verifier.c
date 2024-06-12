/******************************************************************************
 * Copyright (c) 2004, 2011, 2024 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include <sbat_var.h>
#include <libcrypto.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

struct sbat_elfnote_entry {
	const char *component_name;
	const char *component_generation;
	const char *vendor_name;
	const char *vendor_package_name;
	const char *vendor_version;
	const char *vendor_url;
};

struct sbat_var_entry {
	const char *component_name;
	const char *component_generation;
};

struct sbat_var_entry **sbat_varentry = NULL;
int sbat_var_count;

struct sbat_elfnote_entry **sbat_elfentry = NULL;
int sbat_elf_count;

unsigned char *sbat_data = NULL;
unsigned long sbat_data_len = 0;

static int add_sbat_var (struct sbat_var_entry **sbat, char *data, int id)
{

	if (sbat[id]->component_name == NULL) {
		sbat[id]->component_name = (char *)malloc (strlen(data)+1);
		if (sbat[id]->component_name)
			memcpy((char *)sbat[id]->component_name, data, strlen(data));
		else
			return -1;
	} else if (sbat[id]->component_generation == NULL) {
		sbat[id]->component_generation = (char *)malloc (strlen(data)+1);
		if (sbat[id]->component_generation)
			memcpy((char *)sbat[id]->component_generation, data, strlen(data));
		else
			return -1;
	}

	return 0;
}

static int add_sbat_elf (struct sbat_elfnote_entry **sbat, char *data, int id)
{
	if (sbat[id]->component_name == NULL) {
		sbat[id]->component_name = (char *)malloc (strlen(data)+1);
		if (sbat[id]->component_name)
			memcpy((char *)sbat[id]->component_name, data, strlen(data));
		else
			return -1;
	} else if (sbat[id]->component_generation == NULL) {
		sbat[id]->component_generation = (char *)malloc (strlen(data)+1);
		if (sbat[id]->component_generation)
			memcpy((char *)sbat[id]->component_generation, data, strlen(data));
		else
			return -1;
	} else if (sbat[id]->vendor_name == NULL) {
		sbat[id]->vendor_name = (char *)malloc (strlen(data)+1);
		if (sbat[id]->vendor_name)
			memcpy((char *)sbat[id]->vendor_name, data, strlen(data));
		else
			return -1;
	} else if (sbat[id]->vendor_package_name == NULL) {
		sbat[id]->vendor_package_name = (char *)malloc (strlen(data)+1);
		if (sbat[id]->vendor_package_name)
			memcpy((char *)sbat[id]->vendor_package_name, data, strlen(data));
		else
			return -1;
	} else if (sbat[id]->vendor_version == NULL) {
		sbat[id]->vendor_version = (char *)malloc (strlen(data)+1);
		if (sbat[id]->vendor_version)
			memcpy((char *)sbat[id]->vendor_version, data, strlen(data));
		else
			return -1;
	} else if (sbat[id]->vendor_url == NULL) {
		sbat[id]->vendor_url = (char *)malloc (strlen(data)+1);
		if (sbat[id]->vendor_url)
			memcpy((char *)sbat[id]->vendor_url, data, strlen(data));
		else
			return -1;
	}

	return 0;
}

static void free_entry (void **entry, int count)
{
	int i = 0;

	for (i = 0; i < count; i++)
		free (entry[i]);
	free (entry);
}

static int parse_sbat_entry (const char *str, const int flag)
{
	int ret = 0;
	char *line;
	char *token;
	char buf[256];

	for (line = strtok ((char *)str, "\n"); line != NULL;line = strtok (line + strlen (line) + 1, "\n")) {
		memcpy (buf, line, sizeof (buf));
		if (flag) {
			if (sbat_varentry)
				sbat_varentry = realloc(sbat_varentry, (sbat_var_count + 1) * sizeof(void *));
			else
				sbat_varentry = malloc(sizeof(void *));

			if (sbat_varentry == NULL) {
				free_entry ((void **)sbat_varentry, sbat_var_count);
				printf ("Out of Memory\n");
				return -1;
			}
			sbat_varentry[sbat_var_count] = (struct sbat_var_entry *)malloc(sizeof(struct sbat_var_entry)+1);
			if (sbat_varentry[sbat_var_count] == NULL) {
				free_entry ((void **)sbat_varentry, sbat_var_count);
				printf ("Out of Memory\n");
				return -1;
			}
			memset (sbat_varentry[sbat_var_count], 0x00, sizeof(struct sbat_var_entry) + 1);
		} else {
			if (sbat_elfentry)
				sbat_elfentry = realloc(sbat_elfentry, (sbat_elf_count + 1) * sizeof(void *));
			else
				sbat_elfentry = malloc(sizeof(void *));

			if (sbat_elfentry == NULL) {
				free_entry ((void **)sbat_elfentry, sbat_elf_count);
				printf ("Out of Memory\n");
				return -1;
			}
			sbat_elfentry[sbat_elf_count] = (struct sbat_elfnote_entry *)malloc(sizeof(struct sbat_elfnote_entry) + 1);
			if (sbat_elfentry[sbat_elf_count] == NULL) {
				free_entry ((void **)sbat_elfentry, sbat_elf_count);
				printf ("Out of Memory\n");
				return -1;
			}
			memset (sbat_elfentry[sbat_elf_count], 0x00, sizeof(struct sbat_elfnote_entry) + 1);
		}

		for (token = strtok (buf, ","); token != NULL;token = strtok (token + strlen (token) + 1, ",")) {
			if (flag) {
				ret = add_sbat_var (sbat_varentry, token, sbat_var_count);
				if (ret != 0) {
					printf ("Out of Memory\n");
					return -1;
				}
			} else {
				ret = add_sbat_elf (sbat_elfentry, token, sbat_elf_count);
				if (ret != 0) {
					printf ("Out of Memory\n");
					return -1;
				}
			}
		}
		if (flag)
			sbat_var_count++;
		else
			sbat_elf_count++;
	}

	return 0;
}

int verify_sbat(void *blob, size_t len)
{
	int rc = 0, i = 0, j = 0;
	int found = 0;

	rc = parse_sbat_entry ((char *)sbat_var_csv, 1);
	if (rc != 0)
		return SBAT_FAILURE;
	rc = parse_sbat_entry (blob, 0);
	if (rc != 0)
		return SBAT_FAILURE;

	for (i=0;i<sbat_elf_count; i++) {
		for (j = 0;j<sbat_var_count; j++) {
			found = 0;
			if (strcmp (sbat_varentry[j]->component_name,  sbat_elfentry[i]->component_name) == 0) {
				int sbat_gen = atoi(sbat_elfentry[i]->component_generation);
				int sbat_var_gen = atoi(sbat_varentry[j]->component_generation);
				found = 1;
				if (sbat_gen < sbat_var_gen) {
					printf("SBAT: component generation (metadata(%d) < var data(%d)) not supported, aborting\n",
						sbat_gen, sbat_var_gen);
					return SBAT_FAILURE;
				}
			}

			if (found)
				break;
		}
	}

	free_entry ((void **)sbat_varentry, sbat_var_count);
	free_entry ((void **)sbat_elfentry, sbat_elf_count);

	if (!found) {
		printf("SBAT: component generation not found, aborting\n");
		return SBAT_FAILURE;
	}

	return SBAT_VALID;
}
