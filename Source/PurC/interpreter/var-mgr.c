/*
 * @file var-mgr.c
 * @author XueShuming
 * @date 2021/12/06
 * @brief The impl of PurC variable manager.
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

#include "purc.h"

#include "config.h"

#include "internal.h"

#include "private/var-mgr.h"
#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"

#include <stdlib.h>
#include <string.h>

#define MSG_TYPE_CHANGE         "change"
#define SUB_TYPE_ATTACHED       "attached"
#define SUB_TYPE_DETACHED       "detached"
#define SUB_TYPE_DISPLACED      "displaced"

#define EVENT_ATTACHED          "change:attached"
#define EVENT_DETACHED          "change:detached"
#define EVENT_DISPLACED         "change:displaced"
#define EVENT_EXCEPT            "except:"

#define ATTR_KEY_ID             "id"

#define KEY_FLAG                "__name_observe"
#define KEY_NAME                "name"
#define KEY_MGR                 "mgr"

enum var_event_type {
    VAR_EVENT_TYPE_ATTACHED,
    VAR_EVENT_TYPE_DETACHED,
    VAR_EVENT_TYPE_DISPLACED,
    VAR_EVENT_TYPE_EXCEPT,
};

struct var_observe {
    char* name;
    enum var_event_type type;
    pcintr_stack_t stack;
};

struct pcvarmgr_named_variables_observe {
    char *name;
    pcintr_stack_t stack;
    pcvdom_element_t elem;
};

static purc_variant_t
pcvarmgr_build_event_observed(const char *name, pcvarmgr_t mgr)
{
    purc_variant_t v = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (!v) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t flag = purc_variant_make_boolean(true);
    if (!v) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (!purc_variant_object_set_by_static_ckey(v, KEY_FLAG, flag)) {
        goto failed;
    }
    purc_variant_unref(flag);

    purc_variant_t name_val = purc_variant_make_string(name, true);
    if (!name_val) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    if (!purc_variant_object_set_by_static_ckey(v, KEY_NAME, name_val)) {
        goto failed;
    }
    purc_variant_unref(name_val);

    purc_variant_t mgr_val = purc_variant_make_native(mgr, NULL);
    if (!mgr_val) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    if (!purc_variant_object_set_by_static_ckey(v, KEY_NAME, mgr_val)) {
        goto failed;
    }
    purc_variant_unref(mgr_val);

    return v;

failed:
    purc_variant_unref(v);

    return PURC_VARIANT_INVALID;
}

static int find_var_observe_idx(struct pcvarmgr* mgr, const char* name,
        enum var_event_type type, pcintr_stack_t stack)
{
    size_t sz = pcutils_array_length(mgr->var_observers);
    for (size_t i = 0; i < sz; i++) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, i);
        if (strcmp(name, obs->name) == 0
                && obs->type == type && obs->stack == stack) {
            return i;
        }
    }
    return -1;
}

static struct var_observe* find_var_observe(struct pcvarmgr* mgr,
        const char* name, enum var_event_type type, pcintr_stack_t stack)
{
    int idx = find_var_observe_idx(mgr, name, type, stack);
    if (idx == -1) {
        return NULL;
    }
    return pcutils_array_get(mgr->var_observers, idx);
}

static bool mgr_grow_handler(purc_variant_t source, pcvar_op_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(nr_args);

    if (ctxt == NULL) {
        return true;
    }

    pcvarmgr_t mgr = (pcvarmgr_t)ctxt;

    purc_variant_t type = purc_variant_make_string(MSG_TYPE_CHANGE, false);
    if (type == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t sub_type = purc_variant_make_string(SUB_TYPE_ATTACHED,
            false);
    if (sub_type == PURC_VARIANT_INVALID) {
        purc_variant_unref(type);
        return false;
    }

    const char* name = purc_variant_get_string_const(argv[0]);

    size_t sz = pcutils_array_length(mgr->var_observers);
    for (size_t i = 0; i < sz; i++) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, i);
        if (strcmp(name, obs->name) == 0
                && obs->type == VAR_EVENT_TYPE_ATTACHED) {
            pcintr_dispatch_message_ex(obs->stack, source, type, sub_type,
                    PURC_VARIANT_INVALID);
        }
    }

    purc_variant_unref(sub_type);
    purc_variant_unref(type);
    return true;
}

static bool mgr_shrink_handler(purc_variant_t source, pcvar_op_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(nr_args);

    if (ctxt == NULL) {
        return true;
    }

    pcvarmgr_t mgr = (pcvarmgr_t)ctxt;

    purc_variant_t type = purc_variant_make_string(MSG_TYPE_CHANGE, false);
    if (type == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t sub_type = purc_variant_make_string(SUB_TYPE_DETACHED,
            false);
    if (sub_type == PURC_VARIANT_INVALID) {
        purc_variant_unref(type);
        return false;
    }

    const char* name = purc_variant_get_string_const(argv[0]);

    size_t sz = pcutils_array_length(mgr->var_observers);
    for (size_t i = 0; i < sz; i++) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, i);
        if (strcmp(name, obs->name) == 0
                && obs->type == VAR_EVENT_TYPE_DETACHED) {
            pcintr_dispatch_message_ex(obs->stack, source, type, sub_type,
                    PURC_VARIANT_INVALID);
        }
    }

    purc_variant_unref(sub_type);
    purc_variant_unref(type);
    return true;
}

static bool mgr_change_handler(purc_variant_t source, pcvar_op_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(nr_args);

    if (ctxt == NULL) {
        return true;
    }

    pcvarmgr_t mgr = (pcvarmgr_t)ctxt;

    purc_variant_t type = purc_variant_make_string(MSG_TYPE_CHANGE, false);
    if (type == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t sub_type = purc_variant_make_string(SUB_TYPE_DISPLACED,
            false);
    if (sub_type == PURC_VARIANT_INVALID) {
        purc_variant_unref(type);
        return false;
    }

    const char* name = purc_variant_get_string_const(argv[0]);

    size_t sz = pcutils_array_length(mgr->var_observers);
    for (size_t i = 0; i < sz; i++) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, i);
        if (strcmp(name, obs->name) == 0
                && obs->type == VAR_EVENT_TYPE_DISPLACED) {
            pcintr_dispatch_message_ex(obs->stack, source, type, sub_type,
                    PURC_VARIANT_INVALID);
        }
    }

    purc_variant_unref(sub_type);
    purc_variant_unref(type);
    return true;
}

#define DEF_ARRAY_SIZE 10
pcvarmgr_t pcvarmgr_create(void)
{
    pcvarmgr_t mgr = (pcvarmgr_t)calloc(1,
            sizeof(struct pcvarmgr));
    if (!mgr) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto err_ret;
    }

    mgr->object = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (mgr->object == PURC_VARIANT_INVALID) {
        goto err_free_mgr;
    }

    mgr->grow_listener = purc_variant_register_post_listener(mgr->object,
        PCVAR_OPERATION_GROW, mgr_grow_handler, mgr);
    if (!mgr->grow_listener) {
        goto err_clear_object;
    }

    mgr->shrink_listener = purc_variant_register_post_listener(mgr->object,
        PCVAR_OPERATION_SHRINK, mgr_shrink_handler, mgr);
    if (!mgr->shrink_listener) {
        goto err_revoke_grow_listener;
    }

    mgr->change_listener = purc_variant_register_post_listener(mgr->object,
        PCVAR_OPERATION_CHANGE, mgr_change_handler, mgr);
    if (!mgr->change_listener) {
        goto err_revoke_shrink_listener;
    }

    mgr->var_observers = pcutils_array_create();
    unsigned int ret = pcutils_array_init(mgr->var_observers, DEF_ARRAY_SIZE);
    if (PURC_ERROR_OK != ret) {
        purc_set_error(ret);
        goto err_revoke_change_listener;
    }

    return mgr;

err_revoke_change_listener:
    purc_variant_revoke_listener(mgr->object, mgr->change_listener);

err_revoke_shrink_listener:
    purc_variant_revoke_listener(mgr->object, mgr->shrink_listener);

err_revoke_grow_listener:
    purc_variant_revoke_listener(mgr->object, mgr->grow_listener);

err_clear_object:
    purc_variant_unref(mgr->object);

err_free_mgr:
    free(mgr);

err_ret:
    return NULL;
}

int pcvarmgr_destroy(pcvarmgr_t mgr)
{
    if (mgr) {
        PC_ASSERT(mgr->node.rb_parent == NULL);
        size_t sz = pcutils_array_length(mgr->var_observers);
        for (size_t i = 0; i < sz; i++) {
            struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                    mgr->var_observers, i);
            free(obs->name);
            free(obs);
        }
        pcutils_array_destroy(mgr->var_observers, true);
        purc_variant_revoke_listener(mgr->object, mgr->grow_listener);
        purc_variant_revoke_listener(mgr->object, mgr->shrink_listener);
        purc_variant_revoke_listener(mgr->object, mgr->change_listener);
        purc_variant_unref(mgr->object);
        free(mgr);
    }
    return 0;
}

bool pcvarmgr_add(pcvarmgr_t mgr, const char* name,
        purc_variant_t variant)
{
    if (purc_variant_is_undefined(variant)) {
        return pcvarmgr_remove_ex(mgr, name, true);
    }

    if (mgr == NULL || mgr->object == PURC_VARIANT_INVALID
            || name == NULL || !variant) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return false;
    }

    purc_variant_t k = purc_variant_make_string(name, true);
    if (k == PURC_VARIANT_INVALID) {
        return false;
    }
    bool ret = false;
    purc_variant_t v = purc_variant_object_get(mgr->object, k);
    if (v == PURC_VARIANT_INVALID) {
        purc_clr_error();
        ret = purc_variant_object_set(mgr->object, k, variant);
    }
    else {
        enum purc_variant_type type = purc_variant_get_type(v);
        switch (type) {
        case PURC_VARIANT_TYPE_OBJECT:
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
            // XXX: observe on=$name
            ret = purc_variant_container_displace(v, variant, false);
            break;

        default:
            // XXX: observe on=$name
            ret = purc_variant_object_set(mgr->object, k, variant);
            break;
        }
    }

    purc_variant_unref(k);
    return ret;
}

purc_variant_t pcvarmgr_get(pcvarmgr_t mgr, const char* name)
{
    if (mgr == NULL || name == NULL) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;
    v = purc_variant_object_get_by_ckey(mgr->object, name);
    if (v) {
        return v;
    }

    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

bool pcvarmgr_remove_ex(pcvarmgr_t mgr, const char* name, bool silently)
{
    if (name) {
        return purc_variant_object_remove_by_static_ckey(mgr->object,
                name, silently);
    }
    return false;
}

bool pcvarmgr_dispatch_except(pcvarmgr_t mgr, const char* name,
        const char* except)
{
    purc_variant_t type = purc_variant_make_string(MSG_TYPE_CHANGE, false);
    if (type == PURC_VARIANT_INVALID) {
        return false;
    }

    purc_variant_t sub_type = purc_variant_make_string(except, false);
    if (sub_type == PURC_VARIANT_INVALID) {
        purc_variant_unref(type);
        return false;
    }

    size_t sz = pcutils_array_length(mgr->var_observers);
    for (size_t i = 0; i < sz; i++) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, i);
        if (strcmp(name, obs->name) == 0
                && obs->type == VAR_EVENT_TYPE_EXCEPT) {
            pcintr_dispatch_message_ex(obs->stack, mgr->object, type, sub_type,
                    PURC_VARIANT_INVALID);
        }
    }

    purc_variant_unref(sub_type);
    purc_variant_unref(type);
    return true;
}

static purc_variant_t pcvarmgr_add_observer(pcvarmgr_t mgr, const char* name,
        const char* event)
{
    enum var_event_type type = VAR_EVENT_TYPE_ATTACHED;
    if (strcmp(event, EVENT_ATTACHED) == 0) {
        type = VAR_EVENT_TYPE_ATTACHED;
    }
    else if (strcmp(event, EVENT_DETACHED) == 0) {
        type = VAR_EVENT_TYPE_DETACHED;
    }
    else if (strcmp(event, EVENT_DISPLACED) == 0) {
        type = VAR_EVENT_TYPE_DISPLACED;
    }
    else if (strncmp(event, EVENT_EXCEPT, strlen(EVENT_EXCEPT)) == 0) {
        type = VAR_EVENT_TYPE_EXCEPT;
    }

    pcintr_stack_t stack = pcintr_get_stack();
    struct var_observe* obs = find_var_observe(mgr, name, type, stack);
    if (obs != NULL) {
        return mgr->object;
    }

    obs = (struct var_observe*) malloc(sizeof(struct var_observe));
    if (obs == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    obs->name = strdup(name);
    obs->type = type;
    obs->stack = stack;
    unsigned int ret = pcutils_array_push(mgr->var_observers, obs);
    if (ret == PURC_ERROR_OK) {
        return mgr->object;
    }

    free(obs->name);
    free(obs);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t pcvarmgr_remove_observer(pcvarmgr_t mgr, const char* name,
        const char* event)
{
    enum var_event_type type = VAR_EVENT_TYPE_ATTACHED;
    if (strcmp(event, EVENT_ATTACHED) == 0) {
        type = VAR_EVENT_TYPE_ATTACHED;
    }
    else if (strcmp(event, EVENT_DETACHED) == 0) {
        type = VAR_EVENT_TYPE_DETACHED;
    }
    else if (strcmp(event, EVENT_DISPLACED) == 0) {
        type = VAR_EVENT_TYPE_DISPLACED;
    }

    pcintr_stack_t stack = pcintr_get_stack();
    int idx = find_var_observe_idx(mgr, name, type, stack);

    if (idx != -1) {
        struct var_observe* obs = (struct var_observe*) pcutils_array_get(
                mgr->var_observers, idx);
        free(obs->name);
        free(obs);
        pcutils_array_delete(mgr->var_observers, idx, 1);
        return mgr->object;
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
_find_named_scope_var_in_vdom(pcvdom_element_t elem, const char* name, pcvarmgr_t* mgr)
{
    if (!elem || !name) {
        PC_ASSERT(name); // FIXME: still recoverable???
        purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;

again:

    v = pcintr_get_scope_variable(elem, name);
    if (v) {
        if (mgr) {
            *mgr = pcvdom_element_get_variables(elem);
        }
        return v;
    }

    elem = pcvdom_element_parent(elem);
    if (elem)
        goto again;

    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
_find_named_scope_var(struct pcintr_stack_frame *frame, const char* name, pcvarmgr_t* mgr)
{
    if (frame->scope)
        return _find_named_scope_var_in_vdom(frame->scope, name, mgr);

    pcvdom_element_t elem = frame->pos;

    if (!elem || !name) {
        PC_ASSERT(name); // FIXME: still recoverable???
        purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;

again:

    v = pcintr_get_scope_variable(elem, name);
    if (v) {
        if (mgr) {
            *mgr = pcvdom_element_get_variables(elem);
        }
        return v;
    }

    frame = pcintr_stack_frame_get_parent(frame);
    if (frame) {
        if (frame->scope)
            return _find_named_scope_var_in_vdom(frame->scope, name, mgr);

        elem = frame->pos;
        if (elem)
            goto again;
    }

    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
find_doc_buildin_var(purc_vdom_t vdom, const char* name)
{
    PC_ASSERT(name);
    if (!vdom) {
        purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcvdom_document_get_variable(vdom, name);
    if (v) {
        return v;
    }
    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t find_inst_var(const char* name)
{
    if (!name) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    pcvarmgr_t varmgr = pcinst_get_variables();
    if (varmgr == NULL) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcvarmgr_get(varmgr, name);
    if (v) {
        return v;
    }
    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
_find_named_temp_var(struct pcintr_stack_frame *frame, const char *name)
{
    struct pcintr_stack_frame *p = frame;

again:

    if (p == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_NOT_FOUND);
        return PURC_VARIANT_INVALID;
    }

    do {
        purc_variant_t tmp;
        tmp = pcintr_get_exclamation_var(p);
        if (tmp == PURC_VARIANT_INVALID)
            break;

        if (purc_variant_is_object(tmp) == false)
            break;

        purc_variant_t v;
        v = purc_variant_object_get_by_ckey(tmp, name);
        if (v == PURC_VARIANT_INVALID)
            break;

        return v;
    } while (0);

    p = pcintr_stack_frame_get_parent(p);

    goto again;
}

purc_variant_t
pcintr_find_named_var(pcintr_stack_t stack, const char* name)
{
    if (!stack || !name) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    purc_variant_t v;
    v = _find_named_temp_var(frame, name);
    if (v) {
        purc_clr_error();
        return v;
    }

    v = _find_named_scope_var(frame, name, NULL);
    if (v) {
        purc_clr_error();
        return v;
    }

    v = find_doc_buildin_var(stack->vdom, name);
    if (v) {
        purc_clr_error();
        return v;
    }

    v = find_inst_var(name);
    if (v) {
        purc_clr_error();
        return v;
    }

    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PURC_VARIANT_INVALID;
}

enum purc_symbol_var _to_symbol(char symbol)
{
    switch (symbol) {
    case '?':
        return PURC_SYMBOL_VAR_QUESTION_MARK;
    case '<':
        return PURC_SYMBOL_VAR_LESS_THAN;
    case '@':
        return PURC_SYMBOL_VAR_AT_SIGN;
    case '!':
        return PURC_SYMBOL_VAR_EXCLAMATION;
    case ':':
        return PURC_SYMBOL_VAR_COLON;
    case '=':
        return PURC_SYMBOL_VAR_EQUAL;
    case '%':
        return PURC_SYMBOL_VAR_PERCENT_SIGN;
    case '^':
        return PURC_SYMBOL_VAR_CARET;
    default:
        // FIXME: NotFound???
        purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "symbol:%c", symbol);
        return PURC_SYMBOL_VAR_MAX;
    }
}

purc_variant_t
pcintr_get_symbolized_var (pcintr_stack_t stack, unsigned int number,
        char symbol)
{
    enum purc_symbol_var symbol_var = _to_symbol(symbol);
    if (symbol_var == PURC_SYMBOL_VAR_MAX) {
        purc_set_error(PURC_ERROR_BAD_NAME);
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    for (unsigned int i = 0; i < number; i++) {
        frame = pcintr_stack_frame_get_parent(frame);
    }

    if (!frame) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = pcintr_get_symbol_var(frame, symbol_var);
    PC_ASSERT(v != PURC_VARIANT_INVALID);
    if (v != PURC_VARIANT_INVALID) {
        purc_clr_error();
        return v;
    }
    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "symbol:%c", symbol);
    return PURC_VARIANT_INVALID;
}

purc_variant_t
pcintr_find_anchor_symbolized_var(pcintr_stack_t stack, const char *anchor,
        char symbol)
{
    purc_variant_t ret = PURC_VARIANT_INVALID;
    enum purc_symbol_var symbol_var = _to_symbol(symbol);
    if (symbol_var == PURC_SYMBOL_VAR_MAX) {
        purc_set_error(PURC_ERROR_BAD_NAME);
        return PURC_VARIANT_INVALID;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);

    while (frame) {
        pcvdom_element_t elem = frame->pos;
        purc_variant_t elem_id = pcvdom_element_eval_attr_val(elem, ATTR_KEY_ID);
        if (!elem_id) {
            frame = pcintr_stack_frame_get_parent(frame);
            continue;
        }

        if (purc_variant_is_string(elem_id)) {
            const char *id = purc_variant_get_string_const(elem_id);
            if (id && strcmp(id, anchor) == 0) {
                ret = pcintr_get_symbol_var(frame, symbol_var);
                if (ret == PURC_VARIANT_INVALID) {
                    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND,
                            "symbol:%c", symbol);
                }
                else {
                    purc_clr_error();
                }
                purc_variant_unref(elem_id);
                return ret;
            }
        }
        purc_variant_unref(elem_id);
        frame = pcintr_stack_frame_get_parent(frame);
    }

    return PURC_VARIANT_INVALID;
}

static bool
_unbind_named_temp_var(struct pcintr_stack_frame *frame, const char *name)
{
    struct pcintr_stack_frame *p = frame;

again:

    if (p == NULL)
        return false;

    do {
        purc_variant_t tmp;
        tmp = pcintr_get_exclamation_var(p);
        if (tmp == PURC_VARIANT_INVALID)
            break;

        if (purc_variant_is_object(tmp) == false)
            break;

        purc_variant_t v;
        v = purc_variant_object_get_by_ckey(tmp, name);
        if (v == PURC_VARIANT_INVALID)
            break;

        return purc_variant_object_remove_by_static_ckey(tmp, name, false);
    } while (0);

    p = pcintr_stack_frame_get_parent(p);

    goto again;
}

static bool
_unbind_named_scope_var(pcvdom_element_t elem, const char* name)
{
    if (!elem) {
        return false;
    }

    purc_variant_t v = pcintr_get_scope_variable(elem, name);
    if (v) {
        return pcintr_unbind_scope_variable(elem, name);
    }

    pcvdom_element_t parent = pcvdom_element_parent(elem);
    if (parent) {
        return _unbind_named_scope_var(parent, name);
    }
    else {
        // FIXME: vdom.c:563
        purc_clr_error();
    }
    return false;
}

static bool
_unbind_doc_buildin_var(purc_vdom_t vdom, const char* name)
{
    purc_variant_t v = pcvdom_document_get_variable(vdom, name);
    if (v) {
        return pcvdom_document_unbind_variable(vdom, name);
    }
    return false;
}

int
pcintr_unbind_named_var(pcintr_stack_t stack, const char *name)
{
    if (!stack || !name) {
        return PCVARIANT_ERROR_NOT_FOUND;
    }

    struct pcintr_stack_frame* frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    if (_unbind_named_temp_var(frame, name)) {
        return PURC_ERROR_OK;
    }

    if (_unbind_named_scope_var(frame->pos, name)) {
        return PURC_ERROR_OK;
    }

    if (_unbind_doc_buildin_var(stack->vdom, name)) {
        return PURC_ERROR_OK;
    }
    purc_set_error_with_info(PCVARIANT_ERROR_NOT_FOUND, "name:%s", name);
    return PCVARIANT_ERROR_NOT_FOUND;
}

static pcvarmgr_t
find_named_var_mgr(pcintr_stack_t stack, const char *name)
{
    purc_variant_t v = find_doc_buildin_var(stack->vdom, name);
    if (v) {
        purc_clr_error();
        return pcvdom_document_get_variables(stack->vdom);
    }

    v = find_inst_var(name);
    if (v) {
        purc_clr_error();
        return pcinst_get_variables();
    }
    // default
    return pcvdom_document_get_variables(stack->vdom);
}

purc_variant_t
pcintr_get_named_var_observed(pcintr_stack_t stack, const char *name)
{
    if (!stack || !name) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    pcvarmgr_t mgr = find_named_var_mgr(stack, name);
    return mgr? mgr->object : PURC_VARIANT_INVALID;
}

purc_variant_t
pcintr_add_named_var_observer(pcintr_stack_t stack, const char* name,
        const char* event)
{
    UNUSED_PARAM(event);
    if (!stack || !name) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    pcvarmgr_t mgr = find_named_var_mgr(stack, name);
    return mgr ? pcvarmgr_add_observer(mgr, name, event) :
        PURC_VARIANT_INVALID;
}

purc_variant_t
pcintr_remove_named_var_observer(pcintr_stack_t stack, const char* name,
        const char* event)
{
    UNUSED_PARAM(event);
    if (!stack || !name) {
        PC_ASSERT(0); // FIXME: still recoverable???
        return PURC_VARIANT_INVALID;
    }

    pcvarmgr_t mgr = NULL;
    mgr = pcvdom_document_get_variables(stack->vdom);
    purc_variant_t observed = pcvarmgr_remove_observer(mgr, name, event);
    if (observed != PURC_VARIANT_INVALID) {
        return observed;
    }

    mgr = pcinst_get_variables();
    observed = pcvarmgr_remove_observer(mgr, name, event);
    if (observed != PURC_VARIANT_INVALID) {
        return observed;
    }

    return PURC_VARIANT_INVALID;
}

static bool
match_observe(void *native_entity, purc_variant_t val)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(val);
    if (!purc_variant_is_object(val)) {
        return false;
    }

    purc_variant_t flag = purc_variant_object_get_by_ckey(val, KEY_FLAG);
    if (!flag) {
        purc_clr_error();
        return false;
    }

    struct pcvarmgr_named_variables_observe *obs =
        (struct pcvarmgr_named_variables_observe*)native_entity;

    purc_variant_t name_val = purc_variant_object_get_by_ckey(val, KEY_NAME);
    if (!name_val) {
        purc_clr_error();
        return false;
    }

    if (strcmp(obs->name, purc_variant_get_string_const(name_val)) != 0) {
        return false;
    }

    purc_variant_t mgr_val = purc_variant_object_get_by_ckey(val, KEY_MGR);
    if (!mgr_val || purc_variant_is_native(mgr_val)) {
        purc_clr_error();
        return false;
    }

    void *comp = purc_variant_native_get_entity(mgr_val);

    pcvdom_element_t elem = obs->elem;
    while (elem) {
        pcvarmgr_t varmgr = pcvdom_element_get_variables(obs->elem);
        if (varmgr == comp) {
            return true;
        }
        elem  = pcvdom_element_parent(elem);
    }

    pcvarmgr_t mgr = pcvdom_document_get_variables(obs->stack->vdom);
    if (mgr == comp) {
        return true;
    }
    return false;
}

static bool
on_observe(void *native_entity,
        const char *event_name, const char *event_subname)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(event_subname);
    return true;
}


static void
named_destroy(struct pcvarmgr_named_variables_observe *named)
{
    if (named->name) {
        free(named->name);
    }
    free(named);
}

static void
on_release(void *native_entity)
{
    UNUSED_PARAM(native_entity);

    PC_ASSERT(native_entity);
    struct pcvarmgr_named_variables_observe *named =
        (struct pcvarmgr_named_variables_observe*)native_entity;
    named_destroy(named);
}

purc_variant_t
pcintr_get_named_var_for_observed(pcintr_stack_t stack, const char *name,
        pcvdom_element_t elem)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(name);
    UNUSED_PARAM(elem);
    static struct purc_native_ops ops = {
        .property_getter            = NULL,
        .property_setter            = NULL,
        .property_eraser            = NULL,
        .property_cleaner           = NULL,

        .updater                    = NULL,
        .cleaner                    = NULL,
        .eraser                     = NULL,
        .match_observe              = match_observe,

        .on_observe                = on_observe,
        .on_release                = on_release,
    };

    struct pcvarmgr_named_variables_observe *named =
        (struct pcvarmgr_named_variables_observe*)calloc(1, sizeof(*named));
    if (!named) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    named->name = strdup(name);
    if (!named->name) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    named->stack = stack;
    named->elem = elem;

    purc_variant_t v = purc_variant_make_native(named, &ops);
    if (v == PURC_VARIANT_INVALID) {
        named_destroy(named);
        return PURC_VARIANT_INVALID;
    }

    return v;
}

purc_variant_t
pcintr_get_named_var_for_event(pcintr_stack_t stack, const char *name)
{
    pcvarmgr_t mgr = pcvdom_document_get_variables(stack->vdom);
    return pcvarmgr_build_event_observed(name, mgr);
}

