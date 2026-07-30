#ifndef PTCL_TRANSPILER_H
#define PTCL_TRANSPILER_H

#include <ptcl_parser.h>

#define PTCL_TRANSPILER_SIZE_T_MAX_DIGITS (sizeof(size_t) * CHAR_BIT * 302 / 1000 + 1)
#define PTCL_TRANSPILER_ANONYMOUS_PREFIX "__ptcl_t_anonymous_"
#define PTCL_TRANSPILER_TEMP_PREFIX "__ptcl_t_temp_"

typedef struct ptcl_transpiler ptcl_transpiler;

typedef struct ptcl_transpiler_variable
{
    ptcl_name identifier;
    ptcl_type type;
    bool is_inner;
    bool is_self;
    ptcl_func_body *root;
} ptcl_transpiler_variable;

typedef struct ptcl_transpiler_function
{
    ptcl_name identifier;
    ptcl_func_body *root;
} ptcl_transpiler_function;

typedef struct ptcl_transpiler_anonymous
{
    ptcl_name original_identifier;
    char *alias;
    ptcl_func_body *root;
} ptcl_transpiler_anonymous;

typedef struct ptcl_transpiler_replaced
{
    ptcl_name identifier;
    ptcl_name replaced_identifier;
    ptcl_func_body *root;
    bool cleaned;
} ptcl_transpiler_replaced;

typedef struct ptcl_transpiler_caller
{
    ptcl_func_body *root;
    ptcl_expression *caller;
    size_t start;
    size_t count; 
} ptcl_transpiler_caller;

static ptcl_transpiler_variable ptcl_transpiler_variable_create(
    const ptcl_name *const identifier, 
    const ptcl_type *const type, 
    bool is_inner, 
    ptcl_func_body *root)
{
    PTCL_VALIDATE_COND(identifier != NULL && type != NULL);
    return (ptcl_transpiler_variable){
        .identifier = *identifier,
        .type = *type,
        .is_inner = is_inner,
        .is_self = false,
        .root = root};
}

static ptcl_transpiler_variable ptcl_transpiler_variable_create_this(
    const ptcl_type *const type, 
    ptcl_func_body *root)
{
    PTCL_VALIDATE_COND(type != NULL);
    ptcl_name identifier = ptcl_name_create_fast_w("this", false);
    return (ptcl_transpiler_variable){
        .identifier = identifier,
        .type = *type,
        .is_inner = true,
        .is_self = true,
        .root = root};
}

static ptcl_transpiler_anonymous ptcl_transpiler_anonymous_create(
    const ptcl_name* const original_identifier,
    char *alias, 
    ptcl_func_body *root)
{
    PTCL_VALIDATE_COND(original_identifier != NULL);
    return (ptcl_transpiler_anonymous){
        .original_identifier = *original_identifier,
        .alias = alias,
        .root = root};
}

static ptcl_transpiler_replaced ptcl_transpiler_replaced_name_create(
    const ptcl_name *const identifier, 
    const ptcl_name *const replaced, 
    ptcl_func_body *root)
{
    PTCL_VALIDATE_COND(identifier != NULL && replaced != NULL);
    return (ptcl_transpiler_replaced){
        .identifier = *identifier,
        .replaced_identifier = *replaced,
        .root = root,
        .cleaned = false};
}

static ptcl_transpiler_caller ptcl_transpiler_caller_create(
    ptcl_func_body *root, 
    ptcl_expression *caller, 
    size_t start, 
    size_t count)
{
    return (ptcl_transpiler_caller){
        .root = root,
        .caller = caller,
        .start = start,
        .count = count};
}

static ptcl_transpiler_function ptcl_transpiler_function_create(
    const ptcl_name *const identifier, 
    ptcl_func_body *root)
{
    PTCL_VALIDATE_COND(identifier != NULL);
    return (ptcl_transpiler_function){
        .identifier = *identifier,
        .root = root};
}

ptcl_transpiler *ptcl_transpiler_create(
    ptcl_parser_result result);

char *ptcl_transpiler_transpile(
    ptcl_transpiler *transpiler);

bool ptcl_transpiler_append_word_s(
    ptcl_transpiler *transpiler, 
    char *word);

bool ptcl_transpiler_append_word(
    ptcl_transpiler *transpiler, 
    char *word);

bool ptcl_transpiler_append_character(
    ptcl_transpiler *transpiler, 
    char character);

bool ptcl_transpiler_try_get_variable(
    ptcl_transpiler *transpiler, 
    const ptcl_name *const identifier, 
    ptcl_transpiler_variable **variable);

bool ptcl_transpiler_is_inner(
    ptcl_transpiler *transpiler, 
    const ptcl_name *const identifier);

bool ptcl_transpiler_is_inner_function(
    ptcl_transpiler *transpiler, 
    const ptcl_name *const identifier);

bool ptcl_transpiler_is_caller(
    ptcl_transpiler *transpiler, 
    ptcl_expression *expression, 
    ptcl_transpiler_caller **caller);

bool ptcl_transpiler_add_variable(
    ptcl_transpiler *transpiler,
    const ptcl_transpiler_variable *const variable);

bool ptcl_transpiler_add_variable_f(
    ptcl_transpiler *transpiler,
    const ptcl_name *const name, 
    const ptcl_type *const type,
    bool is_inner, 
    ptcl_func_body *root);

bool ptcl_transpiler_add_inner_func(
    ptcl_transpiler *transpiler, 
    const ptcl_name *const name, 
    ptcl_func_body *root);

bool ptcl_transpiler_add_anonymous(
    ptcl_transpiler *transpiler, 
    const ptcl_name* const original_name,
    char *alias, 
    ptcl_func_body *root);

bool ptcl_transpiler_add_replaced(
    ptcl_transpiler *transpiler,
    const ptcl_name *const name, 
    const ptcl_name *const replaced_name,
    ptcl_func_body *root);

bool ptcl_transpiler_add_caller(
    ptcl_transpiler *transpiler,
    ptcl_func_body *root,
    ptcl_expression *caller,
    size_t start,
    size_t count);

void ptcl_transpiler_add_func_body(
    ptcl_transpiler *transpiler,
    ptcl_statement_func_body *statement,
    const ptcl_func_body *const func_body_ptr,
    bool with_brackets, 
    bool is_func_body);

void ptcl_transpiler_add_statement(
    ptcl_transpiler *transpiler,
    ptcl_statement *statement,
    bool is_func_body);

void ptcl_transpiler_add_func_decl(
    ptcl_transpiler *transpiler,
    const ptcl_statement_func_decl *const func_decl,
    const ptcl_name *const name,
    ptcl_type *self);

bool ptcl_transpiler_add_func_call(
    ptcl_transpiler *transpiler,
    const ptcl_statement_func_call *const func_call);

void ptcl_transpiler_add_expression(
    ptcl_transpiler *transpiler, 
    ptcl_expression *expression,
    bool specify_type);

void ptcl_transpiler_add_identifier(
    ptcl_transpiler* transpiler, 
    const ptcl_identifier* const identifier, 
    bool is_declare);

char *ptcl_transpiler_add_name(
    ptcl_transpiler *transpiler, 
    const ptcl_name *const name, 
    bool is_declare);

void ptcl_transpiler_add_func_type_args(
    ptcl_transpiler *transpiler, 
    const ptcl_type_functon_pointer_type *const type);

char *ptcl_transpiler_add_type_and_name(
    ptcl_transpiler *transpiler, 
    const ptcl_type *const type, 
    const ptcl_name * const name,
    const ptcl_statement_func_decl * const func_decl,
    bool with_array, 
    bool is_define);

void ptcl_transpiler_add_binary_type(
    ptcl_transpiler *transpiler, 
    ptcl_binary_operator_type type);

ptcl_name ptcl_transpiler_add_temp_variable(
    ptcl_transpiler *transpiler, 
    ptcl_expression *expression);

char *ptcl_transpiler_generate_anonymous(
    ptcl_transpiler *transpiler);

char *ptcl_transpiler_generate_temp_and_add(
    ptcl_transpiler *transpiler);

char *ptcl_transpiler_get_func_name_in_type(
    char *type, 
    char *function, 
    bool is_static);

bool ptcl_transpiler_try_get_replaced(
    ptcl_transpiler* transpiler,
    const ptcl_name* const name,
    ptcl_transpiler_replaced* replaced,
    ptcl_func_body* root);

void ptcl_transpiler_clear_scope(
    ptcl_transpiler *transpiler);

void ptcl_transpiler_destroy(
    ptcl_transpiler *transpiler);

#endif // PTCL_TRANSPILER_H
