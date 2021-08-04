/**
 * @file mraw.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for mraw.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_MRAW_H
#define PCHTML_MRAW_H


#include "config.h"

#include "private/mem.h"
#include "private/bst.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define pcutils_mraw_meta_size()                                                \
    (((sizeof(size_t) % PCHTML_MEM_ALIGN_STEP) != 0)                           \
    ? sizeof(size_t)                                                           \
        + (PCHTML_MEM_ALIGN_STEP - (sizeof(size_t) % PCHTML_MEM_ALIGN_STEP))   \
    : sizeof(size_t))


typedef struct {
    pcutils_mem_t *mem;
    pcutils_bst_t *cache;
}
pcutils_mraw_t;


pcutils_mraw_t *
pcutils_mraw_create(void) WTF_INTERNAL;

unsigned int
pcutils_mraw_init(pcutils_mraw_t *mraw, size_t chunk_size) WTF_INTERNAL;

void
pcutils_mraw_clean(pcutils_mraw_t *mraw) WTF_INTERNAL;

pcutils_mraw_t *
pcutils_mraw_destroy(pcutils_mraw_t *mraw, bool destroy_self) WTF_INTERNAL;


void *
pcutils_mraw_alloc(pcutils_mraw_t *mraw, size_t size) WTF_INTERNAL;

void *
pcutils_mraw_calloc(pcutils_mraw_t *mraw, size_t size) WTF_INTERNAL;

void *
pcutils_mraw_realloc(pcutils_mraw_t *mraw, void *data, 
                size_t new_size) WTF_INTERNAL;

void *
pcutils_mraw_free(pcutils_mraw_t *mraw, void *data) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline size_t
pcutils_mraw_data_size(void *data)
{
    return *((size_t *) (((uint8_t *) data) - pcutils_mraw_meta_size()));
}

static inline void
pcutils_mraw_data_size_set(void *data, size_t size)
{
    data = (((uint8_t *) data) - pcutils_mraw_meta_size());
    memcpy(data, &size, sizeof(size_t));
}

static inline void *
pcutils_mraw_dup(pcutils_mraw_t *mraw, const void *src, size_t size)
{
    void *data = pcutils_mraw_alloc(mraw, size);

    if (data != NULL) {
        memcpy(data, src, size);
    }

    return data;
}

#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_MRAW_H */
