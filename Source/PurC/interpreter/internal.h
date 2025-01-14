/**
 * @file internal.h
 * @author Xu Xiaohong
 * @date 2021/12/18
 * @brief The internal interfaces for interpreter/internal
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
 */

#ifndef PURC_INTERPRETER_INTERNAL_H
#define PURC_INTERPRETER_INTERNAL_H

#include "purc-macros.h"
#include "purc-document.h"

#include "private/interpreter.h"
#include "private/fetcher.h"

#include "keywords.h"

#ifndef __cplusplus                        /* { */
#include "../vdom/vdom-internal.h"
#endif                                    /* } */

#define PLOG(...) do {                                                        \
    FILE *fp = fopen("/tmp/plog.log", "a+");                                  \
    fprintf(fp, ##__VA_ARGS__);                                               \
    fclose(fp);                                                               \
    fprintf(stderr, ##__VA_ARGS__);                                           \
} while (0)


#define PLINE()   PLOG(">%s:%d:%s\n", __FILE__, __LINE__, __func__)

#define PCINTR_HVML_RUN_SCHEMA                "hvml+run://"
#define PCINTR_LEN_HVML_RUN_SCHEMA            11

#define PCINTR_HVML_RUN_RES_CRTN              "/CRTN/"
#define PCINTR_HVML_RUN_RES_CHAN              "/CHAN/"

#define PCINTR_LEN_HVML_RUN_RES               4
#define PCINTR_HVML_RUN_CURR_ID               "-"
#define PCINTR_RDR_NORETURN_REQUEST_ID        "-"

#define PCINTR_EXCLAMATION_EVENT_NAME         "_eventName"
#define PCINTR_EXCLAMATION_EVENT_SOURCE       "_eventSource"
#define PCINTR_EXCLAMATION_EVENT_REQUEST_ID   "_eventRequestId"

struct pcvdom_template {
    struct pcvcm_node            *vcm;
    bool                          to_free;
    purc_variant_t                type;
};

struct pcintr_observer_task {
    struct list_head              ln;
    int                           cor_stage;
    int                           cor_state;
    pcintr_stack_t                stack;
    pcvdom_element_t              pos;
    pcvdom_element_t              scope;
    pcdoc_element_t               edom_element;
    purc_variant_t                payload;
    purc_variant_t                event_name;
    purc_variant_t                source;
    purc_variant_t                request_id;
};

enum VIA {
    VIA_UNDEFINED,
    VIA_LOAD,
    VIA_GET,
    VIA_POST,
    VIA_DELETE,
};

enum HVML_RUN_URI_TYPE {
    HVML_RUN_URI_INVALID,
    HVML_RUN_URI_FULL,
    HVML_RUN_URI_OMIT_SCHEMA,
    HVML_RUN_URI_OMIT_SCHEMA_AND_HOST,
};

enum HVML_RUN_RES_TYPE {
#define HVML_RUN_RES_TYPE_NAME_INVALID          "INVALID"
    HVML_RUN_RES_TYPE_INVALID,
#define HVML_RUN_RES_TYPE_NAME_CRTN             "CRTN"
    HVML_RUN_RES_TYPE_CRTN,
#define HVML_RUN_RES_TYPE_NAME_CHAN             "CHAN"
    HVML_RUN_RES_TYPE_CHAN
};

enum pcintr_request_id_type {
#define PCINTR_REQUEST_ID_TYPE_NAME_INVALID        "INVALID"
    PCINTR_REQUEST_ID_TYPE_INVALID,
#define PCINTR_REQUEST_ID_TYPE_NAME_ELEMENTS       "ELEMENTS"
    PCINTR_REQUEST_ID_TYPE_ELEMENTS,
#define PCINTR_REQUEST_ID_TYPE_NAME_CRTN           "crtn"
    PCINTR_REQUEST_ID_TYPE_CRTN,
#define PCINTR_REQUEST_ID_TYPE_NAME_CHAN           "CHAN"
    PCINTR_REQUEST_ID_TYPE_CHAN,
#define PCINTR_REQUEST_ID_TYPE_NAME_RDR            "RDR"
    PCINTR_REQUEST_ID_TYPE_RDR
};

PCA_EXTERN_C_BEGIN

void
pcintr_destroy_observer_list(struct list_head *observer_list);

struct pcintr_stack_frame_normal *
pcintr_push_stack_frame_normal(pcintr_stack_t stack);

void
pcintr_execute_one_step_for_ready_co(pcintr_coroutine_t co);

void
pcintr_dispatch_msg(void);

void
pcintr_conn_event_handler(pcrdr_conn *conn, const pcrdr_msg *msg);

void
pcintr_synchronize(void *ctxt, void (*routine)(void *ctxt));

void
pcintr_check_insertion_mode_for_normal_element(pcintr_stack_t stack);

typedef int (*pcintr_attr_f)(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name,
        struct pcvdom_attr *attr,
        void *ud);
int
pcintr_vdom_walk_attrs(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element, void *ud, pcintr_attr_f cb);

bool
pcintr_is_element_silently(struct pcvdom_element *element);

bool
pcintr_is_element_must_yield(struct pcvdom_element *element);

int
pcintr_set_symbol_var(struct pcintr_stack_frame *frame,
        enum purc_symbol_var symbol, purc_variant_t val);
purc_variant_t
pcintr_get_symbol_var(struct pcintr_stack_frame *frame,
        enum purc_symbol_var symbol);

int
pcintr_set_at_var(struct pcintr_stack_frame *frame, purc_variant_t val);
purc_variant_t
pcintr_get_at_var(struct pcintr_stack_frame *frame);
int
pcintr_refresh_at_var(struct pcintr_stack_frame *frame);

int
pcintr_set_question_var(struct pcintr_stack_frame *frame, purc_variant_t val);
purc_variant_t
pcintr_get_question_var(struct pcintr_stack_frame *frame);

int
pcintr_set_exclamation_var(struct pcintr_stack_frame *frame,
        purc_variant_t val);
purc_variant_t
pcintr_get_exclamation_var(struct pcintr_stack_frame *frame);

int
pcintr_inc_percent_var(struct pcintr_stack_frame *frame);
purc_variant_t
pcintr_get_percent_var(struct pcintr_stack_frame *frame);

// $<
void
pcintr_set_input_var(pcintr_stack_t stack, purc_variant_t val);

int
pcintr_set_edom_attribute(pcintr_stack_t stack, struct pcvdom_attr *attr,
        purc_variant_t val);

purc_variant_t
pcintr_load_from_uri(pcintr_stack_t stack, const char* uri);

purc_variant_t
pcintr_load_from_uri_async(pcintr_stack_t stack, const char* uri,
        enum pcfetcher_request_method method, purc_variant_t params,
        pcfetcher_response_handler handler, void* ctxt,
        purc_variant_t progress_event_dest);

bool
pcintr_save_async_request_id(pcintr_stack_t stack, purc_variant_t req_id);

bool
pcintr_remove_async_request_id(pcintr_stack_t stack, purc_variant_t req_id);

purc_variant_t
pcintr_load_vdom_fragment_from_uri(pcintr_stack_t stack, const char* uri);

purc_variant_t
pcintr_doc_query(purc_coroutine_t cor, const char* css, bool silently);

purc_variant_t
pcintr_template_make(void);

int
pcintr_template_set(purc_variant_t val, struct pcvcm_node *vcm,
        purc_variant_t type, bool to_free);


typedef int
(*pcintr_template_walk_cb)(struct pcvcm_node *vcm, void *ctxt);

void
pcintr_template_walk(purc_variant_t val, void *ctxt,
        pcintr_template_walk_cb cb);

void
pcintr_match_template(purc_variant_t val,
        purc_atom_t type, purc_variant_t *content);

typedef purc_variant_t
(*pcintr_attribute_op)(purc_variant_t left, purc_variant_t right);

pcintr_attribute_op
pcintr_attribute_get_op(enum pchvml_attr_operator op);

pcrdr_msg *pcintr_rdr_send_request_and_wait_response(struct pcrdr_conn *conn,
        pcrdr_msg_target target, uint64_t target_value, const char *operation,
        const char *request_id, pcrdr_msg_element_type element_type,
        const char *element, const char *property,
        pcrdr_msg_data_type data_type, purc_variant_t data, size_t data_len);

uint64_t pcintr_rdr_start_session(struct pcrdr_conn *conn, const char *protocol,
        uint64_t protocol_version, const char *host_name, const char *app_name,
        const char *runner_name);

/* retrieve handle of workspace according to the name */
uint64_t pcintr_rdr_retrieve_workspace(struct pcrdr_conn *conn,
        uint64_t session, const char *workspace_name);

uint64_t pcintr_rdr_create_workspace(struct pcrdr_conn *conn,
        uint64_t session, const char *name, const char *title);

bool pcintr_rdr_destroy_workspace(struct pcrdr_conn *conn,
        uint64_t session, uint64_t workspace);

// property: title, class, style
bool pcintr_rdr_update_workspace(struct pcrdr_conn *conn,
        uint64_t session, uint64_t workspace,
        const char *property, const char *value);

uint64_t pcintr_rdr_create_page(struct pcrdr_conn *conn, uint64_t workspace,
        pcrdr_page_type_k page_type, const char *target_group,
        const char *pag_name, const char *title, const char *classes,
        const char *layout_style, purc_variant_t toolkit_style);

bool pcintr_rdr_destroy_page(struct pcrdr_conn *conn, uint64_t workspace,
        pcrdr_page_type_k page_type, uint64_t page_handle);

bool
pcintr_rdr_update_page(struct pcrdr_conn *conn, uint64_t workspace,
        pcrdr_page_type_k page_type, uint64_t page_handle,
        const char *property, purc_variant_t value);

static inline uint64_t
pcintr_rdr_create_plain_window(struct pcrdr_conn *conn, uint64_t workspace,
        const char *target_group, const char *pag_name,
        const char *title, const char *classes,
        const char *layout_style, purc_variant_t toolkit_style)
{
    return pcintr_rdr_create_page(conn, workspace,
        PCRDR_PAGE_TYPE_PLAINWIN, target_group, pag_name, title, classes,
        layout_style, toolkit_style);
}

static inline bool
pcintr_rdr_destroy_plain_window(struct pcrdr_conn *conn,
        uint64_t workspace, uint64_t plain_window)
{
    return pcintr_rdr_destroy_page(conn, workspace,
        PCRDR_PAGE_TYPE_PLAINWIN, plain_window);
}

static inline bool
pcintr_rdr_update_plain_window(struct pcrdr_conn *conn, uint64_t workspace,
        uint64_t plain_window, const char *property, purc_variant_t value)
{
    return pcintr_rdr_update_page(conn, workspace,
        PCRDR_PAGE_TYPE_PLAINWIN, plain_window, property, value);
}

bool pcintr_rdr_set_page_groups(struct pcrdr_conn *conn,
        uint64_t workspace, const char *data);

bool pcintr_rdr_add_page_groups(struct pcrdr_conn *conn,
        uint64_t workspace, const char* page_groups);

bool pcintr_rdr_remove_page_group(struct pcrdr_conn *conn,
        uint64_t workspace, const char* page_group_id);

static inline uint64_t
pcintr_rdr_create_widget(struct pcrdr_conn *conn, uint64_t workspace,
        const char *target_group, const char *page_name,
        const char *title, const char *classes,
        const char *layout_style, purc_variant_t toolkit_style)
{
    return pcintr_rdr_create_page(conn, workspace,
        PCRDR_PAGE_TYPE_WIDGET, target_group, page_name, title, classes,
        layout_style, toolkit_style);
}

static inline bool
pcintr_rdr_destroy_widget(struct pcrdr_conn *conn,
        uint64_t workspace, uint64_t widget)
{
    return pcintr_rdr_destroy_page(conn, workspace,
        PCRDR_PAGE_TYPE_WIDGET, widget);
}

static inline bool
pcintr_rdr_update_widget(struct pcrdr_conn *conn, uint64_t workspace,
        uint64_t widget, const char *property, purc_variant_t value)
{
    return pcintr_rdr_update_page(conn, workspace,
        PCRDR_PAGE_TYPE_WIDGET, widget, property, value);
}

bool
pcintr_rdr_page_control_load(pcintr_stack_t stack);

int
pcintr_doc_op_to_rdr_op(pcdoc_operation_k op);

pcrdr_msg *
pcintr_rdr_send_dom_req(pcintr_stack_t stack, int op, const char *request_id,
        pcrdr_msg_element_type element_type, const char *css_selector,
        pcdoc_element_t element, const char* property,
        pcrdr_msg_data_type data_type, purc_variant_t data);

pcrdr_msg *
pcintr_rdr_send_dom_req_raw(pcintr_stack_t stack, int op, const char *request_id,
        pcrdr_msg_element_type element_type, const char *css_selector,
        pcdoc_element_t element, const char* property,
        pcrdr_msg_data_type data_type, const char *data, size_t len);

bool
pcintr_rdr_send_dom_req_simple(pcintr_stack_t stack, int op,
        const char *request_id,
        pcdoc_element_t element, const char* property,
        pcrdr_msg_data_type data_type, purc_variant_t data);

bool
pcintr_rdr_send_dom_req_simple_raw(pcintr_stack_t stack, int op,
        const char *request_id,
        pcdoc_element_t element, const char *property,
        pcrdr_msg_data_type data_type, const char *data, size_t len);

purc_variant_t
pcintr_rdr_call_method(pcintr_stack_t stack, const char *request_id,
        const char *css_selector, const char *method, purc_variant_t arg);

#define pcintr_rdr_dom_append_content(stack, element, content)                \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_K_OPERATION_APPEND,       \
            NULL, element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content, 0)

#define pcintr_rdr_dom_prepend_content(stack, element, content)               \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_K_OPERATION_PREPEND,      \
            NULL, element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content, 0)

#define pcintr_rdr_dom_insert_before_element(stack, element, content)         \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_K_OPERATION_INSERTBEFORE, \
            NULL, element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content, 0)

#define pcintr_rdr_dom_insert_after_element(stack, element, content)          \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_K_OPERATION_INSERTAFTER,  \
            NULL, element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content, 0)

#define pcintr_rdr_dom_displace_content(stack, element, content)              \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_K_OPERATION_DISPLACE,     \
            NULL, element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content, 0)

#define pcintr_rdr_dom_clear_element_content(stack, element)                  \
    pcintr_rdr_send_dom_req_simple(stack, PCRDR_K_OPERATION_CLEAR,            \
            NULL, element, NULL, PCRDR_MSG_DATA_TYPE_VOID, PURC_VARIANT_INVALID)

#define pcintr_rdr_dom_erase_element(stack, element)                          \
    pcintr_rdr_send_dom_req_simple(stack, PCRDR_K_OPERATION_ERASE,            \
            NULL, element, NULL, PCRDR_MSG_DATA_TYPE_VOID, PURC_VARIANT_INVALID)

#define pcintr_rdr_dom_erase_element_property(stack, element, prop)           \
    pcintr_rdr_send_dom_req_simple(stack, PCRDR_K_OPERATION_ERASE,            \
            NULL, element, prop, PCRDR_MSG_DATA_TYPE_VOID, PURC_VARIANT_INVALID)

#define pcintr_rdr_dom_update_element_content_text(stack, element, content)   \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_K_OPERATION_UPDATE,       \
            NULL, element, NULL, PCRDR_MSG_DATA_TYPE_TEXT, content, 0)

#define pcintr_rdr_dom_update_element_content_ejson(stack, element, data)     \
    pcintr_rdr_send_dom_req_simple(stack, PCRDR_K_OPERATION_UPDATE,           \
            NULL, element, NULL, PCRDR_MSG_DATA_TYPE_EJSON, data)

#define pcintr_rdr_dom_update_element_property(stack, element, prop, content) \
    pcintr_rdr_send_dom_req_simple_raw(stack, PCRDR_K_OPERATION_UPDATE,       \
            NULL, element, prop, PCRDR_MSG_DATA_TYPE_TEXT, content, 0)

bool
pcintr_rdr_dom_operate_element(pcintr_stack_t stack, pcdoc_element_t element,
        int op, const char *contents, size_t len);

purc_variant_t
pcintr_wrap_vdom(pcvdom_element_t vdom);

pcvdom_element_t
pcintr_get_vdom_from_variant(purc_variant_t val);

int
pcintr_bind_template(purc_variant_t templates,
        purc_variant_t type, purc_variant_t contents);

purc_variant_t
pcintr_template_expansion(purc_variant_t val);

purc_variant_t
pcintr_template_get_type(purc_variant_t val);

pcintr_coroutine_t
pcintr_coroutine_get_by_id(purc_atom_t id);


void
pcintr_exception_copy(struct pcintr_exception *exception);

void
pcintr_dump_stack(pcintr_stack_t stack);

void
pcintr_dump_c_stack(struct pcdebug_backtrace *bt);

void
pcintr_notify_to_stop(pcintr_coroutine_t co);

void
pcintr_revoke_all_hvml_observers(pcintr_stack_t stack);

void
pcintr_run_exiting_co(void *ctxt);

bool
pcintr_co_is_observed(pcintr_coroutine_t co);

void
pcintr_check_after_execution_full(struct pcinst *inst, pcintr_coroutine_t co);

void pcintr_coroutine_set_state_with_location(pcintr_coroutine_t co,
        enum pcintr_coroutine_state state,
        const char *file, int line, const char *func);

#define pcintr_coroutine_set_state(co, state) \
    pcintr_coroutine_set_state_with_location(co, state,\
            __FILE__, __LINE__, __func__)

int
pcintr_coroutine_clear_tasks(pcintr_coroutine_t co);

void
pcintr_coroutine_add_sub_exit_observer(pcintr_coroutine_t co);

void
pcintr_coroutine_add_last_msg_observer(pcintr_coroutine_t co);

/* ms */
double
pcintr_get_current_time(void);

void
pcintr_update_timestamp(struct pcinst *inst);

purc_atom_t
pcintr_schedule_child_co(purc_vdom_t vdom, purc_atom_t curator,
        const char *runner, const char *rdr_target, purc_variant_t request,
        const char *body_id, bool create_runner);

purc_atom_t
pcintr_schedule_child_co_from_string(const char *hvml, purc_atom_t curator,
        const char *runner, const char *rdr_target, purc_variant_t request,
        const char *body_id, bool create_runner);


/* for bind named variable */
bool
pcintr_match_id(pcintr_stack_t stack, struct pcvdom_element *elem,
        const char *id);

int
pcintr_bind_named_variable(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *name, purc_variant_t at,
        bool temporarily, bool runner_level_enable, purc_variant_t v);

pcvarmgr_t
pcintr_get_named_variable_mgr_by_at(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, purc_variant_t at, bool temporarily,
        bool runner_level_enable);

purc_vdom_t
pcintr_build_concurrently_call_vdom(pcintr_stack_t stack,
        pcvdom_element_t element);


int
pcintr_coroutine_dump(pcintr_coroutine_t co);

void
pcintr_handle_task(struct pcintr_observer_task *task);


typedef int (*walk_attr_cb)(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud);
int
pcintr_walk_attrs(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element, void *ud, walk_attr_cb cb);


purc_variant_t
pcintr_eval_vcm(pcintr_stack_t stack, struct pcvcm_node *node, bool silently);

enum pcfetcher_request_method
pcintr_method_from_via(enum VIA via);

bool
pcintr_match_exception(purc_atom_t except, purc_variant_t constant);

bool
pcintr_is_hvml_attr(const char *name);

int
pcintr_hvml_run_extract_host_name(const char *uri, char *host_name);

int
pcintr_hvml_run_extract_app_name(const char *uri, char *app_name);

int
pcintr_hvml_run_extract_runner_name(const char *uri, char *runner_name);

int
pcintr_hvml_run_extract_res_name(const char *uri,
        enum HVML_RUN_RES_TYPE *res_type, char *res_name);

bool
pcintr_parse_hvml_run_uri(const char *uri, char *host_name, char *app_name,
        char *runner_name, enum HVML_RUN_RES_TYPE *res_type, char *res_name);

bool
pcintr_is_valid_hvml_run_uri(const char *uri);

bool
pcintr_is_crtn_object(purc_variant_t v, purc_atom_t *cid);

/* used for request tag */
static inline const char *
pcintr_request_get_type_name(enum pcintr_request_id_type type) {
    switch (type) {
    case PCINTR_REQUEST_ID_TYPE_ELEMENTS:
        return PCINTR_REQUEST_ID_TYPE_NAME_ELEMENTS;

    case PCINTR_REQUEST_ID_TYPE_CRTN:
        return PCINTR_REQUEST_ID_TYPE_NAME_CRTN;

    case PCINTR_REQUEST_ID_TYPE_CHAN:
        return PCINTR_REQUEST_ID_TYPE_NAME_CHAN;

    case PCINTR_REQUEST_ID_TYPE_RDR:
        return PCINTR_REQUEST_ID_TYPE_NAME_RDR;

    case PCINTR_REQUEST_ID_TYPE_INVALID:
    default:
        return PCINTR_REQUEST_ID_TYPE_NAME_INVALID;
    }
}


bool
pcintr_is_request_id(purc_variant_t v);

purc_variant_t
pcintr_request_id_create(enum pcintr_request_id_type type, purc_atom_t rid,
        purc_atom_t cid, const char *res);

purc_atom_t
pcintr_request_id_get_rid(purc_variant_t v);

purc_atom_t
pcintr_request_id_get_cid(purc_variant_t v);

bool
pcintr_request_id_set_cid(purc_variant_t v, purc_atom_t cid);

enum pcintr_request_id_type
pcintr_request_id_get_type(purc_variant_t v);

const char *
pcintr_request_id_get_res(purc_variant_t v);

bool
pcintr_request_id_is_equal_to(purc_variant_t v1, purc_variant_t v2);

bool
pcintr_request_id_is_match(purc_variant_t v1, purc_variant_t v2);

int
pcintr_chan_post(const char *chan_name, purc_variant_t data);

bool
pcintr_is_crtn_observed(purc_variant_t v);

purc_variant_t
pcintr_crtn_observed_create(purc_atom_t cid);

purc_atom_t
pcintr_crtn_observed_get_cid(purc_variant_t v);

bool
pcintr_crtn_observed_is_match(purc_variant_t observed, purc_variant_t v);


PCA_EXTERN_C_END

#endif  /* PURC_INTERPRETER_INTERNAL_H */

