/*
 * @file string.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of string dynamic variant object.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

#include "purc-variant.h"
#include "tools.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <locale.h>
#include <time.h>
#include <math.h>


static const char* get_next_segment (const char* data, const char* delim, 
                                                            size_t* length)
{
    const char* head = NULL;
    char* temp = NULL;

    *length = 0;

    if ((*data == 0x00) || (*delim == 0x00))
        return NULL;

    head = data;

    temp = strstr (head, delim);

    if (temp) {
        *length =  temp - head;
    }
    else {
        *length = strlen (head);
    }

    return head;
}

static purc_variant_t
string_contains (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    const char* source = purc_variant_get_string_const (argv[0]);
    const char* sub = purc_variant_get_string_const (argv[1]);

    if (strstr (source, sub))
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}


static purc_variant_t
string_ends_with (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    const char* source = purc_variant_get_string_const (argv[0]);
    size_t len_source = purc_variant_string_length (argv[0]) - 1;
    const char* sub = purc_variant_get_string_const (argv[1]);
    size_t len_sub = purc_variant_string_length (argv[1]) -1;

    size_t i = 0;
    bool find = true;

    if ((len_source == 0) || (len_sub == 0) || (len_source < len_sub)) {
        find = false;
    }
    else {
        for (i = 0; i < len_sub; i++)
        {
            if (*(source + len_source - len_sub + i) != *(sub + i)) {
                find = false;
                break;
            }
        }
    }

    if (find)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}


static purc_variant_t
string_explode (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    const char* source = purc_variant_get_string_const (argv[0]);
    const char* delim = purc_variant_get_string_const (argv[1]);
    size_t len_delim = purc_variant_string_length (argv[1]) - 1;
    size_t length = 0;
    const char* head = get_next_segment (source, delim, &length);

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);

    while (head) {
        val = purc_variant_make_string (head, true);
        purc_variant_array_append (ret_var, val);
        purc_variant_unref (val);
    
        if (*(head + length) != 0x00)
            head = get_next_segment (head + length + len_delim, 
                                                    delim, &length);
        else
            break;
    }

    return ret_var;
}


static purc_variant_t
string_shuffle (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    size_t size = purc_variant_string_length (argv[0]);
    if (size < 2) {      // it is an empty string
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    char * src = malloc (size);
    strcpy (src, purc_variant_get_string_const (argv[0]));
    *(src + size - 1) = 0x00;

    srand(time(NULL));

    size_t i = 0;
    int random = 0;
    for(i =  0; i < size - 2; i++)
    {
        random = i + (rand () % (size - 1 - i));
        int tmp = *(src + random);
        *(src + random) = *(src + i);
        *(src + i) = tmp;
    }

    ret_var = purc_variant_make_string (src, false);
    
    free(src);

    return ret_var;
}


static purc_variant_t
string_replace (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 2)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[2] != NULL) && (!purc_variant_is_string (argv[2]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    const char* source = purc_variant_get_string_const (argv[0]);
    const char* delim = purc_variant_get_string_const (argv[1]);
    const char* replace = purc_variant_get_string_const (argv[2]);
    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, 1024);

    size_t len_delim = purc_variant_string_length (argv[1]) - 1;
    size_t length = 0;
    const char* head = get_next_segment (source, delim, &length);

    while (head) {
        purc_rwstream_write (rwstream, head, length);

        if (*(head + length) != 0x00) {
            purc_rwstream_write (rwstream, replace, len_delim);
            head = get_next_segment (head + length + len_delim, 
                                                    delim, &length);
        }
        else
            break;
    }

    size_t rw_size = 0;
    const char * rw_string = purc_rwstream_get_mem_buffer (rwstream, &rw_size);

    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        ret_var = purc_variant_make_string (rw_string, false); 
        if(ret_var == PURC_VARIANT_INVALID)
            ret_var = PURC_VARIANT_INVALID;
    }

    purc_rwstream_destroy (rwstream);

    return ret_var;
}

static purc_variant_t
string_format_c (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    const char *format = NULL;
    purc_variant_t ret_var = NULL;
    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, 1024);

    if ((argv == NULL) || (nr_args == 0))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;
    else
        format = purc_variant_get_string_const (argv[0]);

    char buffer[16];
    int start = 0;
    int i = 0;
    int j = 1;
    int64_t i64 = 0;
    uint64_t u64 = 0;
    double number = 0;
    const char *temp_str = NULL;

    while (*(format + i) != 0x00) {
        if (*(format + i) == '%') {
            switch (*(format + i + 1)) {
                case 0x00:
                    break;
                case '%':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    purc_rwstream_write (rwstream, "%", 1);
                    i++;
                    start = i + 1;
                    break;
                case 'd':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == NULL) {
                        purc_rwstream_destroy (rwstream); 
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_longint (argv[j], &i64, false);
                    sprintf (buffer, "%ld", i64);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 'o':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == NULL) {
                        purc_rwstream_destroy (rwstream); 
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_ulongint (argv[j], &u64, false);
                    sprintf (buffer, "%lo", u64);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 'u':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == NULL) {
                        purc_rwstream_destroy (rwstream); 
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_ulongint (argv[j], &u64, false);
                    sprintf (buffer, "%lu", u64);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 'x':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == NULL) {
                        purc_rwstream_destroy (rwstream); 
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_ulongint (argv[j], &u64, false);
                    sprintf (buffer, "%lx", u64);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 'f':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == NULL) {
                        purc_rwstream_destroy (rwstream); 
                        return PURC_VARIANT_INVALID;
                    }
                    purc_variant_cast_to_number (argv[j], &number, false);
                    sprintf (buffer, "%lf", number);
                    purc_rwstream_write (rwstream, buffer, strlen (buffer));
                    i++;
                    j++;
                    start = i + 1;
                    break;
                case 's':
                    purc_rwstream_write (rwstream, format + start, i - start);
                    if (argv[j] == NULL) {
                        purc_rwstream_destroy (rwstream); 
                        return PURC_VARIANT_INVALID;
                    }
                    if (!purc_variant_is_string (argv[0])) {
                        purc_rwstream_destroy (rwstream); 
                        return PURC_VARIANT_INVALID;
                    }
                    temp_str = purc_variant_get_string_const (argv[i]);
                    purc_rwstream_write (rwstream, temp_str, strlen (temp_str));
                    i++;
                    j++;
                    start = i + 1;
                    break;
            }
        }
        i++;
    }

    if (i != start)
        purc_rwstream_write (rwstream, format + start, strlen (format + start));

    size_t rw_size = 0;
    size_t content_size = 0;
    char * rw_string = purc_rwstream_get_mem_buffer_ex (rwstream, 
            &content_size, &rw_size, true);

    if ((rw_size == 0) || (rw_string == NULL))
        ret_var = PURC_VARIANT_INVALID;
    else {
        ret_var = purc_variant_make_string_reuse_buff (rw_string, 
                rw_size, false);
        if(ret_var == PURC_VARIANT_INVALID) {
            pcinst_set_error (PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

    purc_rwstream_destroy (rwstream);

    return ret_var;
}

static purc_variant_t
string_format_p (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    const char *format = NULL;
    purc_variant_t ret_var = NULL;
    purc_variant_t tmp_var = NULL;
    purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, 1024);
    int type = 0;
    char buffer[32];
    int index = 0;
    char *buf = NULL;
    size_t sz_stream = 0;
    purc_rwstream_t serialize = NULL;     // used for serialize

    if ((argv == NULL) || (nr_args == 0))
        return PURC_VARIANT_INVALID;

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0])))
        return PURC_VARIANT_INVALID;
    else
        format = purc_variant_get_string_const (argv[0]);

    if ((argv[1] != NULL) && (!purc_variant_is_array (argv[1])))
        type = 0;
    else if ((argv[1] != NULL) && (!purc_variant_is_object (argv[1])))
        type = 1;
    else
        return PURC_VARIANT_INVALID;
        
    if (type == 0) {
        const char *start = NULL;
        const char *end = NULL;
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (format, "{", &length);
        while (head) {
            purc_rwstream_write (rwstream, head, length);

            start = head + length + 2;
            head = pcdvobjs_get_next_option (head + length + 1, "}", &length);
            end = head + length;
            strncpy(buffer, start, end - start);
            *(buffer + (end - start)) = 0x00;
            pcdvobjs_remove_space (buffer);
            index = atoi (buffer);

            tmp_var = purc_variant_array_get (argv[1], index);
            if (tmp_var == NULL) {
                purc_rwstream_destroy (rwstream); 
                return PURC_VARIANT_INVALID;
            }

            serialize = purc_rwstream_new_buffer (32, 1024);
            purc_variant_serialize (tmp_var, serialize, 3, 0, &sz_stream);
            buf = purc_rwstream_get_mem_buffer (serialize, &sz_stream);
            purc_rwstream_write (rwstream, buf, sz_stream);
            purc_rwstream_destroy (serialize);

            head = pcdvobjs_get_next_option (head + length + 1, "{", &length);
        }

        if (end != NULL)
            purc_rwstream_write (rwstream, end, strlen (end));
    }
    else {
        const char *start = NULL;
        const char *end = NULL;
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (format, "{", &length);
        while (head) {
            purc_rwstream_write (rwstream, head, length);

            start = head + length + 2;
            head = pcdvobjs_get_next_option (head + length + 1, "}", &length);
            end = head + length;
            strncpy(buffer, start, end - start);
            *(buffer + (end - start)) = 0x00;
            pcdvobjs_remove_space (buffer);

            tmp_var = purc_variant_object_get_by_ckey (argv[1], buffer);
            if (tmp_var == NULL) {
                purc_rwstream_destroy (rwstream); 
                return PURC_VARIANT_INVALID;
            }

            serialize = purc_rwstream_new_buffer (32, 1024);
            purc_variant_serialize (tmp_var, serialize, 3, 0, &sz_stream);
            buf = purc_rwstream_get_mem_buffer (serialize, &sz_stream);
            purc_rwstream_write (rwstream, buf, sz_stream);
            purc_rwstream_destroy (serialize);

            head = pcdvobjs_get_next_option (head + length + 1, "{", &length);
        }

        if (end != NULL)
            purc_rwstream_write (rwstream, end, strlen (end));
    }

    return ret_var;
}

// only for test now.
purc_variant_t pcdvojbs_get_string (void)
{
    purc_variant_t v1 = NULL;
    purc_variant_t v2 = NULL;
    purc_variant_t v3 = NULL;
    purc_variant_t v4 = NULL;
    purc_variant_t v5 = NULL;
    purc_variant_t v6 = NULL;
    purc_variant_t v7 = NULL;

    v1 = purc_variant_make_dynamic (string_contains, NULL);
    v2 = purc_variant_make_dynamic (string_ends_with, NULL);
    v3 = purc_variant_make_dynamic (string_explode, NULL);
    v4 = purc_variant_make_dynamic (string_shuffle, NULL);
    v5 = purc_variant_make_dynamic (string_replace, NULL);
    v6 = purc_variant_make_dynamic (string_format_c, NULL);
    v7 = purc_variant_make_dynamic (string_format_p, NULL);

    purc_variant_t string = purc_variant_make_object_by_static_ckey (7,
                                "conatins",     v1,
                                "ends_with",    v2,
                                "explode",      v3,
                                "shuffle",      v4,
                                "replace",      v5,
                                "format_c",     v6,
                                "format_p",     v7);

    purc_variant_unref (v1);
    purc_variant_unref (v2);
    purc_variant_unref (v3);
    purc_variant_unref (v4);
    purc_variant_unref (v5);
    purc_variant_unref (v6);
    purc_variant_unref (v7);
    return string;
}
