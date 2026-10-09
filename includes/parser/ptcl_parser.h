#ifndef PTCL_PARSER_H
#define PTCL_PARSER_H

#include <ptcl_node.h>
#include <ptcl_parser_error.h>
#include <ptcl_lexer_configuration.h>

#define PTCL_PARSER_MAX_DEPTH 1024
#define PTCL_PARSER_MAX_MODIFIERS_RECURSION 16
#define PTCL_PARSER_DEFAULT_INSTANCE_CAPACITY 16
#define PTCL_PARSER_DECL_INSERT_DEPTH 16
#define PTCL_PARSER_LANGUAGE_NAMESPACE_NAME "language"
#define PTCL_PARSER_STATEMENT_TYPE_NAME "statement"
#define PTCL_PARSER_EXPRESSION_TYPE_NAME "expression"
#define PTCL_PARSER_TOKEN_TYPE_NAME "token"

#define PTCL_PARSER_STATEMENTS_FUNC_NAME "statements"
#define PTCL_PARSER_INSERT_FUNC_NAME "insert"
#define PTCL_PARSER_ERROR_FUNC_NAME "error"
#define PTCL_PARSER_DEFINED_FUNC_NAME "defined"

const ptcl_name* ptcl_get_token_t_name(void);

const ptcl_name* ptcl_get_statement_t_name(void);

const ptcl_name* ptcl_get_expression_t_name(void);

const ptcl_name* ptcl_get_statements_func_name(void);

const ptcl_name* ptcl_get_defined_func_name(void);

const ptcl_name* ptcl_get_error_func_name(void);

const ptcl_name* ptcl_get_insert_func_name(void);

const ptcl_type* ptcl_get_statement_t(void);

const ptcl_type* ptcl_get_token_t(void);

static bool ptcl_parser_is_language_namespace(const ptcl_name* const name) 
{
    PTCL_VALIDATE_COND(name != NULL);
    return ptcl_string_equals(ptcl_name_value(name), PTCL_PARSER_LANGUAGE_NAMESPACE_NAME);
}

static bool ptcl_parser_is_statement_t_name(const ptcl_name* const name) 
{
    PTCL_VALIDATE_COND(name != NULL);
    if (ptcl_name_next(name) == NULL || !ptcl_parser_is_language_namespace(name))
    {
        return false;
    }

    return ptcl_string_equals(ptcl_name_value(ptcl_name_next(name)), PTCL_PARSER_STATEMENT_TYPE_NAME);
}

static bool ptcl_parser_is_expression_t_name(const ptcl_name* const name)
{
    PTCL_VALIDATE_COND(name != NULL);
    if (ptcl_name_next(name) == NULL || !ptcl_parser_is_language_namespace(name))
    {
        return false;
    }

    return ptcl_string_equals(ptcl_name_value(ptcl_name_next(name)), PTCL_PARSER_EXPRESSION_TYPE_NAME);
}

static bool ptcl_parser_is_token_t_name(const ptcl_name* const name)
{
    PTCL_VALIDATE_COND(name != NULL);
    if (ptcl_name_next(name) == NULL || !ptcl_parser_is_language_namespace(name))
    {
        return false;
    }

    return ptcl_string_equals(ptcl_name_value(ptcl_name_next(name)), PTCL_PARSER_TOKEN_TYPE_NAME);
}

static bool ptcl_parser_is_error_name(const ptcl_name* const name)
{
    PTCL_VALIDATE_COND(name != NULL);
    if (ptcl_name_next(name) == NULL || !ptcl_parser_is_language_namespace(name))
    {
        return false;
    }

    return ptcl_string_equals(ptcl_name_value(ptcl_name_next(name)), PTCL_PARSER_ERROR_FUNC_NAME);
}

static bool ptcl_parser_is_insert_name(const ptcl_name* const name)
{
    PTCL_VALIDATE_COND(name != NULL);
    if (ptcl_name_next(name) == NULL || !ptcl_parser_is_language_namespace(name))
    {
        return false;
    }

    return ptcl_string_equals(ptcl_name_value(ptcl_name_next(name)), PTCL_PARSER_INSERT_FUNC_NAME);
}

static bool ptcl_parser_is_insert_str(const char* const name)
{
    if (name == NULL)
    {
        return false;
    }

    return ptcl_string_equals(name, PTCL_PARSER_INSERT_FUNC_NAME);
}

#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

typedef enum ptcl_parser_flags
{
    PTCL_PARSER_NONE_FLAG = 0,
    PTCL_PARSER_CRITICAL_FLAG = 1 << 0,
    PTCL_PARSER_IN_SYNTAX_FLAG = 1 << 1,
    PTCL_PARSER_IN_TYPE_FLAG = 1 << 2,
    PTCL_PARSER_IGNORE_ERROR_FLAG = 1 << 3,
    PTCL_PARSER_ADD_ERRORS_FLAG = 1 << 4,
    PTCL_PARSER_EXCEPT_RETURN_FLAG = 1 << 5,
    PTCL_PARSER_IN_IF_FLAG = 1 << 6,
    PTCL_PARSER_IN_RETURN_FLAG = 1 << 7,
    PTCL_PARSER_IN_DOT_FLAG = 1 << 8
} ptcl_parser_flags;

typedef struct ptcl_parser_tokens_state
{
    size_t position;
    ptcl_token *tokens;
    size_t count;
    bool is_free;
} ptcl_parser_tokens_state;

typedef struct ptcl_parser_insert_state
{
    ptcl_parser_tokens_state state;
    size_t syntax_depth;
} ptcl_parser_insert_state;

typedef struct ptcl_parser_this_s_pair
{
    ptcl_func_body *body;
    size_t index;
} ptcl_parser_this_s_pair;

typedef struct ptcl_parser_function ptcl_parser_function;

typedef enum ptcl_parser_var_flags
{
    PTCL_PARSER_VAR_NONE_FLAG = 0,
    PTCL_PARSER_VAR_SYNTAX_WORD_FLAG = 1 << 0,
    PTCL_PARSER_VAR_FUNC_PTR_FLAG = 1 << 1,
    PTCL_PARSER_VAR_SYNTAX_FLAG = 1 << 2,
    PTCL_PARSER_VAR_SYNTAX_ANOM_FLAG = 1 << 3,
    PTCL_PARSER_VAR_USED_FLAG = 1 << 4,
    PTCL_PARSER_VAR_SOURCE_FLAG = 1 << 5,
    PTCL_PARSER_VAR_BUILT_IN_FLAG = 1 << 6
} ptcl_parser_var_flags;

static bool ptcl_parser_var_flags_is_syntax_word(
    ptcl_parser_var_flags flags)
{
    return PTCL_FLAGS_HAS(flags, PTCL_PARSER_VAR_SYNTAX_WORD_FLAG);
}

static bool ptcl_parser_var_flags_is_func_ptr(
    ptcl_parser_var_flags flags)
{
    return PTCL_FLAGS_HAS(flags, PTCL_PARSER_VAR_FUNC_PTR_FLAG);
}

static bool ptcl_parser_var_flags_is_syntax(
    ptcl_parser_var_flags flags)
{
    return PTCL_FLAGS_HAS(flags, PTCL_PARSER_VAR_SYNTAX_FLAG);
}

static bool ptcl_parser_var_flags_is_syntax_anom(
    ptcl_parser_var_flags flags)
{
    return PTCL_FLAGS_HAS(flags, PTCL_PARSER_VAR_SYNTAX_ANOM_FLAG);
}

static bool ptcl_parser_var_flags_is_used(
    ptcl_parser_var_flags flags)
{
    return PTCL_FLAGS_HAS(flags, PTCL_PARSER_VAR_USED_FLAG);
}

static bool ptcl_parser_var_flags_is_source(
    ptcl_parser_var_flags flags)
{
    return PTCL_FLAGS_HAS(flags, PTCL_PARSER_VAR_SOURCE_FLAG);
}

static bool ptcl_parser_var_flags_is_built_in(
    ptcl_parser_var_flags flags)
{
    return PTCL_FLAGS_HAS(flags, PTCL_PARSER_VAR_BUILT_IN_FLAG);
}

static ptcl_parser_var_flags ptcl_parser_var_flags_default(bool is_built_in)
{
    if (is_built_in)
    {
        return PTCL_PARSER_VAR_NONE_FLAG | PTCL_PARSER_VAR_BUILT_IN_FLAG;
    }

    return PTCL_PARSER_VAR_NONE_FLAG;
}

static ptcl_parser_var_flags ptcl_parser_var_flags_provider()
{
    return PTCL_PARSER_VAR_NONE_FLAG;
}

static ptcl_parser_var_flags ptcl_parser_var_flags_func()
{
    return PTCL_PARSER_VAR_FUNC_PTR_FLAG;
}

static ptcl_parser_var_flags ptcl_parser_var_flags_source()
{
    return PTCL_PARSER_VAR_SOURCE_FLAG;
}

typedef struct ptcl_parser_variable
{
    ptcl_name identifier;
    ptrdiff_t provider_index;
    ptcl_func_body *root;
    ptcl_type type;
    ptcl_expression *built_in;
    ptcl_parser_var_flags flags;
    bool is_out_of_scope;
} ptcl_parser_variable;

typedef struct ptcl_parser_typedata
{
    ptcl_func_body *root;
    ptcl_name identifier;
    ptrdiff_t provider_index;
    ptcl_type_typedata *typedata;
    bool is_out_of_scope;
} ptcl_parser_typedata;

typedef struct ptcl_parser_comp_type
{
    ptcl_name identifier;
    ptrdiff_t provider_index;
    ptcl_func_body *root;
    ptcl_type_comp_type *comp_type;
    ptcl_type_comp_type *static_type;
    bool is_auto_static;
    bool is_out_of_scope;
} ptcl_parser_comp_type;

typedef enum ptcl_parser_syntax_node_type
{
    ptcl_parser_syntax_node_word_type,
    ptcl_parser_syntax_node_variable_type,
    ptcl_parser_syntax_node_object_type_type,
    ptcl_parser_syntax_node_value_type
} ptcl_parser_syntax_node_type;

typedef struct ptcl_parser_syntax_word_node
{
    ptcl_token_type type;
    ptcl_name identifier;
} ptcl_parser_syntax_word_node;

typedef struct ptcl_parser_syntax_word_variable
{
    ptcl_type type;
    char *name;
    bool is_variadic;
} ptcl_parser_syntax_word_variable;

typedef struct ptcl_parser_tokens_node
{
    ptcl_token *tokens;
    size_t count;
} ptcl_parser_tokens_node;

typedef struct ptcl_parser_value_node
{
    ptcl_expression *value;
    ptcl_parser_tokens_state state;
} ptcl_parser_value_node;

typedef struct ptcl_parser_syntax_node
{
    ptcl_parser_syntax_node_type type;

    union
    {
        ptcl_parser_syntax_word_node word;
        ptcl_parser_syntax_word_variable variable;
        ptcl_type object_type;
        ptcl_parser_value_node value;
    };
} ptcl_parser_syntax_node;

typedef struct ptcl_parser_syntax
{
    ptcl_name identifier;
    ptcl_func_body *root;
    ptcl_parser_syntax_node *nodes;
    size_t count;
    size_t index;
    bool is_out_of_scope;
} ptcl_parser_syntax;

typedef struct ptcl_parser_syntax_pair
{
    ptcl_func_body *body;
    ptcl_func_body *previous_body;
    ptcl_parser_syntax syntax;
    ptcl_parser_tokens_state state;
} ptcl_parser_syntax_pair;

typedef struct ptcl_parser ptcl_parser;
typedef ptcl_expression *(*ptcl_built_in_function_t)(
    ptcl_parser *parser,
    ptcl_expression **arguments,
    size_t count,
    ptcl_location location,
    bool is_expression);

typedef struct ptcl_parser_function
{
    ptcl_name identifier;
    ptrdiff_t provider_index;
    ptcl_func_body *root;
    bool is_built_in;
    ptcl_built_in_function_t bind;
    ptcl_statement_func_decl func;
    bool is_out_of_scope;
} ptcl_parser_function;

typedef struct ptcl_parser_provider_variant
{
    ptcl_expression **values;
    size_t capacity;
    size_t count;
    ptrdiff_t member;
} ptcl_parser_provider_variant;

typedef struct ptcl_parser_provider
{
    ptcl_statement_type type;
    ptcl_attributes attributes;
    ptcl_stat_mods modifiers;
    ptcl_name identifier;
    ptcl_func_body *root;
    ptcl_statement_func_decl func_decl;
    ptcl_parser_provider_variant *variants;
    size_t variants_capacity;
    size_t variants_count;
    ptcl_parser_provider_variant* generators;
    size_t generators_capacity;
    size_t generators_count;
    size_t index;
    bool is_provider;
    bool is_out_of_scope;
} ptcl_parser_provider;

typedef enum ptcl_parser_instance_type
{
    ptcl_parser_instance_typedata_type,
    ptcl_parser_instance_comp_type_type,
    ptcl_parser_instance_function_type,
    ptcl_parser_instance_variable_type,
    ptcl_parser_instance_syntax_type
} ptcl_parser_instance_type;

typedef struct ptcl_parser_result
{
    ptcl_lexer_configuration *configuration;
    ptcl_func_body body;
    ptcl_parser_error *errors;
    size_t errors_count;
    ptcl_parser_syntax *syntaxes;
    size_t syntaxes_count;
    ptcl_parser_typedata *typedatas;
    size_t typedatas_count;
    ptcl_parser_comp_type *comp_types;
    size_t comp_types_count;
    ptcl_parser_function *functions;
    size_t functions_count;
    ptcl_parser_variable *variables;
    size_t variables_count;
    ptcl_parser_tokens_state *lated_states;
    size_t lated_states_count;
    ptcl_parser_this_s_pair *this_pairs;
    size_t this_pairs_count;
    ptcl_parser_provider *providers;
    size_t providers_count;
    ptcl_func_body** unique_pointers;
    size_t unique_pointers_count;
    bool is_critical;
} ptcl_parser_result;

typedef struct ptcl_parser_statement_info
{
    ptcl_statement_type type;
    ptcl_name name;
    ptcl_expression *expression;
    ptcl_stat_mods modifiers;
} ptcl_parser_statement_info;

typedef struct ptcl_parser_assign_dot_data
{
    bool *is_provider;
    ptcl_identifier *identifier;
    ptcl_argument *typedata_member;
    ptcl_expression *ctor;
    size_t ctor_member_index;
} ptcl_parser_assign_dot_data;

// TODO: expression -> expr with all other stuff with that
typedef enum ptcl_parser_expr_flags
{
    ptcl_parser_expr_none_flag = 0,
    ptcl_parser_expr_with_word_flag = 1 << 0,
    ptcl_parser_expr_only_value_flag = 1 << 1,
    ptcl_parser_expr_require_expression_flag = 1 << 2,
    ptcl_parser_expr_change_the_value_flag = 1 << 3,
    ptcl_parser_expr_ignore_excepted_flag = 1 << 4
} ptcl_parser_expr_flags;

static bool ptcl_parser_expr_flags_is_only_value(
    ptcl_parser_expr_flags flags)
{
    return PTCL_FLAGS_HAS(flags, ptcl_parser_expr_only_value_flag);
}

static bool ptcl_parser_expr_flags_is_change_value(
    ptcl_parser_expr_flags flags)
{
    return PTCL_FLAGS_HAS(flags, ptcl_parser_expr_change_the_value_flag);
}

static bool ptcl_parser_expr_flags_is_ignore_expected(
    ptcl_parser_expr_flags flags)
{
    return PTCL_FLAGS_HAS(flags, ptcl_parser_expr_ignore_excepted_flag);
}

static bool ptcl_parser_expr_flags_is_require_expr(
    ptcl_parser_expr_flags flags)
{
    return PTCL_FLAGS_HAS(flags, ptcl_parser_expr_require_expression_flag);
}

static bool ptcl_parser_expr_flags_is_with_word(
    ptcl_parser_expr_flags flags)
{
    return PTCL_FLAGS_HAS(flags, ptcl_parser_expr_with_word_flag);
}

static ptcl_parser_expr_flags ptcl_parser_expr_flags_default(
    bool is_expression)
{
    if (is_expression)
    {
        return ptcl_parser_expr_with_word_flag | ptcl_parser_expr_require_expression_flag;
    }

    return ptcl_parser_expr_with_word_flag;
}

static ptcl_parser_flags ptcl_parser_flags_default()
{
    return PTCL_PARSER_ADD_ERRORS_FLAG;
}

static ptcl_parser_insert_state ptcl_parser_insert_state_create(
    const ptcl_parser_tokens_state *const state, size_t syntax_depth)
{
    PTCL_VALIDATE_COND(state != NULL);
    return (ptcl_parser_insert_state){
        .state = *state,
        .syntax_depth = syntax_depth};
}

static ptcl_parser_statement_info ptcl_parser_statement_info_default()
{
    return (ptcl_parser_statement_info){
        .type = 0,
        .modifiers = ptcl_stat_mods_none_flag,
        .expression = NULL,
        .name = ptcl_name_empty};
}

static ptcl_parser_syntax_pair ptcl_parser_syntax_pair_create(
    const ptcl_parser_syntax *const syntax,
    const ptcl_parser_tokens_state *const state,
    ptcl_func_body *body,
    ptcl_func_body *previos_body)
{
    PTCL_VALIDATE_COND(syntax != NULL && state != NULL);
    return (ptcl_parser_syntax_pair){
        .syntax = *syntax,
        .state = *state,
        .body = body,
        .previous_body = previos_body};
}

static ptcl_parser_this_s_pair ptcl_parser_this_s_pair_create(
    ptcl_func_body *body,
    size_t index)
{
    return (ptcl_parser_this_s_pair){
        .body = body,
        .index = index};
}

static ptcl_parser_comp_type ptcl_parser_comp_type_create_provider(
    ptcl_func_body *root,
    const ptcl_name *const identifier,
    ptrdiff_t index)
{
    PTCL_VALIDATE_COND(identifier != NULL);
    return (ptcl_parser_comp_type){
        .root = root,
        .identifier = *identifier,
        .provider_index = (int)index,
        .is_out_of_scope = false};
}

static ptcl_parser_comp_type ptcl_parser_comp_type_create(
    ptcl_func_body *root,
    const ptcl_name *const identifier,
    ptcl_type_comp_type *type,
    bool is_static,
    bool is_auto_static)
{
    PTCL_VALIDATE_COND(identifier != NULL);
    return (ptcl_parser_comp_type){
        .root = root,
        .identifier = *identifier,
        .provider_index = -1,
        .static_type = is_static ? type : NULL,
        .comp_type = is_static ? NULL : type,
        .is_auto_static = is_auto_static,
        .is_out_of_scope = false};
}

static ptcl_parser_variable ptcl_parser_variable_create(
    const ptcl_name *const identifier,
    const ptcl_type *const type,
    ptcl_expression *built_in,
    bool is_built_in,
    ptcl_func_body *root)
{
    PTCL_VALIDATE_COND(identifier != NULL && type != NULL);
    return (ptcl_parser_variable){
        .identifier = *identifier,
        .provider_index = -1,
        .root = root,
        .is_out_of_scope = false,
        .type = *type,
        .built_in = built_in,
        .flags = ptcl_parser_var_flags_default(is_built_in)};
}

static ptcl_parser_variable ptcl_parser_variable_create_source(
    const ptcl_name* const identifier,
    const ptcl_type* const type,
    ptcl_expression* built_in,
    ptcl_func_body* root)
{
    PTCL_VALIDATE_COND(identifier != NULL && type != NULL);
    return (ptcl_parser_variable) {
        .identifier = *identifier,
            .provider_index = -1,
            .root = root,
            .is_out_of_scope = false,
            .type = *type,
            .built_in = built_in,
            .flags = ptcl_parser_var_flags_source()};
}

static ptcl_parser_variable ptcl_parser_variable_create_provider(
    ptcl_func_body *root,
    const ptcl_name *const identifier,
    ptrdiff_t index)
{
    PTCL_VALIDATE_COND(identifier != NULL);
    return (ptcl_parser_variable) {
        .identifier = *identifier,
            .provider_index = index,
            .root = root,
            .is_out_of_scope = false,
            .built_in = NULL,
            .flags = ptcl_parser_var_flags_provider()};
}

// TODO: more cool name
static ptcl_parser_variable ptcl_parser_variable_not_static_create(
    const ptcl_name *const identifier,
    const ptcl_type *const type,
    ptcl_expression *built_in,
    bool is_built_in,
    ptcl_func_body *root)
{
    PTCL_VALIDATE_COND(identifier != NULL && type != NULL);
    return (ptcl_parser_variable){
        .identifier = *identifier,
        .provider_index = -1,
        .root = root,
        .is_out_of_scope = false,
        .type = *type,
        .built_in = built_in,
        .flags = ptcl_parser_var_flags_default(is_built_in)};
}

static ptcl_parser_variable ptcl_parser_func_variable_create(
    const ptcl_name *const identifier,
    const ptcl_type *const type,
    ptcl_func_body *root)
{
    PTCL_VALIDATE_COND(identifier != NULL && type != NULL);
    return (ptcl_parser_variable){
        .identifier = *identifier,
        .provider_index = -1,
        .root = root,
        .is_out_of_scope = false,
        .type = *type,
        .flags = ptcl_parser_var_flags_func()};
}

static ptcl_parser_function ptcl_parser_function_create_provider(
    ptcl_func_body *root,
    const ptcl_name *const identifier,
    ptrdiff_t index)
{
    PTCL_VALIDATE_COND(identifier != NULL);
    return (ptcl_parser_function){
        .identifier = *identifier,
        .provider_index = (int)index,
        .root = root,
        .is_out_of_scope = false,
        .is_built_in = false};
};

static ptcl_parser_function ptcl_parser_function_create(
    ptcl_func_body *root,
    const ptcl_statement_func_decl *const func)
{
    PTCL_VALIDATE_COND(func != NULL);
    return (ptcl_parser_function){
        .identifier = func->identifier,
        .provider_index = -1,
        .root = root,
        .is_out_of_scope = false,
        .is_built_in = false,
        .func = *func};
};

static ptcl_parser_function ptcl_parser_built_in_create(
    ptcl_func_body *root,
    const ptcl_name *const identifier,
    ptcl_built_in_function_t bind,
    ptcl_argument *arguments,
    size_t count,
    const ptcl_type *const return_type)
{
    PTCL_VALIDATE_COND(identifier != NULL && return_type != NULL);
    return (ptcl_parser_function){
        .identifier = *identifier,
        .provider_index = -1,
        .root = root,
        .is_out_of_scope = false,
        .is_built_in = true,
        .bind = bind,
        .func = (ptcl_statement_func_decl){
            .identifier = *identifier,
            .arguments = arguments,
            .count = count,
            .return_type = *return_type}};
}

static ptcl_parser_typedata ptcl_parser_typedata_create(
    ptcl_func_body *root,
    ptcl_type_typedata *typedata)
{
    return (ptcl_parser_typedata){
        .root = root,
        .identifier = typedata->identifier,
        .typedata = typedata,
        .provider_index = -1,
        .is_out_of_scope = false};
}

static ptcl_parser_typedata ptcl_parser_typedata_create_provider(
    ptcl_func_body *root,
    const ptcl_name *const identifier,
    ptrdiff_t index)
{
    PTCL_VALIDATE_COND(identifier != NULL);
    return (ptcl_parser_typedata){
        .root = root,
        .identifier = *identifier,
        .provider_index = index,
        .is_out_of_scope = false};
}

static ptcl_parser_provider ptcl_parser_provider_create(
    ptcl_statement_type type,
    const ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers,
    const ptcl_name *const identifier,
    ptcl_func_body *root,
    ptcl_argument *arguments,
    size_t arguments_count,
    bool has_variadic,
    size_t index,
    bool is_provider)
{
    PTCL_VALIDATE_COND(identifier != NULL && attributes != NULL);
    return (ptcl_parser_provider){
        .type = type,
        .modifiers = modifiers,
        .attributes = *attributes,
        .identifier = *identifier,
        .root = root,
        .func_decl = ptcl_statement_func_decl_create(ptcl_stat_mods_none_flag, root, identifier, arguments, arguments_count, NULL, ptcl_type_any, has_variadic, false),
        .variants = NULL,
        .variants_capacity = 0,
        .variants_count = 0,
        .generators = NULL,
        .generators_capacity = 0,
        .generators_count = 0,
        .index = index,
        .is_provider = is_provider,
        .is_out_of_scope = false};
}

static ptcl_parser_syntax ptcl_parser_syntax_create(
    const ptcl_name *const identifier,
    ptcl_func_body *root,
    ptcl_parser_syntax_node *nodes,
    size_t count,
    size_t index)
{
    PTCL_VALIDATE_COND(identifier != NULL);
    return (ptcl_parser_syntax){
        .root = root,
        .identifier = *identifier,
        .is_out_of_scope = false,
        .nodes = nodes,
        .count = count,
        .index = index};
}

static ptcl_parser_syntax_node ptcl_parser_syntax_node_create_word(
    ptcl_token_type type,
    const ptcl_name *const identifier)
{
    PTCL_VALIDATE_COND(identifier != NULL);
    return (ptcl_parser_syntax_node){
        .type = ptcl_parser_syntax_node_word_type,
        .word = (ptcl_parser_syntax_word_node){
            .type = type,
            .identifier = *identifier}};
}

static ptcl_parser_syntax_node ptcl_parser_syntax_node_create_variable(
    const ptcl_type *const type,
    char *name)
{
    PTCL_VALIDATE_COND(type != NULL);
    return (ptcl_parser_syntax_node){
        .type = ptcl_parser_syntax_node_variable_type,
        .variable = (ptcl_parser_syntax_word_variable){
            .type = *type,
            .is_variadic = false,
            .name = name}};
}

static ptcl_parser_syntax_node ptcl_parser_syntax_node_create_end_token(
    char *name,
    const ptcl_type *token_type)
{
    return (ptcl_parser_syntax_node){
        .type = ptcl_parser_syntax_node_variable_type,
        .variable = (ptcl_parser_syntax_word_variable){
            .name = name,
            .type = ptcl_type_create_array(token_type, -1, false, true, false),
            .is_variadic = true}};
}

static ptcl_parser_syntax_node ptcl_parser_syntax_node_create_object_type(
    const ptcl_type *const type)
{
    PTCL_VALIDATE_COND(type != NULL);
    return (ptcl_parser_syntax_node){
        .type = ptcl_parser_syntax_node_object_type_type,
        .object_type = *type};
}

static ptcl_parser_syntax_node ptcl_parser_syntax_node_create_value(
    ptcl_expression *value,
    const ptcl_parser_tokens_state *const state)
{
    PTCL_VALIDATE_COND(state != NULL);
    return (ptcl_parser_syntax_node){
        .type = ptcl_parser_syntax_node_value_type,
        .value.value = value,
        .value.state = *state};
}

static ptcl_parser_provider_variant ptcl_parser_provider_variant_create(
    ptcl_expression **values,
    size_t count,
    ptrdiff_t member)
{
    return (ptcl_parser_provider_variant){
        .values = values,
        .count = count,
        .member = member};
}

static bool ptcl_parser_provider_add_variant(
    ptcl_parser_provider *provider,
    const ptcl_parser_provider_variant *const variant)
{
    PTCL_VALIDATE_COND(variant != NULL);
    if (provider->variants_count >= provider->variants_capacity)
    {
        if (provider->variants_count == 0)
        {
            provider->variants_capacity = 1;
        }

        provider->variants_capacity *= 2;
        ptcl_parser_provider_variant *buffer = realloc(provider->variants, provider->variants_capacity * sizeof(ptcl_parser_provider_variant));
        if (buffer == NULL)
        {
            return false;
        }

        provider->variants = buffer;
    }

    provider->variants[provider->variants_count++] = *variant;
    return true;
}

static bool ptcl_parser_provider_add_generator(
    ptcl_parser_provider* provider,
    const ptcl_parser_provider_variant* const generator)
{
    PTCL_VALIDATE_COND(generator != NULL);
    if (provider->generators_count >= provider->generators_capacity)
    {
        if (provider->generators_count == 0)
        {
            provider->generators_capacity = 1;
        }

        provider->generators_capacity *= 2;
        ptcl_parser_provider_variant* buffer = realloc(provider->generators, provider->generators_capacity * sizeof(ptcl_parser_provider_variant));
        if (buffer == NULL)
        {
            return false;
        }

        provider->generators = buffer;
    }

    provider->generators[provider->generators_count++] = *generator;
    return true;
}

static void ptcl_parser_provider_variant_destroy(
    ptcl_parser_provider_variant *const variant)
{
    PTCL_VALIDATE_COND(variant != NULL);
    for (size_t i = 0; i < variant->count; i++)
    {
        ptcl_expression_destroy(variant->values[i]);
    }

    free(variant->values);
}

static void ptcl_parser_provider_destroy(
    ptcl_parser_provider *const provider)
{
    PTCL_VALIDATE_COND(provider != NULL);
    ptcl_name_destroy(&provider->identifier);
    ptcl_attributes_destroy(&provider->attributes);
    // TODO: make func for func decl and this
    for (size_t i = 0; i < provider->func_decl.count; i++)
    {
        ptcl_argument_destroy(&provider->func_decl.arguments[i]);
    }

    free(provider->func_decl.arguments);
    for (size_t i = 0; i < provider->variants_count; i++)
    {
        ptcl_parser_provider_variant_destroy(&provider->variants[i]);
    }

    free(provider->variants);
    for (size_t i = 0; i < provider->generators_count; i++)
    {
        ptcl_parser_provider_variant_destroy(&provider->generators[i]);
    }

    free(provider->generators);
}

static void ptcl_parser_syntax_node_destroy(
    ptcl_parser_syntax_node *const node)
{
    PTCL_VALIDATE_COND(node != NULL);
    switch (node->type)
    {
    case ptcl_parser_syntax_node_variable_type:
        ptcl_type_destroy(&node->variable.type);
        break;
    case ptcl_parser_syntax_node_object_type_type:
        ptcl_type_destroy(&node->object_type);
        break;
    case ptcl_parser_syntax_node_value_type:
        ptcl_expression_destroy(node->value.value);
        break;
    case ptcl_parser_syntax_node_word_type:
        ptcl_name_destroy(&node->word.identifier);
        break;
    }
}

static void ptcl_parser_function_destroy(
    ptcl_parser_function *const function)
{
    PTCL_VALIDATE_COND(function != NULL);
}

static void ptcl_parser_typedata_destroy(
    ptcl_parser_typedata *const typedata)
{
    PTCL_VALIDATE_COND(typedata != NULL);
    free(typedata->typedata);
}

static void ptcl_parser_syntax_destroy(
    ptcl_parser_syntax *const syntax)
{
    PTCL_VALIDATE_COND(syntax != NULL);
    if (syntax->count > 0)
    {
        for (size_t i = 0; i < syntax->count; i++)
        {
            ptcl_parser_syntax_node_destroy(&syntax->nodes[i]);
        }

        free(syntax->nodes);
    }
}

static void ptcl_parser_variable_destroy(
    ptcl_parser_variable *const variable)
{
    PTCL_VALIDATE_COND(variable != NULL);
    if (ptcl_parser_var_flags_is_syntax_word(variable->flags))
    {
        ptcl_name_destroy(&variable->built_in->word);
        free(variable->built_in);
    }
    else if (ptcl_parser_var_flags_is_func_ptr(variable->flags))
    {
        free(variable->type.function_pointer.return_type);
    }
    else
    {
        if (ptcl_parser_var_flags_is_built_in(variable->flags))
        {
            ptcl_expression_destroy(variable->built_in);
        }
    }
}

static void ptcl_parser_tokens_state_destroy(
    ptcl_parser_tokens_state *const state)
{
    PTCL_VALIDATE_COND(state != NULL);
    if (state->is_free)
    {
        free(state->tokens);
    }
}

static void ptcl_parser_comp_type_destroy(
    ptcl_parser_comp_type *const comp_type)
{
    if (ptcl_get_statement_t()->comp_type == comp_type->comp_type || ptcl_get_token_t()->comp_type == comp_type->comp_type)
    {
        return;
    }

    PTCL_VALIDATE_COND(comp_type != NULL);
    free(comp_type->comp_type);
    free(comp_type->static_type);
}

ptcl_parser *ptcl_parser_create(
    ptcl_tokens_list *input,
    ptcl_lexer_configuration *configuration);

ptcl_parser_result ptcl_parser_parse(
    ptcl_parser *parser);

bool ptcl_parser_parse_get_statement(
    ptcl_parser *parser,
    ptcl_parser_statement_info *info);

ptcl_statement *ptcl_parser_parse_statement(
    ptcl_parser *parser);

ptcl_attributes ptcl_parser_parse_attributes(
    ptcl_parser *parser);

bool ptcl_parser_parse_try_syntax_usage_here(
    ptcl_parser *parser,
    bool is_statement);

bool ptcl_parser_parse_try_syntax_usage(
    ptcl_parser *parser,
    ptcl_parser_syntax_node **nodes,
    size_t count,
    int down_start,
    bool skip_first,
    bool is_statement);

void ptcl_parser_leave_from_syntax(
    ptcl_parser *parser);

void ptcl_parser_leave_from_insert_state(
    ptcl_parser *parser);

void ptcl_parser_leave_from_extra_body(
    ptcl_parser* parser);

bool ptcl_parser_func_call(
    ptcl_parser *parser,
    ptcl_parser_function *function,
    ptcl_expression *self,
    bool is_expression,
    ptcl_statement_func_call *const out);

bool ptcl_parser_func_name(
    ptcl_parser *parser,
    ptcl_name *name,
    bool *is_provider,
    bool *is_constructor,
    ptcl_parser_function **prototype,
    ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers);

bool ptcl_parser_func_body_and_storage(
    ptcl_parser *parser,
    ptcl_name *const name,
    ptcl_statement_func_decl *decl,
    ptcl_parser_function *prototype,
    bool is_constructor,
    ptcl_stat_mods modifiers);

bool ptcl_parser_func_decl(
    ptcl_parser *parser,
    bool *is_provider,
    ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers,
    ptcl_statement_func_decl *const out);

bool ptcl_parser_typedata_name(
    ptcl_parser *parser,
    ptcl_name *name,
    bool *is_provider,
    ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers);

bool ptcl_parser_typedata_body_and_storage(
    ptcl_parser *parser,
    ptcl_name *const name,
    ptcl_statement_typedata_decl *decl,
    ptcl_stat_mods modifiers);

bool ptcl_parser_typedata_decl(
    ptcl_parser *parser,
    bool *is_provider,
    ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers,
    ptcl_statement_typedata_decl *const out);

bool ptcl_parser_type_name(
    ptcl_parser *parser,
    ptcl_name *name,
    bool *is_provider,
    ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers);

bool ptcl_parser_type_body_and_storage(
    ptcl_parser *parser,
    ptcl_name *const name,
    ptcl_statement_type_decl *decl,
    ptcl_stat_mods modifiers);

bool ptcl_parser_type_decl(
    ptcl_parser *parser,
    bool *is_provider,
    ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers,
    ptcl_statement_type_decl *const out);

bool ptcl_parser_assign_name(
    ptcl_parser *parser,
    ptcl_expression *left,
    ptcl_name *const name,
    ptcl_parser_assign_dot_data *dot,
    ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers);

bool ptcl_parser_assign_var_and_storage(
    ptcl_parser *parser,
    ptcl_identifier *const identifier,
    ptcl_statement_assign *assign,
    ptcl_stat_mods modifiers,
    bool is_decl_expected,
    ptcl_location location);

bool ptcl_parser_assign(
    ptcl_parser *parser,
    bool *is_provider,
    ptcl_name *const name,
    ptcl_expression *left,
    ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers,
    ptcl_statement_assign *const out);

bool ptcl_parser_return(
    ptcl_parser *parser,
    ptcl_statement_return *const out);

bool ptcl_parser_if(
    ptcl_parser *parser,
    bool is_static,
    ptcl_statement *const out);

bool ptcl_parser_if_expression(
    ptcl_parser *parser,
    bool is_static,
    ptcl_expression **const out);

bool ptcl_parser_unsyntax(
    ptcl_parser *parser,
    ptcl_statement_func_body *const out);

bool ptcl_parser_syntax_decl(
    ptcl_parser *parser);

bool ptcl_parser_loop(
    ptcl_parser *parser,
    ptcl_statement_loop *const out);

bool ptcl_parser_undefine(
    ptcl_parser *parser);

bool ptcl_parser_func_body(
    ptcl_parser *parser,
    bool with_brackets,
    bool change_root,
    bool is_ignore_error,
    ptcl_func_body *const out);

bool ptcl_parser_func_body_by_pointer(
    ptcl_parser *parser,
    ptcl_func_body *func_body_pointer,
    bool with_brackets,
    bool change_root,
    bool is_ignore_error);

bool ptcl_parser_func_body_by_pointer_cap(
    ptcl_parser *parser,
    ptcl_func_body *func_body_pointer,
    size_t *capacity,
    bool with_brackets,
    bool change_root,
    bool is_ignore_error);

ptcl_func_body *ptcl_parser_current_body(
    ptcl_parser *parser);

bool ptcl_parser_extra_body(
    ptcl_parser *parser,
    bool is_syntax);

bool ptcl_parser_type(
    ptcl_parser *parser,
    bool with_word,
    bool with_any,
    bool with_static,
    ptcl_type *const out);

ptcl_expression *ptcl_parser_cast(
    ptcl_parser *parser,
    const ptcl_type * const expected,
    ptcl_parser_expr_flags flags);

ptcl_expression *ptcl_parser_binary(
    ptcl_parser *parser,
    const ptcl_type * const expected,
    ptcl_parser_expr_flags flags);

ptcl_expression *ptcl_parser_additive(
    ptcl_parser *parser,
    const ptcl_type * const expected,
    ptcl_parser_expr_flags flags);

ptcl_expression *ptcl_parser_multiplicative(
    ptcl_parser *parser,
    const ptcl_type * const expected,
    ptcl_parser_expr_flags flags);

ptcl_expression *ptcl_parser_unary(
    ptcl_parser *parser,
    const ptcl_type * const expected,
    ptcl_parser_expr_flags flags);

ptcl_expression *ptcl_parser_dot(
    ptcl_parser *parser,
    const ptcl_type * const expected,
    ptcl_expression *left,
    ptcl_parser_expr_flags flags);

ptcl_expression *ptcl_parser_array_element(
    ptcl_parser *parser,
    const ptcl_type * const expected,
    ptcl_expression *left,
    ptcl_parser_expr_flags flags);

ptcl_expression *ptcl_parser_value(
    ptcl_parser *parser,
    const ptcl_type *const expected,
    ptcl_parser_expr_flags flags);

bool ptcl_parser_try_provider_name(
    ptcl_parser *parser,
    ptcl_name *const name,
    ptcl_statement_type type,
    ptcl_attributes *const attributes,
    ptcl_stat_mods modifiers);

ptcl_name ptcl_parser_name_word(
    ptcl_parser *parser);

ptcl_name ptcl_parser_name_word_or_type(
    ptcl_parser *parser);

ptcl_name ptcl_parser_name(
    ptcl_parser *parser,
    bool with_type);

ptcl_expression *ptcl_parser_get_default(
    ptcl_parser *parser,
    const ptcl_type *const type,
    ptcl_location location);

ptcl_token *ptcl_parser_except(
    ptcl_parser *parser,
    ptcl_token_type token_type);

bool ptcl_parser_not(
    ptcl_parser *parser,
    ptcl_token_type token_type);

bool ptcl_parser_match(
    ptcl_parser *parser,
    ptcl_token_type token_type);

ptcl_token *ptcl_parser_peek(
    ptcl_parser *parser,
    size_t offset);

ptcl_token *ptcl_parser_current(
    ptcl_parser *parser);

ptcl_parser_tokens_state ptcl_parser_get_tokens_state(
    ptcl_parser *parser);

inline void ptcl_parser_set_tokens_state(
    ptcl_parser *parser,
    const ptcl_parser_tokens_state *const state);

ptcl_token *ptcl_parser_tokens(
    ptcl_parser *parser);

void ptcl_parser_set_tokens(
    ptcl_parser *parser,
    ptcl_token *tokens);

size_t ptcl_parser_count(
    ptcl_parser *parser);

void ptcl_parser_set_count(
    ptcl_parser *parser,
    size_t count);

size_t ptcl_parser_position(
    ptcl_parser *parser);

void ptcl_parser_set_position(
    ptcl_parser *parser, size_t
                             position);

void ptcl_parser_skip(
    ptcl_parser *parser);

void ptcl_parser_back(
    ptcl_parser *parser);

bool ptcl_parser_ended(
    ptcl_parser *parser);

bool ptcl_parser_critical(
    ptcl_parser *parser);

bool ptcl_parser_ignore_error(
    ptcl_parser *parser);

bool ptcl_parser_in_type(
    ptcl_parser *parser);

bool ptcl_parser_add_errors(
    ptcl_parser *parser);

bool ptcl_parser_except_return(
    ptcl_parser *parser);

bool ptcl_parser_in_syntax(
    ptcl_parser *parser);

bool ptcl_parser_in_if(
    ptcl_parser *parser);

bool ptcl_parser_in_return(
    ptcl_parser *parser);

bool ptcl_parser_in_dot(
    ptcl_parser *parser);

size_t ptcl_parser_insert_states_count(
    ptcl_parser *parser);

ptcl_parser_insert_state *ptcl_parser_insert_state_at(
    ptcl_parser *parser,
    size_t index);

void ptcl_parser_insert_state_add_and_set(
    ptcl_parser *parser,
    const ptcl_parser_insert_state *const state);

ptcl_func_body *ptcl_parser_root(
    ptcl_parser *parser);

void ptcl_parser_set_root(
    ptcl_parser *parser,
    ptcl_func_body *root);   

size_t ptcl_parser_variables_count(
    ptcl_parser *parser);

ptcl_parser_variable *ptcl_parser_variables(
    ptcl_parser *parser);

void ptcl_parser_set_state(
    ptcl_parser *parser,
    ptcl_parser_flags flag,
    bool value);

void ptcl_parser_enable_state(
    ptcl_parser *parser,
    ptcl_parser_flags flag);

void ptcl_parser_disable_state(
    ptcl_parser *parser,
    ptcl_parser_flags flag);

bool ptcl_parser_has_state(
    ptcl_parser *parser,
    ptcl_parser_flags flag);

bool ptcl_parser_add_this_pair(
    ptcl_parser *parser,
    const ptcl_parser_this_s_pair *const instance);

ptcl_statement *ptcl_parser_insert_pairs(
    ptcl_parser *parser,
    ptcl_statement *statement,
    ptcl_func_body *body);

bool ptcl_parser_add_instance_unique_pointer(
    ptcl_parser* parser,
    ptcl_func_body* instance);

bool ptcl_parser_add_instance_provider(
    ptcl_parser *parser,
    ptcl_parser_provider *const instance);

bool ptcl_parser_add_instance_syntax(
    ptcl_parser *parser,
    ptcl_parser_syntax *const instance);

bool ptcl_parser_add_instance_comp_type(
    ptcl_parser *parser,
    ptcl_parser_comp_type *const instance);

bool ptcl_parser_add_instance_typedata(
    ptcl_parser *parser,
    ptcl_parser_typedata *const instance);

bool ptcl_parser_add_instance_function(
    ptcl_parser *parser,
    ptcl_parser_function *const instance);

bool ptcl_parser_add_instance_variable(
    ptcl_parser *parser,
    ptcl_parser_variable *const instance);

bool ptcl_parser_try_get_syntax(
    ptcl_parser *parser,
    const ptcl_name *const name,
    ptcl_parser_syntax **instance);

bool ptcl_parser_try_get_comp_type(
    ptcl_parser *parser,
    const ptcl_name *const name,
    bool is_static,
    ptcl_parser_comp_type **instance);

bool ptcl_parser_try_get_any_comp_type(
    ptcl_parser *parser,
    const ptcl_name *const name,
    ptcl_parser_comp_type **instance);

bool ptcl_parser_try_get_typedata(
    ptcl_parser *parser,
    const ptcl_name *const name,
    ptcl_parser_typedata **instance);

bool ptcl_parser_try_get_function(
    ptcl_parser *parser,
    const ptcl_name *const name,
    ptcl_parser_function **instance);

bool ptcl_parser_try_get_variable(
    ptcl_parser *parser,
    const ptcl_name *name,
    ptcl_parser_variable **instance);

bool ptcl_parser_try_get_function_by_id(
    ptcl_parser* parser,
    size_t index,
    ptcl_parser_function** function);

size_t ptcl_parser_add_lated_body(
    ptcl_parser *parser,
    size_t start,
    size_t tokens_count,
    bool is_free,
    ptcl_location location);

bool ptcl_parser_is_syntax_defined(
    ptcl_parser *parser,
    const ptcl_name *const name);

bool ptcl_parser_is_comp_type_defined(
    ptcl_parser *parser,
    const ptcl_name *const name, bool is_static);

bool ptcl_parser_is_typedata_defined(
    ptcl_parser *parser,
    const ptcl_name *const name);

bool ptcl_parser_is_function_defined(
    ptcl_parser *parser,
    const ptcl_name *const name);

bool ptcl_parser_is_variable_defined(
    ptcl_parser *parser,
    const ptcl_name *const name);

bool ptcl_parser_is_defined(
    ptcl_parser *parser,
    const ptcl_name *const name);

bool ptcl_parser_check_arguments(
    ptcl_parser *parser,
    ptcl_parser_function *function,
    ptcl_expression **arguments,
    size_t count);

bool ptcl_parser_syntax_try_find(
    ptcl_parser *parser,
    ptcl_parser_syntax_node *nodes,
    size_t count,
    ptcl_parser_syntax **syntax,
    bool *can_continue,
    char **end_token);

bool ptcl_parser_try_get_typedata_member(
    ptcl_parser *parser,
    const ptcl_name *const name,
    const char *member_name,
    ptcl_argument **member,
    size_t *index);

bool ptcl_parser_res_try_get_variable_by_id(
    ptcl_parser_result *result,
    size_t index,
    ptcl_parser_variable **variable);

void ptcl_parser_clear_scope(
    ptcl_parser *parser);

ptcl_expression *ptcl_parser_get_ctor_from_dot(
    ptcl_parser *parser,
    ptcl_expression *dot,
    bool is_root);

void ptcl_parser_throw_out_of_memory(
    ptcl_parser *parser,
    ptcl_location location);

void ptcl_parser_throw_not_allowed_token(
    ptcl_parser *parser,
    char *token,
    ptcl_location location);

void ptcl_parser_throw_except_token(
    ptcl_parser *parser,
    char *value,
    ptcl_location location);

void ptcl_parser_throw_except_type_specifier(
    ptcl_parser *parser,
    ptcl_location location);

void ptcl_parser_throw_incorrect_type(
    ptcl_parser *parser,
    char *excepted,
    char *received,
    ptcl_location location);

void ptcl_parser_throw_fast_incorrect_type(
    ptcl_parser *parser,
    const ptcl_type *const excepted,
    const ptcl_type *const received,
    ptcl_location location);

void ptcl_parser_throw_variable_redefinition(
    ptcl_parser *parser,
    char *name,
    ptcl_location location);

void ptcl_parser_throw_none_type(
    ptcl_parser *parser,
    ptcl_location location);

void ptcl_parser_throw_must_be_variable(
    ptcl_parser *parser,
    ptcl_location location);

void ptcl_parser_throw_must_be_static(
    ptcl_parser *parser,
    ptcl_location location);

void ptcl_parser_throw_unknown_member(
    ptcl_parser *parser,
    const char *name,
    ptcl_location location);

void ptcl_parser_throw_unknown_statement(
    ptcl_parser *parser,
    ptcl_location location);

void ptcl_parser_throw_unknown_expression(
    ptcl_parser *parser,
    ptcl_location location);

void ptcl_parser_throw_unknown_variable(
    ptcl_parser *parser,
    const char *name,
    ptcl_location location);

void ptcl_parser_throw_unknown_syntax(
    ptcl_parser *parser,
    const ptcl_parser_syntax *const syntax,
    ptcl_location location);

void ptcl_parser_throw_wrong_arguments(
    ptcl_parser *parser,
    char *name,
    ptcl_expression **values,
    size_t count,
    ptcl_argument *arguments,
    size_t arguments_count,
    ptcl_location location);

void ptcl_parser_throw_max_depth(
    ptcl_parser *parser,
    ptcl_location location);

void ptcl_parser_throw_unknown_function(
    ptcl_parser *parser,
    const char *name,
    ptcl_location location);

void ptcl_parser_throw_unknown_variable_or_type(
    ptcl_parser *parser,
    char *name,
    ptcl_location location);

void ptcl_parser_throw_unknown_type(
    ptcl_parser *parser,
    char *value,
    ptcl_location location);

void ptcl_parser_throw_redefination(
    ptcl_parser *parser,
    char *name,
    ptcl_location location);

void ptcl_parser_throw_user(
    ptcl_parser *parser,
    char *message,
    ptcl_location location);

void ptcl_parser_add_error(
    ptcl_parser *parser,
    ptcl_parser_error *const error);

void ptcl_parser_result_destroy(
    ptcl_parser_result *const result);

void ptcl_parser_destroy(
    ptcl_parser *parser);

#endif // PTCL_PARSER_H