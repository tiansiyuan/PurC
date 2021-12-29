/*
 * @file html.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of public part for html parser.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/atom-buckets.h"
#include "private/html.h"

static struct const_str_atom html_atoms [] = {
    { "append", 0 },
    { "prepend", 0 },
    { "insertBefore", 0 },
    { "insertAfter", 0 },
};

void pchtml_init_once(void)
{
    // initialize others

    for (size_t i = 0; i < sizeof(html_atoms)/sizeof(html_atoms[0]); i++) {
        html_atoms[i].atom = purc_atom_from_static_string_ex (
                ATOM_BUCKET_HTML, html_atoms[i].str);
    }
}

void pchtml_init_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);

    // initialize others
}

void pchtml_cleanup_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);
}

purc_atom_t get_html_cmd_atom (size_t id)
{
    purc_atom_t atom = 0;
    if (id < sizeof(html_atoms)/sizeof(html_atoms[0]))
        atom = html_atoms[id].atom;

    return atom;
}
