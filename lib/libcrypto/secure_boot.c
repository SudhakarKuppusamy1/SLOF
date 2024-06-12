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

#include <libcrypto.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include "../../slof/paflof.h"

int is_secureboot() {
        /* only verify if in secure-boot mode.*/
        forth_eval("s\" /\" find-device s\" ibm,secure-boot\" get-node get-property");
        if (forth_pop() == -1)
		return 0;
	forth_pop();
	if (*(int32_t *)forth_pop() < 2)
		return 0;
	return 1;
}


