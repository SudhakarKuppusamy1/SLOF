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

#include <stddef.h>

enum _sbat_errors {
        SBAT_VALID = 0, /* the binary data is OK given the variable data */
        SBAT_FAILURE   /* a generation in the binary was less than the minimum
                          from the variable */
};

int is_secureboot(void);
int verify_sbat(void *blob, size_t len);
