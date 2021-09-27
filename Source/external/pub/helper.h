/*
 * @file helper.h
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The header file of tools used by all files in this directory.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _DVOJBS_TOOLS_H_
#define _DVOJBS_TOOLS_H_

#include "config.h"
//#include "private/debug.h"
#include "purc-variant.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#define STREAM_SIZE 1024

typedef purc_variant_t (*pcdvobjs_create) (void);

// as FILE, FS, MATH
struct pcdvojbs_dvobjs_object {
    const char *name;
    const char *description;
    pcdvobjs_create create_func;
};

// dynamic variant in dynamic object
struct pcdvojbs_dvobjs {
    const char * name;
    purc_dvariant_method getter;
    purc_dvariant_method setter;
};


struct pcdvobjs_math_value {
    double d;
    long double ld;
};

struct pcdvobjs_math_param {
    double d;
    long double ld;
    purc_variant_t v;
    int is_long_double;

    purc_variant_t variables;
};

int pcdvobjs_math_param_set_var(struct pcdvobjs_math_param *param,
        const char *var, struct pcdvobjs_math_value *val);

int pcdvobjs_math_param_get_var(struct pcdvobjs_math_param *param,
        const char *var, struct pcdvobjs_math_value *val);

struct pcdvobjs_logical_param {
    int result;
    purc_variant_t v;

    purc_variant_t variables;
};

bool wildcard_cmp (const char *str1, const char *pattern);

const char * pcdvobjs_remove_space (char * buffer);

const char* pcdvobjs_get_next_option (const char* data, const char* delims,
                                            size_t* length);
const char* pcdvobjs_get_prev_option (const char* data, size_t str_len,
                            const char* delims, size_t* length);
const char* pcdvobjs_file_get_next_option (const char* data, const char* delims,
                                            size_t* length);
const char* pcdvobjs_file_get_prev_option (const char* data, size_t str_len,
                            const char* delims, size_t* length);

purc_variant_t pcdvobjs_make_dvobjs (const struct pcdvojbs_dvobjs *method,
                                    size_t size);

extern int
math_parse(const char *input, struct pcdvobjs_math_param *param);

extern int
logical_parse(const char *input, struct pcdvobjs_logical_param *param);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // _DVOJBS_TOOLS_H_