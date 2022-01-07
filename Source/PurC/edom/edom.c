/*
 * @file edom.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of public part for edom.
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
#include "private/edom.h"
#include "private/stringbuilder.h"

void pcedom_init_once(void)
{
    // initialize others
}

/* VW NOTE: eDOM module should work without instance
void pcedom_init_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);

    // initialize others
}

void pcedom_cleanup_instance(struct pcinst* inst)
{
    UNUSED_PARAM(inst);
}
*/


// ============================= for element-variant ==========================
// .attr(<string: attributeName>)
int
pcedom_element_attr(pcedom_element_t *elem, const char *attr_name,
        const unsigned char **val, size_t *len)
{
    PC_ASSERT(elem && attr_name && val && len);
    *val = pcedom_element_get_attribute(elem,
            (const unsigned char*)attr_name, strlen(attr_name),
            len);
    return 0;
}

// TODO:
// .prop(<string: propertyName>)
// int
// pcedom_element_prop(pcedom_element_t *elem, const char *attr_name,
//         struct pcedom_attr **attr);

static const char*
find_char(const char *start, size_t len, const char c)
{
    const char *p = start;
    for (size_t i=0; i<len; ++i, ++p) {
        if (*p == c)
            return p;
    }
    return NULL;
}

// .style(<string: styleName>)
int
pcedom_element_style(pcedom_element_t *elem, const char *style_name,
        const unsigned char **style, size_t *len)
{
    PC_ASSERT(elem && style_name && style && len);
    *style = NULL;
    *len = 0;

    const unsigned char *s;
    size_t n;
    s = pcedom_element_get_attribute(elem,
                (const unsigned char*)"style", 5,
                &n);
    if (!s)
        return 0;

    const char *p = (const char*)s;
    const char *end = p + n;
    const size_t name_len = strlen(style_name);

    while (p < end && *p) {
        const char *colon = find_char(p, end-p, ':');
        if (!colon) {
            if (strncmp(p, style_name, name_len) == 0) {
                PC_ASSERT(0); // Not implemented yet
                *style = (const unsigned char*)"";
                *len = 0;
            }
            break;
        }

        if (strncmp(p, style_name, name_len) == 0) {
            *style = (const unsigned char*)colon + 1;
            const char *semi = find_char(colon+1, end-colon-1, ';');
            if (semi) {
                *len = semi - colon - 1;
            }
            else {
                *len = end - colon -1;
            }
            break;
        }

        const char *semi = find_char(colon+1, end-colon-1, ';');
        if (!semi)
            break;

        p = semi + 1;
    }

    return 0;
}

// .content()
int
pcedom_element_content(pcedom_element_t *elem,
        const unsigned char **content, size_t *len)
{
    PC_ASSERT(elem && content && len);
    *content = NULL;
    *len = 0;

    PC_ASSERT(0); // Not implemented yet
    return -1;
}

// .textContent()
int
pcedom_element_text_content(pcedom_element_t *elem,
        char **text, size_t *len)
{
    PC_ASSERT(elem && text && len);
    *text = NULL;
    *len = 0;

    struct pcutils_string str;
    pcutils_string_init(&str, 128);

    int r = 0;
    pcedom_node_t *node = elem->node.first_child;
    for (; node; node = node->next) {
        if (node->type!=PCEDOM_NODE_TYPE_TEXT)
            continue;

        const unsigned char *s;
        size_t n;
        s = pcedom_node_text_content(node, &n);

        r = pcutils_string_append(&str, "%.*s", (int)n, (const char*)s);
        if (r)
            break;
    }

    if (r == 0) {
        if (str.buf == str.abuf) {
            *text = strndup(str.buf, str.end - str.buf);
            if (!*text) {
                r = -1;
            }
            *len = str.end - str.buf;
        }
        else {
            *text = str.abuf;
            *len  = str.end - str.abuf;
            str.abuf = NULL;
        }
    }

    pcutils_string_reset(&str);

    return r ? -1 : 0;
}

// TODO:
// .jsonContent()
// FIXME: json in serialized form?
// static int
// pcedom_element_json_content(pcedom_element_t *elem, char **json);

// .val()
// TODO: what type for val?
// static int
// pcedom_element_val(pcedom_element_t *elem, what_type **val);

// .hasClass(<string: className>)
int
pcedom_element_has_class(pcedom_element_t *elem, const char *class_name,
        bool *has)
{
    PC_ASSERT(elem && class_name && has);
    *has = false;

    const unsigned char *s;
    size_t len;
    s = pcedom_element_get_attribute(elem,
            (const unsigned char*)class_name, strlen(class_name),
            &len);

    if (s &&
        strncmp((const char*)s, class_name, len) == 0 &&
        class_name[len] == '\0')
    {
        *has = true;
    }
    return 0;
}




// .attr(! <string: attributeName>, <string: value>)
int
pcedom_element_set_attr(pcedom_element_t *elem, const char *attr_name,
        const char *attr_val)
{
    PC_ASSERT(elem && attr_name && attr_val);
    pcedom_attr_t *attr;
    attr = pcedom_element_set_attribute(elem,
                (const unsigned char*)attr_name, strlen(attr_name),
                (const unsigned char*)attr_val, strlen(attr_val));

    return attr ? 0 : -1;
}

// TODO:
// .prop(! <string: propertyName>, <any: value>)
// static int
// pcedom_element_set_prop(pcedom_element_t *elem, ...);

struct style_token_arg {
    const char              *style_name;
    size_t                   style_name_len;
    const char              *style;

    struct pcutils_string    string;
};

static int
set_style_token_found(const char *token, const char *end, void *ud)
{
    struct style_token_arg *arg = (struct style_token_arg*)ud;
    const char *p = token;
    for (; p<end; ++p) {
        if (*p == ':') {
            break;
        }
    }
    if ((size_t)(p-token) == arg->style_name_len &&
        strncmp(token, arg->style_name, p-token) == 0)
    {
        // bypass
        return 0;
    }

    int r;
    r = pcutils_string_append(&arg->string,
            "%.*s", (int)(end-token), token);
    return r ? -1 : 0;
}

// .style(! <string: styleName>, <string: value>)
int
pcedom_element_set_style(pcedom_element_t *elem, const char *style_name,
        const char *style)
{
    PC_ASSERT(elem && style_name && style);

    int r;
    const unsigned char *s;
    size_t len;
    r = pcedom_element_attr(elem, "style", &s, &len);
    if (r)
        return -1;

    struct style_token_arg arg;
    arg.style_name     = style_name;
    arg.style_name_len = strlen(style_name);
    arg.style          = style;
    pcutils_string_init(&arg.string, 128);

    r = pcutils_token_by_delim((const char*)s, (const char*)s + len, ';',
            &arg, set_style_token_found);

    if (r == 0) {
        pcedom_attr_t *attr;
        attr = pcedom_element_set_attribute(elem,
                (const unsigned char*)"style", 5,
                (const unsigned char*)arg.string.abuf,
                arg.string.end - arg.string.abuf);
        if (attr == 0)
            r = -1;
    }

    pcutils_string_reset(&arg.string);

    return r ? -1 : 0;
}

// FIXME: de-serialize and then replace?
// .content(! <string: content>)
int
pcedom_element_set_content(pcedom_element_t *elem, const char *content)
{
    PC_ASSERT(elem && content);
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

// .textContent(! <string: content>)
int
pcedom_element_set_text_content(pcedom_element_t *elem,
        const char *text)
{
    PC_ASSERT(elem && text);

    pcedom_node_t *node = elem->node.first_child;
    pcedom_node_t *next;
    for (; node; node = next) {
        next = node ? node->next : NULL;
        if (node->type!=PCEDOM_NODE_TYPE_TEXT)
            continue;

        // FIXME: remove or destroy dilemma
        //        leakage or access-violation-problem
        pcedom_node_destroy_deep(node);
    }

    PC_ASSERT(0); // Not implemented yet
    return -1;
}

// FIXME: json in serialized form?
// .jsonContent(! <string: content>)
int
pcedom_element_set_json_content(pcedom_element_t *elem, const char *json)
{
    PC_ASSERT(elem && json);
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

// .val(! <newValue>)
// TODO: what type for val?
// static int
// pcedom_element_set_val(pcedom_element_t *elem, const what_type *val);

// .addClass(! <string: className>)
int
pcedom_element_add_class(pcedom_element_t *elem, const char *class_name)
{
    PC_ASSERT(elem && class_name);
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

// .removeAttr(! <string: attributeName>)
int
pcedom_element_remove_attr(pcedom_element_t *elem, const char *attr_name)
{
    PC_ASSERT(elem && attr_name);
    unsigned int ui;
    ui = pcedom_element_remove_attribute(elem,
            (const unsigned char*)attr_name, strlen(attr_name));
    return ui ? -1 : 0;
}

// .removeClass(! <string: className>)
int
pcedom_element_remove_class_by_name(pcedom_element_t *elem,
        const char *class_name)
{
    PC_ASSERT(elem && class_name);
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

// ============================= for collection-variant =======================
// .count()
int
pcedom_collection_count(pcedom_collection_t *col, size_t *count)
{
    PC_ASSERT(col && count);
    *count = pcedom_collection_length(col);
    return 0;
}

// .at(<real: index>)
int
pcedom_collection_at(pcedom_collection_t *col, size_t idx,
        struct pcedom_element **element)
{
    PC_ASSERT(col && element);

    *element = pcedom_collection_element(col, idx);
    return 0;
}

