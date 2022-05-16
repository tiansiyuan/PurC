/*
 * @file instance.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/05
 * @brief The structures for PurC instance.
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

#ifndef PURC_PRIVATE_INSTANCE_H
#define PURC_PRIVATE_INSTANCE_H

#include "purc.h"

#include "config.h"

#include "private/variant.h"
#include "private/map.h"
#include "private/executor.h"
#include "private/interpreter.h"

#include <stdio.h>

struct hvml_app;
typedef struct hvml_app hvml_app;
typedef struct hvml_app *hvml_app_t;

hvml_app_t hvml_app_get(void);

struct pcinst;
typedef struct pcinst pcinst;
typedef struct pcinst *pcinst_t;

typedef int (*module_init_once_f)(void);
typedef int (*module_init_instance_f)(struct pcinst* inst);

struct pcmodule {
    // PURC_HAVE_XXXX if !always
    unsigned int            id;
    unsigned int            module_inited;

    module_init_once_f      init_once;
    module_init_instance_f  init_instance;
};

struct pcinst {
    int                     errcode;
    purc_variant_t          err_exinfo;
    purc_atom_t             error_except;

    unsigned int            modules;

    char                   *app_name;
    char                   *runner_name;
    purc_atom_t             endpoint_atom;

#define LOG_FILE_SYSLOG     ((FILE *)-1)
    /* the FILE object for logging (-1: use syslog; NULL: disabled) */
    FILE                   *fp_log;

    /* data bounden to the current session, e.g, the statbuf of the random
       number generator */
    pcutils_map            *local_data_map;

    struct pcvariant_heap  *variant_heap;
    struct pcvariant_heap  *org_vrt_heap;

    struct pcvarmgr        *variables;

    struct pcrdr_conn      *conn_to_rdr;
    struct renderer_capabilities *rdr_caps;

    struct pcexecutor_heap *executor_heap;
    struct pcintr_heap     *intr_heap;

    bool initialized_main_runloop;

    /* FIXME: enable the fields ONLY when NDEBUG is undefined */
    struct pcdebug_backtrace  *bt;
};

/* gets the current instance */
struct pcinst* pcinst_current(void) WTF_INTERNAL;
pcvarmgr_t pcinst_get_variables(void) WTF_INTERNAL;

struct pcrdr_msg *pcinst_get_message(void) WTF_INTERNAL;
void pcinst_put_message(struct pcrdr_msg *msg) WTF_INTERNAL;

void pcinst_clear_error(struct pcinst *inst) WTF_INTERNAL;

#endif /* not defined PURC_PRIVATE_INSTANCE_H */

