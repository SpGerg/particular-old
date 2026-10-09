#include <ptcl_interpreter.h>
#include <ptcl_parser.h>
#include <ptcl_parser_builder.h>

#define PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count) \
	for (size_t i = 0; i < count; i++)                  \
	{                                                   \
		ptcl_expression_destroy(arguments[i]);          \
	}                                                   \
	free(arguments);

#define SETUP()                         \
	ptcl_type any_copy = ptcl_type_any; \
	any_copy.is_static = true;          \
	any_copy.is_const = true;

#define CREATE_TOKEN_TYPE()                                                                                                \
	ptcl_comp_type_builder token_builder = ptcl_comp_type_builder_create(ptcl_get_token_t_name());                         \
	ptcl_parser_comp_type parser_token_type = ptcl_comp_type_builder_build(&token_builder, ptcl_get_token_t()->comp_type); \
	ptcl_parser_add_instance_comp_type(parser, &parser_token_type)

#define CREATE_STATEMENT_TYPE()                                                                                                        \
	ptcl_comp_type_builder statement_builder = ptcl_comp_type_builder_create(ptcl_get_statement_t_name());                             \
	ptcl_parser_comp_type parser_statement_type = ptcl_comp_type_builder_build(&statement_builder, ptcl_get_statement_t()->comp_type); \
	ptcl_parser_add_instance_comp_type(parser, &parser_statement_type)

#define CREATE_EXPRESSION_TYPE()                                                                                                 \
	ptcl_comp_type_builder expression_builder = ptcl_comp_type_builder_create(ptcl_get_expression_t_name());                     \
	ptcl_type_comp_type *expression_comp_type = malloc(sizeof(ptcl_type_comp_type));                                             \
	if (expression_comp_type == NULL)                                                                                            \
		goto out_of_memory;                                                                                                      \
	*expression_comp_type = ptcl_comp_type_builder_build_type(&expression_builder);                                              \
	ptcl_type expression_type = ptcl_type_create_comp_type_t(expression_comp_type, true);                                        \
	ptcl_parser_comp_type parser_expression_type = ptcl_comp_type_builder_build(&expression_builder, expression_type.comp_type); \
	ptcl_parser_add_instance_comp_type(parser, &parser_expression_type)

#define CREATE_STATEMENTS_FUNC()                                                                                            \
	ptcl_func_built_in_builder get_statements_builder = ptcl_func_built_in_builder_create(ptcl_get_statements_func_name()); \
	ptcl_type array_statements = ptcl_type_create_array(ptcl_get_statement_t(), -1, true, true, false);                     \
	array_statements.is_const = true;                                                                                       \
	ptcl_func_built_in_builder_bind(&get_statements_builder, ptcl_statements_realization);                                  \
	ptcl_func_built_in_builder_add_argument(&get_statements_builder, &any_copy);                                            \
	ptcl_func_built_in_builder_return(&get_statements_builder, &array_statements);                                          \
	ptcl_parser_function parser_get_stats_type = ptcl_func_built_in_builder_build(&get_statements_builder);                 \
	ptcl_parser_add_instance_function(parser, &parser_get_stats_type)

#define CREATE_ERROR_FUNC()                                                                                   \
	ptcl_func_built_in_builder error_builder = ptcl_func_built_in_builder_create(ptcl_get_error_func_name()); \
	ptcl_type character_copy = ptcl_type_character;                                                           \
	ptcl_type string_type = ptcl_type_create_array(&character_copy, -1, true, true, false);                   \
	string_type.is_static = true;                                                                             \
	ptcl_func_built_in_builder_bind(&error_builder, ptcl_error_realization);                                  \
	ptcl_func_built_in_builder_add_argument(&error_builder, &string_type);                                    \
	ptcl_func_built_in_builder_return(&error_builder, &ptcl_type_void);                                       \
	ptcl_parser_function parser_error_type = ptcl_func_built_in_builder_build(&error_builder);                \
	ptcl_parser_add_instance_function(parser, &parser_error_type)

#define CREATE_DEFINED_FUNC()                                                                                     \
	ptcl_func_built_in_builder defined_builder = ptcl_func_built_in_builder_create(ptcl_get_defined_func_name()); \
	ptcl_func_built_in_builder_bind(&defined_builder, ptcl_defined_realization);                                  \
	ptcl_func_built_in_builder_add_argument(&defined_builder, &any_copy);                                         \
	ptcl_func_built_in_builder_return(&defined_builder, &ptcl_type_integer);                                      \
	ptcl_parser_function parser_defined_type = ptcl_func_built_in_builder_build(&defined_builder);                \
	ptcl_parser_add_instance_function(parser, &parser_defined_type)

#define CREATE_INSERT_FUNC()                                                                                    \
	ptcl_func_built_in_builder insert_builder = ptcl_func_built_in_builder_create(ptcl_get_insert_func_name()); \
	ptcl_func_built_in_builder_bind(&insert_builder, ptcl_insert_realization);                                  \
	ptcl_func_built_in_builder_add_argument(&insert_builder, &any_copy);                                        \
	ptcl_func_built_in_builder_return(&insert_builder, &ptcl_type_integer);                                     \
	ptcl_parser_function parser_insert_type = ptcl_func_built_in_builder_build(&insert_builder);                \
	ptcl_parser_add_instance_function(parser, &parser_insert_type)

#define CLEANUP_BUILDERS()                                       \
	ptcl_func_built_in_builder_destroy(&get_statements_builder); \
	ptcl_func_built_in_builder_destroy(&defined_builder);        \
	ptcl_func_built_in_builder_destroy(&insert_builder);         \
	ptcl_func_built_in_builder_destroy(&error_builder);          \
	ptcl_comp_type_builder_destroy(&token_builder);              \
	ptcl_comp_type_builder_destroy(&statement_builder);          \
	ptcl_comp_type_builder_destroy(&expression_builder)

// TODO: need to make one function for vars declaring and args

typedef struct ptcl_syntax_array
{
	ptcl_parser_syntax *items;
	size_t count;
	size_t capacity;
} ptcl_syntax_array;

typedef struct ptcl_typedata_array
{
	ptcl_parser_typedata *items;
	size_t count;
	size_t capacity;
} ptcl_typedata_array;

typedef struct ptcl_comptype_array
{
	ptcl_parser_comp_type *items;
	size_t count;
	size_t capacity;
} ptcl_comptype_array;

typedef struct ptcl_function_array
{
	ptcl_parser_function *items;
	size_t count;
	size_t capacity;
} ptcl_function_array;

typedef struct ptcl_variable_array
{
	ptcl_parser_variable *items;
	size_t count;
	size_t capacity;
} ptcl_variable_array;

typedef struct ptcl_error_array
{
	ptcl_parser_error *items;
	size_t count;
	size_t capacity;
} ptcl_error_array;

typedef struct ptcl_lated_states_array
{
	ptcl_parser_tokens_state *items;
	size_t count;
	size_t capacity;
} ptcl_lated_states_array;

typedef struct ptcl_providers_array
{
	ptcl_parser_provider *items;
	size_t count;
	size_t capacity;
	size_t generators_count;
} ptcl_providers_array;

typedef struct ptcl_this_pairs_array
{
	ptcl_parser_this_s_pair *items;
	size_t count;
} ptcl_this_pairs_array;

typedef struct ptcl_unique_root_pointers_array
{
	ptcl_func_body **items;
	size_t count;
	size_t capacity;
} ptcl_unique_root_pointers_array;

typedef struct ptcl_extra_body
{
	const ptcl_func_body *root;
} ptcl_extra_body;

typedef struct ptcl_parser_status
{
	ptcl_parser_tokens_state tokens;
	ptcl_parser_syntax_pair syntaxes_nodes[PTCL_PARSER_MAX_DEPTH];
	size_t syntax_depth;
	ptcl_extra_body extra_bodies[PTCL_PARSER_MAX_DEPTH];
	size_t extra_bodies_depth;
	ptcl_parser_flags states;
} ptcl_parser_status;

typedef struct ptcl_parser_temp
{
	ptcl_func_body *root;
	ptcl_func_body *main_root;
	ptcl_func_body *inserted_body;
	const ptcl_type *return_type;
	ptcl_expression *return_value;
	ptcl_parser_insert_state insert_states[PTCL_PARSER_DECL_INSERT_DEPTH];
	size_t insert_states_count;
} ptcl_parser_temp;

typedef struct ptcl_parser
{
	ptcl_interpreter *interpreter;
	ptcl_lexer_configuration *configuration;
	ptcl_tokens_list *input;

	ptcl_parser_status state;
	ptcl_parser_temp temp;

	ptcl_error_array errors;
	ptcl_syntax_array syntaxes;
	ptcl_typedata_array typedatas;
	ptcl_comptype_array comp_types;
	ptcl_function_array functions;
	ptcl_variable_array variables;
	ptcl_lated_states_array lated_states;
	ptcl_this_pairs_array this_pairs;
	ptcl_providers_array providers;
	ptcl_unique_root_pointers_array unique_root_pointers;
} ptcl_parser;

static ptcl_expression_ctor ptcl_parser_ctor_args(
	ptcl_parser *parser,
	ptcl_name *name,
	ptcl_type_typedata *typedata)
{
	if (ptcl_parser_not(parser, ptcl_token_left_par_type))
	{
		return (ptcl_expression_ctor){0};
	}

	ptcl_location location = ptcl_parser_current(parser)->location;
	ptcl_expression_ctor ctor = ptcl_expression_ctor_create(name, NULL, typedata->members, 0);
	ctor.values = malloc(typedata->count * sizeof(ptcl_expression *));
	if (ctor.values == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return (ptcl_expression_ctor){0};
	}

	bool saw_none = false;
	while (!ptcl_parser_match(parser, ptcl_token_right_par_type))
	{
		if (ptcl_parser_ended(parser) || ctor.count > typedata->count)
		{
			ptcl_parser_throw_except_token(
				parser, ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_par_type),
				ptcl_parser_current(parser)->location);
		cleanup:
			ptcl_expression_ctor_destroy(&ctor);
			return (ptcl_expression_ctor){0};
		}

		if (ptcl_parser_current(parser)->type == ptcl_token_exclamation_mark_type &&
			ptcl_parser_peek(parser, 1)->type == ptcl_token_none_type)
		{
			ptcl_location none_loc = ptcl_parser_current(parser)->location;
			ptcl_parser_skip(parser);
			ptcl_parser_skip(parser);

			if (ptcl_parser_not(parser, ptcl_token_right_par_type))
			{
				ptcl_parser_throw_except_token(
					parser,
					ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_par_type),
					ptcl_parser_current(parser)->location);
				goto cleanup;
			}

			saw_none = true;
			for (size_t i = ctor.count; i < typedata->count; i++)
			{
				ptcl_expression *default_value = ptcl_parser_get_default(
					parser, &typedata->members[i].type, ptcl_parser_current(parser)->location);
				if (ptcl_parser_critical(parser))
				{
					ctor.count = i;
					goto cleanup;
				}

				ctor.values[ctor.count++] = default_value;
			}

			break;
		}

		if (ctor.count >= typedata->count)
		{
			ptcl_parser_throw_except_token(
				parser, ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_par_type),
				ptcl_parser_current(parser)->location);
			goto cleanup;
		}

		ptcl_expression *value = ptcl_parser_cast(parser, &typedata->members[ctor.count].type, false);
		if (ptcl_parser_critical(parser))
		{
			goto cleanup;
		}

		ctor.values[ctor.count++] = value;
		if (ctor.count < typedata->count)
		{
			if (ptcl_parser_not(parser, ptcl_token_comma_type))
			{
				ptcl_parser_throw_except_token(
					parser,
					ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_comma_type),
					ptcl_parser_current(parser)->location);
				goto cleanup;
			}
		}
	}

	if (!saw_none)
	{
		if (ctor.count != typedata->count)
		{
			ptcl_parser_throw_except_token(
				parser,
				ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_comma_type),
				location);
			goto cleanup;
		}
	}
	else
	{
		if (ctor.count != typedata->count)
		{
			ptcl_parser_throw_except_token(
				parser,
				ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_par_type),
				location);
			goto cleanup;
		}
	}

	return ctor;
}

static bool ptcl_parser_get_instance(
	ptcl_parser *parser,
	ptcl_statement_type type,
	ptrdiff_t provider_index,
	void **result,
	bool is_static,
	ptcl_location location);

static ptcl_expression *ptcl_parser_try_ctor(
	ptcl_parser *parser,
	ptcl_name *name,
	ptcl_parser_typedata *instance,
	ptcl_location location)
{
	ptcl_type_typedata *typedata = instance->typedata;
	if (!ptcl_parser_get_instance(parser, ptcl_statement_typedata_decl_type, instance->provider_index, &typedata, false, location))
	{
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}
	}

	ptcl_name_destroy(name);
	ptcl_type typedata_type = ptcl_type_create_typedata(typedata, false);
	ptcl_expression *result = ptcl_expression_create(
		ptcl_expression_ctor_type,
		&typedata_type,
		location);
	if (result == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	result->ctor = ptcl_parser_ctor_args(parser, &typedata->identifier, typedata);
	if (ptcl_parser_critical(parser))
	{
		free(result);
		return NULL;
	}

	result->return_type.is_static = true;
	return result;
}

static ptcl_expression *ptcl_parser_try_func(
	ptcl_parser *parser,
	ptcl_name *name,
	ptcl_parser_function *func_instance,
	bool is_expression,
	ptcl_location location)
{
	if (!ptcl_parser_get_instance(parser, ptcl_statement_func_decl_type, func_instance->provider_index, &func_instance, false, location))
	{
		if (ptcl_parser_critical(parser))
		{
			ptcl_name_destroy(name);
			return NULL;
		}
	}

	ptcl_statement_func_call func_call;
	if (!ptcl_parser_func_call(parser, func_instance, NULL, is_expression, &func_call))
	{
		ptcl_name_destroy(name);
		return NULL;
	}

	ptcl_name_destroy(name);
	ptcl_expression *result;
	if (func_call.is_built_in)
	{
		result = func_call.built_in;
	}
	else
	{
		result = ptcl_expression_create(
			ptcl_expression_func_call_type, &func_call.return_type, location);
		if (result == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			ptcl_statement_func_call_destroy(&func_call);
			return NULL;
		}

		result->func_call = func_call;
	}

	return result;
}

static bool ptcl_parser_try_get_left_par(
	ptcl_parser *parser)
{
	if (ptcl_parser_ended(parser))
	{
		if (ptcl_parser_insert_states_count(parser) > 0)
		{
			ptcl_parser_insert_state *state = ptcl_parser_insert_state_at(parser, ptcl_parser_insert_states_count(parser));
			ptcl_parser_tokens_state current = state->state;
			size_t position = current.position;
			if (state->state.tokens == parser->state.tokens.tokens)
			{
				position = parser->state.tokens.position;
			}

			if (state->syntax_depth == parser->state.syntax_depth)
			{
				ptcl_parser_set_tokens_state(parser, &ptcl_parser_insert_state_at(parser, --parser->temp.insert_states_count)->state);
				bool result = ptcl_parser_try_get_left_par(parser);

				ptcl_parser_set_tokens_state(parser, &current);
				ptcl_parser_set_position(parser, position);
				parser->temp.insert_states_count++;
				return result;
			}
			else
			{
				goto syntax;
			}
		}
		else
		{
		syntax:
			if (parser->state.syntax_depth > 0)
			{
				ptcl_parser_tokens_state current = parser->state.tokens;
				ptcl_parser_syntax_pair *syntax = &parser->state.syntaxes_nodes[--parser->state.syntax_depth];
				ptcl_parser_set_tokens_state(parser, &syntax->state);
				bool result = ptcl_parser_try_get_left_par(parser);
				parser->state.syntax_depth++;
				ptcl_parser_set_tokens_state(parser, &current);
				return result;
			}
		}
	}

	return parser->state.tokens.tokens[parser->state.tokens.position].type == ptcl_token_left_par_type;
}

static ptcl_expression *ptcl_parser_var_call_expr(
	ptcl_parser *parser,
	ptcl_name *name,
	ptcl_parser_variable *variable,
	ptcl_location location,
	bool is_expression)
{
	ptcl_type_functon_pointer_type function = variable->type.function_pointer;
	ptcl_statement_func_decl func_decl = ptcl_statement_func_decl_create(
		ptcl_stat_mods_none(),
		ptcl_parser_root(parser),
		&variable->identifier,
		function.arguments,
		function.count,
		NULL,
		*function.return_type,
		function.is_variadic, false);
	ptcl_parser_function instance = ptcl_parser_function_create(parser->temp.root, &func_decl);
	ptcl_statement_func_call func_call;
	if (!ptcl_parser_func_call(parser, &instance, NULL, is_expression, &func_call))
	{
		return NULL;
	}

	ptcl_expression *result = ptcl_expression_create(ptcl_expression_func_call_type, &func_decl.return_type, location);
	if (result == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		ptcl_statement_func_call_destroy(&func_call);
		ptcl_name_destroy(name);
		return NULL;
	}

	result->with_type = false;
	result->func_call = func_call;
	return result;
}

static void ptcl_parser_make_unstatic(
	ptcl_parser_variable *variable)
{
	if (variable->type.comp_type->invariant == NULL)
	{
		return;
	}

	PTCL_FLAGS_REMOVE(variable->flags, false);
	variable->type.comp_type = variable->type.comp_type->invariant;
	variable->type.is_static = false;
}

static ptcl_expression *ptcl_parser_variable_from_transformed(
	ptcl_parser *parser,
	ptcl_parser_variable *variable,
	ptcl_location location)
{
	ptcl_parser_make_unstatic(variable);
	ptcl_expression *result = ptcl_expression_create_variable(&variable->identifier, &variable->type, variable - parser->variables.items, variable->root, location);
	if (result == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	ptcl_name_make_immutable(&result->variable.name);
	return result;
}

static ptcl_expression *ptcl_parser_var_expr(
	ptcl_parser *parser,
	ptcl_name *name,
	ptcl_parser_variable *variable,
	bool is_change_value,
	ptcl_location location)
{
	if (!ptcl_parser_get_instance(parser, ptcl_statement_assign_type, variable->provider_index, &variable, false, location))
	{
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}
	}

	ptcl_expression *result = NULL;
	if ((variable->type.is_static || ptcl_parser_var_flags_is_syntax(variable->flags)) && variable->built_in != NULL)
	{
		if (ptcl_parser_in_return(parser))
		{
			PTCL_FLAGS_SET(variable->flags, PTCL_PARSER_VAR_USED_FLAG);
		}

		ptcl_name_destroy(name);
		// TODO: In fact, we need this transformation after dot parsing (if need don't have dot expr)
		if (ptcl_parser_current(parser)->type != ptcl_token_dot_type && variable->type.type == ptcl_value_type_type && is_change_value)
		{
			result = ptcl_parser_variable_from_transformed(parser, variable, location);
			if (result != NULL)
			{
				return result;
			}
		}

		result = ptcl_expression_copy(variable->built_in, location);
		if (result == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			return NULL;
		}

		if (result->return_type.is_static && result->return_type.type == ptcl_value_word_type)
		{
			ptcl_name_make_immutable(&result->word);
		}

		result->return_type.is_static = variable->built_in->return_type.is_static || !ptcl_parser_var_flags_is_built_in(variable->flags);
	}
	else
	{
		ptcl_name_destroy(name);
		result = ptcl_expression_create_variable(&variable->identifier, &variable->type, variable - parser->variables.items, variable->root, location);
		if (result == NULL)
		{
			ptcl_name_destroy(name);
			ptcl_parser_throw_out_of_memory(parser, location);
			return NULL;
		}

		ptcl_name_make_immutable(&result->variable.name);
	}

	return result;
}

static ptcl_expression *ptcl_parser_func_call_or_var(
	ptcl_parser *parser,
	ptcl_name *name,
	const ptcl_token *const current,
	ptcl_parser_expr_flags flags,
	bool except_static)
{
	PTCL_VALIDATE_COND(current != NULL);
	ptcl_location location = current->location;
	ptcl_parser_typedata *typedata;
	ptcl_parser_variable *variable;
	ptcl_expression *result = NULL;
	const bool with_word = PTCL_FLAGS_HAS(flags, ptcl_parser_expr_with_word_flag);
	if (ptcl_parser_try_get_left_par(parser))
	{
		const size_t start = ptcl_parser_position(parser);
		ptcl_parser_function *func_instance;
		if (ptcl_parser_try_get_function(parser, name, &func_instance))
		{
			// Destroys name
			return ptcl_parser_try_func(
				parser,
				name,
				func_instance,
				ptcl_parser_expr_flags_is_require_expr(flags),
				location);
		}
		else if (ptcl_parser_try_get_typedata(parser, name, &typedata))
		{
			// Destroys name
			return ptcl_parser_try_ctor(parser, name, typedata, location);
		}
		else if (ptcl_parser_try_get_variable(parser, name, &variable))
		{
			// Destroys name
			if ((variable->type.type == ptcl_value_function_pointer_type && !variable->type.is_static && variable->provider_index < 0))
			{
				return ptcl_parser_var_call_expr(
					parser,
					name,
					variable,
					location,
					ptcl_parser_expr_flags_is_require_expr(flags));
			}
			else
			{
				return ptcl_parser_var_expr(
					parser,
					name,
					variable,
					!ptcl_parser_expr_flags_is_ignore_expected(flags) && except_static,
					location);
			}
		}

		if (with_word)
		{
		word_parsing:
			result = ptcl_expression_word_create(name, current->location);
			if (result == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, location);
				return NULL;
			}

			result->return_type.is_static = true;
			return result;
		}

		ptcl_parser_throw_unknown_function(parser, ptcl_name_value(&name), current->location);
		return result;
	}
	else if (ptcl_parser_try_get_variable(parser, name, &variable))
	{
		return ptcl_parser_var_expr(
			parser,
			name,
			variable,
			!ptcl_parser_expr_flags_is_ignore_expected(flags) && except_static,
			location);
	}

	if (with_word)
	{
		goto word_parsing;
	}

	ptcl_parser_throw_unknown_variable(parser, current->value, location);
	return NULL;
}

static void ptcl_statement_func_call_by_ident(
	ptcl_statement *statement,
	ptcl_identifier *identifier)
{
	if (identifier->is_name)
	{
		return;
	}

	ptcl_expression *expression = identifier->value;
	if (expression->type == ptcl_expression_dot_type)
	{
		statement->func_call = ptcl_statement_func_call_create(
			identifier,
			0,
			NULL,
			0);
	}
	else if (expression->type == ptcl_expression_func_call_type)
	{
		statement->func_call = ptcl_statement_func_call_create(
			identifier,
			0,
			NULL,
			0);
	}
}

static bool ptcl_parser_func_call_stat(
	ptcl_parser *parser,
	ptcl_statement **result,
	ptcl_expression *expression,
	ptcl_name *name,
	ptcl_stat_mods modifiers,
	bool *is_null,
	ptcl_location location)
{
	ptcl_statement *statement = *result;
	if (statement == NULL)
	{
		return false;
	}

	*is_null = false;
	const bool with_injection = ptcl_stat_mods_is_flags_injection(modifiers);
	if (expression != NULL)
	{
		ptcl_identifier identifier = ptcl_identifier_create_by_expr(expression);
		ptcl_statement_func_call_by_ident(statement, &identifier);
		return true;
	}

	ptcl_parser_function *function;
	if (ptcl_parser_try_get_function(parser, name, &function))
	{
		ptcl_name_destroy(name);
		ptcl_parser_function placeholder = *function;
		statement->func_call;
		if (!ptcl_parser_func_call(parser, &placeholder, NULL, false, &statement->func_call))
		{
			free(statement);
			return false;
		}

		if (statement->func_call.is_built_in)
		{
			if (statement->func_call.built_in != NULL)
			{
				ptcl_expression_destroy(statement->func_call.built_in);
			}

			free(statement);
			*is_null = true;
		}

		// TODO: fix null
		ptcl_statement *injection = statement;
		if (with_injection)
		{
			injection = ptcl_parser_insert_pairs(parser, statement, function->func.func_body);
			if (ptcl_parser_critical(parser))
			{
				// Destroyed above
				return false;
			}

			*result = injection;
		}

		return true;
	}

	ptcl_parser_variable *variable;
	if (ptcl_parser_try_get_variable(parser, name, &variable) &&
		variable->type.type == ptcl_value_function_pointer_type)
	{
		ptcl_token token = (ptcl_token){0};
		expression = ptcl_parser_func_call_or_var(parser, name, &token, ptcl_parser_expr_none_flag, false);
		if (ptcl_parser_critical(parser))
		{
			free(statement);
			return false;
		}

		ptcl_identifier identifier = ptcl_identifier_create_by_expr(expression);
		ptcl_statement_func_call_by_ident(statement, &identifier);
		return true;
	}

	ptcl_parser_throw_unknown_function(parser, ptcl_name_value(&name), location);
	free(statement);
	ptcl_name_destroy(name);
	return false;
}

static ptcl_token *ptcl_parser_tokens_from_array(
	ptcl_expression *expression)
{
	ptcl_token *expression_tokens = malloc(expression->array.count * sizeof(ptcl_token));
	if (expression_tokens == NULL && expression->array.count > 0)
	{
		return NULL;
	}

	for (size_t i = 0; i < expression->array.count; i++)
	{
		expression_tokens[i] = expression->array.expressions[i]->internal_token;
	}

	return expression_tokens;
}

typedef struct ptcl_parse_arguments_result
{
	ptcl_argument *arguments;
	size_t count;
	bool has_unstatic_member;
	bool has_variadic;
	bool with_self;
	bool is_const;
} ptcl_parse_arguments_result;

static bool ptcl_parser_try_parse_insert(
	ptcl_parser *parser,
	ptcl_parser_tokens_state *state,
	ptcl_location location)
{
	ptcl_token *name = ptcl_parser_current(parser);
	if (!ptcl_parser_is_insert_str(name->value) || ptcl_parser_peek(parser, 1)->type != ptcl_token_left_par_type)
	{
		return false;
	}

	ptcl_parser_skip(parser);
	ptcl_parser_skip(parser);
	ptcl_type array = ptcl_type_create_array(ptcl_get_token_t(), -1, true, true, false);
	ptcl_expression *expression = ptcl_parser_cast(parser, &array, false);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	expression->is_original = false;
	if (ptcl_parser_not(parser, ptcl_token_right_par_type))
	{
		ptcl_expression_destroy(expression);
		return false;
	}

	ptcl_token *expression_tokens = ptcl_parser_tokens_from_array(expression);
	if (ptcl_parser_critical(parser))
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	state->tokens = expression_tokens;
	state->count = expression->array.count;
	state->position = 0;
	state->is_free = true;
	expression->is_original = false;
	ptcl_expression_destroy(expression);
	return true;
}

static void ptcl_parse_arguments_result_destroy(
	const ptcl_parse_arguments_result *const result)
{
	for (size_t i = 0; i < result->count; i++)
	{
		ptcl_argument_destroy(&result->arguments[i]);
	}

	free(result->arguments);
}

static bool ptcl_parser_variadic_arg(
	ptcl_parser *parser,
	ptcl_argument *argument,
	size_t arguments_count,
	bool with_variadic,
	bool is_static,
	ptcl_location location)
{
	if (arguments_count == 0)
	{
		ptcl_parser_throw_except_type_specifier(parser, location);
		return false;
	}
	else if (!with_variadic)
	{
		ptcl_parser_throw_not_allowed_token(parser, "...", ptcl_parser_current(parser)->location);
		return false;
	}

	*argument = ptcl_argument_create_variadic(is_static);
	if (ptcl_parser_not(parser, ptcl_token_right_par_type))
	{
		return false;
	}

	return true;
}

static bool ptcl_parser_name_arg(
	ptcl_parser *parser,
	ptcl_parse_arguments_result *result,
	ptcl_name *argument_name,
	bool with_self,
	bool is_const,
	bool *is_self)
{
	*is_self = false;
	*argument_name = ptcl_parser_name_word(parser);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	// We need manually parse "self" argument without argument type.
	// Make fixes about "self" name unique
	if (result->count == 0 && !result->with_self && with_self)
	{
		result->is_const = is_const;
		result->with_self = true;
		*is_self = true;

		ptcl_parser_match(parser, ptcl_token_comma_type);
		return true;
	}

	if (ptcl_parser_not(parser, ptcl_token_colon_type))
	{
		ptcl_name_destroy(argument_name);
		return false;
	}

	return true;
}

static bool ptcl_parser_type_arg(
	ptcl_parser *parser,
	ptcl_argument *argument,
	ptcl_name *argument_name,
	bool is_const,
	bool with_any)
{
	ptcl_type type;
	if (!ptcl_parser_type(parser, false, with_any, true, &type))
	{
		ptcl_name_destroy(argument_name);
		return false;
	}

	if (!type.is_const)
	{
		type.is_const = is_const;
	}

	*argument = ptcl_argument_create(&type, argument_name);
	return true;
}

typedef enum
{
	ptcl_parser_args_typedata_type,
	ptcl_parser_args_func_type,
	ptcl_parser_args_func_pointer_type
} ptcl_parse_arguments_mode;

static ptcl_parse_arguments_result ptcl_parser_parse_arguments(
	ptcl_parser *parser,
	ptcl_location location,
	bool with_self,
	ptcl_parse_arguments_mode mode)
{
	// resolve mode flags
	bool with_names, with_any, with_variadic, with_default_value;
	switch (mode)
	{
	case ptcl_parser_args_typedata_type:
		with_names = true;
		with_any = true;
		with_variadic = false;
		with_default_value = true;
		break;
	case ptcl_parser_args_func_type:
		with_names = true;
		with_any = true;
		with_variadic = true;
		with_default_value = false;
		break;
	case ptcl_parser_args_func_pointer_type:
		with_names = true;
		with_any = true;
		with_variadic = true;
		with_default_value = false;
		break;
	default:
		with_names = false;
		with_any = false;
		with_variadic = false;
		with_default_value = false;
		break;
	}

	ptcl_parse_arguments_result result = {
		.arguments = NULL,
		.count = 0,
		.has_unstatic_member = false,
		.has_variadic = false,
		.is_const = false,
		.with_self = false};

	const size_t start = ptcl_parser_insert_states_count(parser);
	ptcl_parser_insert_state *start_state = ptcl_parser_insert_state_at(parser, start);
	bool is_parsing_inserted = false;
	size_t capacity = 0;

	while (!ptcl_parser_match(parser, ptcl_token_right_par_type))
	{
		if (ptcl_parser_ended(parser))
		{
			if (is_parsing_inserted)
			{
				ptcl_parser_leave_from_insert_state(parser);
				if (ptcl_parser_insert_states_count(parser) == 0)
				{
					is_parsing_inserted = false;
				}
				continue;
			}

			ptcl_parse_arguments_result_destroy(&result);
			ptcl_parser_throw_except_token(parser, ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_par_type), location);
			break;
		}

		ptcl_parser_tokens_state temp_state;
		const bool is_toggle = ptcl_parser_try_parse_insert(parser, &temp_state, ptcl_parser_current(parser)->location);
		if (ptcl_parser_critical(parser))
		{
		leave:
			ptcl_parse_arguments_result_destroy(&result);
			if (ptcl_parser_insert_states_count(parser) > 1)
			{
				for (size_t i = start + 1; i < ptcl_parser_insert_states_count(parser); i++)
				{
					free(ptcl_parser_insert_state_at(parser, i)->state.tokens);
				}
				free(parser->state.tokens.tokens);
				ptcl_parser_set_tokens_state(parser, &ptcl_parser_insert_state_at(parser, 0)->state);
			}

			break;
		}

		if (ptcl_parser_insert_states_count(parser) >= PTCL_PARSER_DECL_INSERT_DEPTH)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			goto leave;
		}

		if (is_toggle)
		{
			// entering inserted context
			is_parsing_inserted = true;
			start_state->state = parser->state.tokens;
			start_state->syntax_depth = parser->state.syntax_depth;

			ptcl_parser_insert_state *state = ptcl_parser_insert_state_at(parser, ++parser->temp.insert_states_count);
			state->state = temp_state;
			state->syntax_depth = parser->state.syntax_depth;
			ptcl_parser_set_tokens_state(parser, &temp_state);
			continue;
		}

		// allocate argument slot
		if (result.count + 1 > capacity)
		{
			size_t new_capacity = (capacity == 0) ? 3 : capacity * 2;
			ptcl_argument *buffer = realloc(result.arguments, new_capacity * sizeof(ptcl_argument));
			if (buffer == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, location);
				goto leave;
			}

			result.arguments = buffer;
			capacity = new_capacity;
		}

		// TODO: need to remake all this queues
		// TODO: need to care static in arg
		const bool is_static = ptcl_parser_match(parser, ptcl_token_static_type);
		const bool is_const = ptcl_parser_match(parser, ptcl_token_const_type);
		const bool is_variadic = ptcl_parser_match(parser, ptcl_token_elipsis_type);
		if (is_variadic)
		{
			// variadic argument
			if (!result.with_self && with_self)
			{
				ptcl_parser_throw_not_allowed_token(parser, "...", ptcl_parser_current(parser)->location);
				goto leave;
			}

			const size_t current = result.count;
			if (!ptcl_parser_variadic_arg(parser, &result.arguments[current], current, with_variadic, is_static, location))
			{
				goto leave;
			}

			result.has_variadic = true;
			result.count = current + 1;
			break; // variadic must be last
		}
		else
		{
			// normal argument
			const size_t current = result.count;
			ptcl_name argument_name = ptcl_name_empty;
			if (with_names)
			{
				bool is_self = false;
				if (!ptcl_parser_name_arg(parser, &result, &argument_name, with_self, is_const, &is_self))
				{
					goto leave;
				}

				if (is_self)
				{
					// self consumes no type, just skip comma
					ptcl_parser_match(parser, ptcl_token_comma_type);
					continue;
				}
			}

			// parse type specifier
			ptcl_argument *argument = &result.arguments[current];
			if (!ptcl_parser_type_arg(parser, argument, &argument_name, is_const, with_any))
			{
				if (with_names)
				{
					ptcl_name_destroy(&argument_name);
				}

				goto leave;
			}

			if (!result.has_unstatic_member && !argument->type.is_static)
			{
				result.has_unstatic_member = true;
			}

			if (with_default_value && argument->type.is_static)
			{
				ptcl_parser_except(parser, ptcl_token_equals_type);
				if (ptcl_parser_critical(parser))
				{
					ptcl_argument_destroy(argument);
					goto leave;
				}

				ptcl_expression *default_value = ptcl_parser_cast(parser, &argument->type, ptcl_parser_expr_flags_default(true));
				if (ptcl_parser_critical(parser))
				{
					ptcl_argument_destroy(argument);
					goto leave;
				}

				argument->default_value = default_value;
			}

			result.count = current + 1;
		}

		// optional trailing comma
		ptcl_parser_match(parser, ptcl_token_comma_type);
	}

	// error if expected self but nothing parsed
	if (result.count == 0 && !result.with_self && with_self)
	{
		ptcl_parser_throw_except_type_specifier(parser, location);
	}

	return result;
}

static bool ptcl_parser_pointers(
	ptcl_parser *parser,
	ptcl_type *type)
{
	if (type == NULL)
	{
		return false;
	}

	if (ptcl_parser_ended(parser))
	{
		return type;
	}

	if (ptcl_parser_match(parser, ptcl_token_asterisk_type))
	{
		if (type->type == ptcl_value_pointer_type)
		{
			type->pointer.count++;
			return true;
		}

		ptcl_type *target = malloc(sizeof(ptcl_type));
		if (target == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			ptcl_type_destroy(type);
			return false;
		}

		*target = *type;
		*type = ptcl_type_create_pointer(target, false, false, true);
		return ptcl_parser_pointers(parser, type);
	}
	else if (ptcl_parser_match(parser, ptcl_token_left_square_type))
	{
		if (!type->is_static)
		{
			ptcl_type excepted = *type;
			excepted.is_static = true;
			ptcl_parser_throw_fast_incorrect_type(parser, &excepted, type, ptcl_parser_current(parser)->location);
			ptcl_type_destroy(type);
			return false;
		}

		if (ptcl_parser_not(parser, ptcl_token_right_square_type))
		{
			ptcl_type_destroy(type);
			return false;
		}

		if (type->type == ptcl_value_array_type)
		{
			type->array.count++;
			return true;
		}

		ptcl_type *target = malloc(sizeof(ptcl_type));
		if (target == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			ptcl_type_destroy(type);
			return false;
		}

		*target = *type;
		*type = ptcl_type_create_array(target, -1, true, true, true);
		return ptcl_parser_pointers(parser, type);
	}
	else if (ptcl_parser_match(parser, ptcl_token_const_type))
	{
		if (type->type == ptcl_value_pointer_type)
		{
			type->pointer.is_const = true;
		}
		else if (type->type == ptcl_value_array_type)
		{
			type->array.is_const = true;
		}
		else
		{
			ptcl_parser_throw_fast_incorrect_type(parser, &ptcl_type_any_pointer, type, ptcl_parser_current(parser)->location);
			ptcl_type_destroy(type);
			return false;
		}
	}

	return true;
}

// TODO: Only calling in statements
static void ptcl_parser_skip_block_or_expression(
	ptcl_parser *parser)
{
	ptcl_location location = ptcl_parser_current(parser)->location;
	size_t depth = 1;
	while (depth > 0 && !ptcl_parser_ended(parser))
	{
		ptcl_token *current = ptcl_parser_current(parser);
		if (current->type == ptcl_token_left_curly_type)
		{
			depth++;
		}
		else if (current->type == ptcl_token_right_curly_type)
		{
			depth--;
		}

		ptcl_parser_skip(parser);
	}

	if (depth > 0)
	{
		ptcl_parser_throw_except_token(parser, ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_curly_type), location);
	}
}

#pragma warning(push)
#pragma warning(disable : 4090)

static void ptcl_parser_set_arrays_length(
	ptcl_type *type,
	ptcl_type *target)
{
	if (type->type == ptcl_value_array_type)
	{
		type->array.count = target->array.count;
		ptcl_parser_set_arrays_length(type->array.target, target->array.target);
	}
}

#pragma warning(pop)

static ptcl_statement *ptcl_create_statement_from_expression(
	ptcl_func_body *root,
	ptcl_expression *expression,
	ptcl_location location)
{
	ptcl_attributes attributes = ptcl_attributes_create(NULL, 0, true);
	ptcl_statement *statement = ptcl_statement_create(
		expression->internal_statement->type,
		expression->internal_statement->root,
		&attributes,
		location);
	if (statement == NULL)
	{
		return NULL;
	}

	*statement = *expression->internal_statement;
	statement->root = root;
	return statement;
}

static ptcl_expression *ptcl_create_expression_from_statement(
	ptcl_parser *parser,
	ptcl_statement *statement,
	ptcl_location location)
{
	ptcl_expression *expression = ptcl_expression_create(
		ptcl_expression_in_statement_type,
		ptcl_get_statement_t(),
		location);
	if (expression == NULL)
	{
		return NULL;
	}

	if (statement->is_original)
	{
		ptcl_statement *copy = malloc(sizeof(ptcl_statement));
		if (copy == NULL)
		{
			free(expression);
			return NULL;
		}

		*copy = *statement;
		statement->is_original = false;
		statement = copy;
		statement->is_original = true;
	}

	expression->internal_statement = statement;
	return expression;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static ptcl_expression *ptcl_error_realization(
	ptcl_parser *parser,
	ptcl_expression **arguments,
	size_t count,
	ptcl_location location,
	bool is_expression)
{
	if (ptcl_parser_ignore_error(parser))
	{
		PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
		return NULL;
	}

	ptcl_expression *content = arguments[0];
	char *message = ptcl_string_from_array(&arguments[0]->array);
	PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
	if (message == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	ptcl_parser_throw_user(parser, message, location);
	return NULL;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static ptcl_expression *ptcl_defined_realization(
	ptcl_parser *parser,
	ptcl_expression **arguments,
	size_t count,
	ptcl_location location,
	bool is_expression)
{
	ptcl_expression *name_argument = arguments[0];
	ptcl_name name;
	if (ptcl_type_is_string(&name_argument->return_type))
	{
		name = ptcl_name_create_owned(ptcl_string_from_array(&name_argument->array), false);
		if (ptcl_name_value(&name) == NULL)
		{
			PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
			ptcl_parser_throw_out_of_memory(parser, location);
			return NULL;
		}
	}
	else if (name_argument->return_type.type == ptcl_value_word_type)
	{
		name = ptcl_name_create_borrowed(ptcl_name_value(&name_argument->word), false);
	}
	else
	{
		ptcl_parser_throw_fast_incorrect_type(parser, &ptcl_type_word, &name_argument->return_type, location);
		PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
		return NULL;
	}

	bool is_defined = ptcl_parser_is_defined(parser, &name);
	ptcl_name_destroy(&name);
	PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
	return ptcl_expression_create_integer(is_defined, location);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static ptcl_expression *ptcl_statements_realization(ptcl_parser *parser, ptcl_expression **arguments, size_t count, ptcl_location location, bool is_expression)
{
	ptcl_expression *argument = arguments[0];
	ptcl_type *type = &argument->return_type;
	ptcl_type array_type = ptcl_type_create_array(ptcl_get_statement_t(), -1, true, true, false);
	switch (type->type)
	{
	case ptcl_value_function_pointer_type:
	{
		ptcl_parser_temp *temp = &parser->temp;
		ptcl_parser_tokens_state *lated_body = &parser->lated_states.items[argument->lated_body.index];
		const ptcl_parser_tokens_state last_tokens_state = parser->state.tokens;
		ptcl_parser_set_tokens_state(parser, lated_body);
		ptcl_parser_set_position(parser, 0);

		ptcl_func_body *last_state = temp->inserted_body;
		ptcl_func_body *unique_body_pointer = malloc(sizeof(ptcl_func_body));
		if (unique_body_pointer == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
			return false;
		}

		ptcl_parser_add_instance_unique_pointer(parser, unique_body_pointer);
		*unique_body_pointer = ptcl_func_body_create(
			NULL, 0,
			ptcl_parser_root(parser));
		temp->inserted_body = unique_body_pointer;

		ptcl_parser_func_body_by_pointer(parser, unique_body_pointer, true, false, ptcl_parser_ignore_error(parser));
		ptcl_parser_set_tokens_state(parser, &last_tokens_state);
		temp->inserted_body = last_state;

		if (ptcl_parser_critical(parser))
		{
			PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
			return NULL;
		}

		ptcl_expression **statements = malloc(unique_body_pointer->count * sizeof(ptcl_expression *));
		if (statements == NULL && unique_body_pointer->count > 0)
		{
			ptcl_func_body_destroy(unique_body_pointer);
			PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
			return NULL;
		}

		ptcl_expression *array = ptcl_expression_create(ptcl_expression_array_type, &array_type, location);
		if (array == NULL)
		{
			free(statements);
			ptcl_func_body_destroy(unique_body_pointer);
			PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
			return NULL;
		}

		for (size_t i = 0; i < unique_body_pointer->count; i++)
		{
			ptcl_expression *element = ptcl_create_expression_from_statement(parser, unique_body_pointer->statements[i], location);
			if (element == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, location);
				ptcl_func_body_destroy(unique_body_pointer);
				for (size_t j = 0; j < i; j++)
				{
					free(statements[j]);
				}

				free(statements);
				PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
				return NULL;
			}

			statements[i] = element;
		}

		PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
		array->array = ptcl_expression_array_create_member(ptcl_get_statement_t(), statements, unique_body_pointer->count);
		ptcl_func_body_destroy(unique_body_pointer);
		return array;
	}
	default:
		ptcl_parser_throw_fast_incorrect_type(
			parser,
			&array_type,
			&argument->return_type, location);
		PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
		return NULL;
	}
}

static bool ptcl_check_statements(ptcl_parser *parser, ptcl_func_body func_body)
{
	for (size_t i = 0; i < func_body.count; i++)
	{
		ptcl_statement *statement = func_body.statements[i];
		if (statement->type == ptcl_statement_if_type)
		{
			if (!ptcl_check_statements(parser, statement->if_stat.body))
			{
				return false;
			}

			if (statement->if_stat.with_else && !ptcl_check_statements(parser, statement->if_stat.else_body))
			{
				return false;
			}
		}
		else if (statement->type == ptcl_statement_return_type)
		{
			ptcl_type type = statement->ret.value == NULL ? ptcl_type_void : statement->ret.value->return_type;
			if (!ptcl_type_equals(parser->temp.return_type, &type))
			{
				ptcl_parser_throw_fast_incorrect_type(parser, parser->temp.return_type, &type, statement->location);
				return false;
			}
		}
	}

	return true;
}

static bool ptcl_check_array_statements(
	ptcl_parser *parser,
	const ptcl_expression_array *const array)
{
	for (size_t i = 0; i < array->count; i++)
	{
		ptcl_expression *expression = array->expressions[i];
		if (expression->type != ptcl_expression_in_statement_type)
		{
			continue;
		}

		ptcl_statement *statement = expression->internal_statement;
		if (statement->type == ptcl_statement_if_type)
		{
			if (!ptcl_check_statements(parser, statement->if_stat.body))
			{
				return false;
			}

			if (statement->if_stat.with_else && !ptcl_check_statements(parser, statement->if_stat.else_body))
			{
				return false;
			}
		}
		else if (statement->type == ptcl_statement_func_body_type)
		{
			if (!ptcl_check_statements(parser, statement->body.body))
			{
				return false;
			}
		}
		else if (statement->type == ptcl_statement_return_type)
		{
			ptcl_type *type = statement->ret.value == NULL ? &ptcl_type_void : &statement->ret.value->return_type;
			if (!ptcl_type_equals(parser->temp.return_type, type))
			{
				ptcl_parser_throw_fast_incorrect_type(parser, parser->temp.return_type, type, statement->location);
				return false;
			}
		}
	}

	return true;
}

static ptcl_expression *ptcl_insert_statements_array(
	ptcl_parser *parser,
	ptcl_expression *argument,
	ptcl_location location)
{
	ptcl_expression_array array = argument->array;
	if (!ptcl_check_array_statements(parser, &array))
	{
		ptcl_expression_destroy(argument);
		return NULL;
	}

	ptcl_func_body *body = ptcl_parser_current_body(parser);
	const size_t buffer_count = body->count + array.count;
	ptcl_statement **buffer = realloc(body->statements, buffer_count * sizeof(ptcl_statement *));
	if (buffer == NULL && buffer_count > 0)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		ptcl_expression_destroy(argument);
		return NULL;
	}

	body->statements = buffer;
	for (size_t i = 0; i < array.count; i++)
	{
		ptcl_expression *expression = array.expressions[i];
		ptcl_statement *element = ptcl_create_statement_from_expression(
			ptcl_parser_root(parser),
			expression,
			location);
		if (element == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			for (size_t j = i; j < array.count; j++)
			{
				ptcl_expression_destroy(array.expressions[j]);
			}

			free(array.expressions);
			return NULL;
		}

		if (expression->is_original)
		{
			expression->is_original = false;
			free(expression->internal_statement);
			free(expression);

			element->is_original = true;
		}

		body->statements[body->count] = element;
		body->count++;
	}

	free(argument);
	free(array.expressions);
	return NULL;
}

static ptcl_expression *ptcl_insert_realization(ptcl_parser *parser, ptcl_expression **arguments, size_t count, ptcl_location location, bool is_expression);
static ptcl_expression *ptcl_insert_tokens_array(ptcl_parser *parser, ptcl_expression **arguments, size_t count, ptcl_expression *argument, bool is_expression, ptcl_location location)
{
	if (is_expression)
	{
		ptcl_token *expression_tokens = ptcl_parser_tokens_from_array(argument);
		if (ptcl_parser_critical(parser))
		{
			PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
			ptcl_parser_throw_out_of_memory(parser, location);
			return NULL;
		}

		const ptcl_parser_tokens_state last_state = parser->state.tokens;
		ptcl_parser_set_position(parser, 0);
		ptcl_parser_set_tokens(parser, expression_tokens);
		ptcl_parser_set_count(parser, argument->array.count);

		ptcl_expression *value = ptcl_parser_cast(parser, NULL, false);

		ptcl_parser_set_tokens_state(parser, &last_state);

		PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
		free(expression_tokens);
		if (ptcl_parser_critical(parser))
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			return NULL;
		}

		return value;
	}

	ptcl_expression *statements = ptcl_statements_realization(parser, arguments, 1, location, true);
	if (ptcl_parser_critical(parser))
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	return ptcl_insert_realization(parser, &statements, 1, location, false);
}

static ptcl_expression *ptcl_insert_realization(
	ptcl_parser *parser,
	ptcl_expression **arguments,
	size_t count,
	ptcl_location location,
	bool is_expression)
{
	ptcl_expression *argument = arguments[0];
	ptcl_type type = argument->return_type;
	ptcl_type array_type = ptcl_type_create_array(ptcl_get_statement_t(), -1, true, true, false);
	if (is_expression)
	{
		if (type.type == ptcl_value_array_type)
		{
			const ptcl_type *target = type.array.target;
			if (target->type == ptcl_value_type_type && ptcl_parser_is_token_t_name(&target->comp_type->identifier))
			{
				goto tokens;
			}
		}

		PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
		ptcl_parser_throw_unknown_expression(parser, location);
		return NULL;
	}

	switch (type.type)
	{
	case ptcl_value_array_type:
	{
		const ptcl_type *target = type.array.target;
		if (target->type != ptcl_value_type_type)
		{
			PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
			ptcl_parser_throw_fast_incorrect_type(parser, &array_type, &type, location);
			return NULL;
		}

		ptcl_name *identifier = &target->comp_type->identifier;
		if (ptcl_parser_is_statement_t_name(identifier))
		{
			free(arguments);
			return ptcl_insert_statements_array(parser, argument, location);
		}
		else if (ptcl_parser_is_statement_t_name(identifier))
		{
		tokens:
			return ptcl_insert_tokens_array(parser, arguments, count, argument, is_expression, location);
		}
	}
	default:
		break;
	}

	ptcl_parser_throw_fast_incorrect_type(
		parser,
		&array_type,
		&argument->return_type, location);
	PTCL_PARSER_DESTROY_ARGUMENTS(arguments, count);
	return NULL;
}

static ptcl_name internal_token_t_base;
static ptcl_name internal_token_t_name;
static bool internal_token_t_name_initialized = 0;

const ptcl_name *ptcl_get_token_t_name(void)
{
	if (!internal_token_t_name_initialized)
	{
		internal_token_t_base = ptcl_name_create_borrowed(PTCL_PARSER_TOKEN_TYPE_NAME, false);
		internal_token_t_name = ptcl_name_create_borrowed_with_next(PTCL_PARSER_LANGUAGE_NAMESPACE_NAME, &internal_token_t_base, false);
		internal_token_t_name_initialized = 1;
	}

	return &internal_token_t_name;
}

static ptcl_name internal_statement_t_base;
static ptcl_name internal_statement_t_name;
static bool internal_statement_t_name_initialized = 0;

const ptcl_name *ptcl_get_statement_t_name(void)
{
	if (!internal_statement_t_name_initialized)
	{
		internal_statement_t_base = ptcl_name_create_borrowed(PTCL_PARSER_STATEMENT_TYPE_NAME, false);
		internal_statement_t_name = ptcl_name_create_borrowed_with_next(PTCL_PARSER_LANGUAGE_NAMESPACE_NAME, &internal_statement_t_base, false);
		internal_statement_t_name_initialized = 1;
	}

	return &internal_statement_t_name;
}

static ptcl_name internal_expression_t_base;
static ptcl_name internal_expression_t_name;
static bool internal_expression_t_name_initialized = 0;

const ptcl_name *ptcl_get_expression_t_name(void)
{
	if (!internal_expression_t_name_initialized)
	{
		internal_expression_t_base = ptcl_name_create_borrowed(PTCL_PARSER_EXPRESSION_TYPE_NAME, false);
		internal_expression_t_name = ptcl_name_create_borrowed_with_next(PTCL_PARSER_LANGUAGE_NAMESPACE_NAME, &internal_expression_t_base, false);
		internal_expression_t_name_initialized = 1;
	}

	return &internal_expression_t_name;
}

static ptcl_name internal_statements_func_base;
static ptcl_name internal_statements_func_name;
static bool internal_statements_func_name_initialized = 0;

const ptcl_name *ptcl_get_statements_func_name(void)
{
	if (!internal_statements_func_name_initialized)
	{
		internal_statements_func_base = ptcl_name_create_borrowed(PTCL_PARSER_STATEMENTS_FUNC_NAME, false);
		internal_statements_func_name = ptcl_name_create_borrowed_with_next(PTCL_PARSER_LANGUAGE_NAMESPACE_NAME, &internal_statements_func_base, false);
		internal_statements_func_name_initialized = 1;
	}

	return &internal_statements_func_name;
}

static ptcl_name internal_defined_func_base;
static ptcl_name internal_defined_func_name;
static bool internal_defined_func_name_initialized = 0;

const ptcl_name *ptcl_get_defined_func_name(void)
{
	if (!internal_defined_func_name_initialized)
	{
		internal_defined_func_base = ptcl_name_create_borrowed(PTCL_PARSER_DEFINED_FUNC_NAME, false);
		internal_defined_func_name = ptcl_name_create_borrowed_with_next(PTCL_PARSER_LANGUAGE_NAMESPACE_NAME, &internal_defined_func_base, false);
		internal_defined_func_name_initialized = 1;
	}

	return &internal_defined_func_name;
}

static ptcl_name internal_error_func_base;
static ptcl_name internal_error_func_name;
static bool internal_error_func_name_initialized = 0;

const ptcl_name *ptcl_get_error_func_name(void)
{
	if (!internal_error_func_name_initialized)
	{
		internal_error_func_base = ptcl_name_create_borrowed(PTCL_PARSER_ERROR_FUNC_NAME, false);
		internal_error_func_name = ptcl_name_create_borrowed_with_next(PTCL_PARSER_LANGUAGE_NAMESPACE_NAME, &internal_error_func_base, false);
		internal_error_func_name_initialized = 1;
	}

	return &internal_error_func_name;
}

static ptcl_name internal_insert_func_base;
static ptcl_name internal_insert_func_name;
static bool internal_insert_func_name_initialized = 0;

const ptcl_name *ptcl_get_insert_func_name(void)
{
	if (!internal_insert_func_name_initialized)
	{
		internal_insert_func_base = ptcl_name_create_borrowed(PTCL_PARSER_INSERT_FUNC_NAME, false);
		internal_insert_func_name = ptcl_name_create_borrowed_with_next(PTCL_PARSER_LANGUAGE_NAMESPACE_NAME, &internal_insert_func_base, false);
		internal_insert_func_name_initialized = 1;
	}

	return &internal_insert_func_name;
}

static ptcl_type internal_statement_t;
static ptcl_type_comp_type internal_statement_comp_type;
static bool internal_statement_t_initialized = 0;

const ptcl_type *ptcl_get_statement_t(void)
{
	if (!internal_statement_t_initialized)
	{
		internal_statement_comp_type = ptcl_type_create_comp_type_empty(ptcl_get_statement_t_name());
		internal_statement_t = ptcl_type_create_comp_type_t(&internal_statement_comp_type, false);
		internal_statement_t_initialized = 1;
	}

	return &internal_statement_t;
}

static ptcl_type internal_token_t;
static ptcl_type_comp_type internal_token_comp_type;
static bool internal_token_t_initialized = 0;

const ptcl_type *ptcl_get_token_t(void)
{
	if (!internal_token_t_initialized)
	{
		internal_token_comp_type = ptcl_type_create_comp_type_empty(ptcl_get_token_t_name());
		internal_token_t = ptcl_type_create_comp_type_t(&internal_token_comp_type, false);
		internal_token_t_initialized = 1;
	}

	return &internal_token_t;
}

ptcl_parser *ptcl_parser_create(
	ptcl_tokens_list *input,
	ptcl_lexer_configuration *configuration)
{
	ptcl_parser *parser = malloc(sizeof(ptcl_parser));
	if (parser == NULL)
	{
		return NULL;
	}

	parser->configuration = configuration;
	parser->input = input;
	return parser;
}

static void ptcl_parser_reset(
	ptcl_parser *parser)
{
	parser->state = (ptcl_parser_status){0};
	parser->temp = (ptcl_parser_temp){0};
	parser->errors = (ptcl_error_array){0};
	parser->syntaxes = (ptcl_syntax_array){0};
	parser->typedatas = (ptcl_typedata_array){0};
	parser->comp_types = (ptcl_comptype_array){0};
	parser->functions = (ptcl_function_array){0};
	parser->variables = (ptcl_variable_array){0};
	parser->lated_states = (ptcl_lated_states_array){0};
	parser->this_pairs = (ptcl_this_pairs_array){0};
	parser->providers = (ptcl_providers_array){0};
	parser->unique_root_pointers = (ptcl_unique_root_pointers_array){0};

	parser->syntaxes.capacity = PTCL_PARSER_DEFAULT_INSTANCE_CAPACITY;
	parser->comp_types.capacity = PTCL_PARSER_DEFAULT_INSTANCE_CAPACITY;
	parser->typedatas.capacity = PTCL_PARSER_DEFAULT_INSTANCE_CAPACITY;
	parser->functions.capacity = PTCL_PARSER_DEFAULT_INSTANCE_CAPACITY;
	parser->variables.capacity = PTCL_PARSER_DEFAULT_INSTANCE_CAPACITY;
	parser->lated_states.capacity = PTCL_PARSER_DEFAULT_INSTANCE_CAPACITY;
	// We don't allocate memory for 'this_pairs' because this feature is not very popular rn
	parser->providers.capacity = PTCL_PARSER_DEFAULT_INSTANCE_CAPACITY;
	parser->unique_root_pointers.capacity = PTCL_PARSER_DEFAULT_INSTANCE_CAPACITY;
	parser->interpreter = ptcl_interpreter_create(parser);
	if (parser->interpreter == NULL)
	{
		goto cleanup;
	}

	parser->syntaxes.items = malloc(parser->syntaxes.capacity * sizeof(ptcl_parser_syntax));
	if (parser->syntaxes.items == NULL)
	{
		goto cleanup;
	}

	parser->comp_types.items = malloc(parser->comp_types.capacity * sizeof(ptcl_parser_comp_type));
	if (parser->comp_types.items == NULL)
	{
		goto cleanup;
	}

	parser->typedatas.items = malloc(parser->typedatas.capacity * sizeof(ptcl_parser_typedata));
	if (parser->typedatas.items == NULL)
		goto cleanup;

	parser->functions.items = malloc(parser->functions.capacity * sizeof(ptcl_parser_function));
	if (parser->functions.items == NULL)
	{
		goto cleanup;
	}

	parser->variables.items = malloc(parser->variables.capacity * sizeof(ptcl_parser_variable));
	if (parser->variables.items == NULL)
	{
		goto cleanup;
	}

	parser->lated_states.items = malloc(parser->lated_states.capacity * sizeof(ptcl_parser_tokens_state));
	if (parser->lated_states.items == NULL)
	{
		goto cleanup;
	}

	parser->providers.items = malloc(parser->providers.capacity * sizeof(ptcl_parser_provider));
	if (parser->providers.items == NULL)
	{
		goto cleanup;
	}

	parser->unique_root_pointers.items = malloc(parser->unique_root_pointers.capacity * sizeof(ptcl_parser_provider));
	if (parser->unique_root_pointers.items == NULL)
	{
		goto cleanup;
	}

	ptcl_parser_enable_state(parser, PTCL_PARSER_ADD_ERRORS_FLAG);
	ptcl_parser_enable_state(parser, PTCL_PARSER_IN_SYNTAX_FLAG);
	return;
cleanup:
	ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
	if (parser->interpreter != NULL)
	{
		ptcl_interpreter_destroy(parser->interpreter);
	}

	free(parser->syntaxes.items);
	free(parser->comp_types.items);
	free(parser->typedatas.items);
	free(parser->functions.items);
	free(parser->variables.items);
	free(parser->lated_states.items);
	free(parser->providers.items);
}

ptcl_parser_result ptcl_parser_parse(
	ptcl_parser *parser)
{
	ptcl_parser_reset(parser);
	if (ptcl_parser_critical(parser))
	{
		goto out_of_memory;
	}

	SETUP();
	CREATE_TOKEN_TYPE();
	CREATE_STATEMENT_TYPE();
	CREATE_EXPRESSION_TYPE();
	CREATE_STATEMENTS_FUNC();
	CREATE_DEFINED_FUNC();
	CREATE_ERROR_FUNC();
	CREATE_INSERT_FUNC();

	ptcl_parser_set_tokens(parser, parser->input->tokens);
	ptcl_parser_set_count(parser, parser->input->count);
	ptcl_func_body func_body;
	ptcl_parser_func_body(
		parser, false, true, ptcl_parser_ignore_error(parser), &func_body);
	ptcl_parser_result result = {
		.configuration = parser->configuration,
		.body = func_body,
		.errors = parser->errors.items,
		.errors_count = parser->errors.count,
		.syntaxes = parser->syntaxes.items,
		.syntaxes_count = parser->syntaxes.count,
		.comp_types = parser->comp_types.items,
		.comp_types_count = parser->comp_types.count,
		.typedatas = parser->typedatas.items,
		.typedatas_count = parser->typedatas.count,
		.functions = parser->functions.items,
		.functions_count = parser->functions.count,
		.variables = parser->variables.items,
		.variables_count = parser->variables.count,
		.lated_states = parser->lated_states.items,
		.lated_states_count = parser->lated_states.count,
		.this_pairs = parser->this_pairs.items,
		.this_pairs_count = parser->this_pairs.count,
		.providers = parser->providers.items,
		.providers_count = parser->providers.count,
		.unique_pointers = parser->unique_root_pointers.items,
		.unique_pointers_count = parser->unique_root_pointers.count,
		.is_critical = ptcl_parser_critical(parser)};

	goto success;

out_of_memory:
	ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
	result = (ptcl_parser_result){
		.body = ptcl_func_body_create(NULL, 0, NULL),
		.errors_count = 1,
		.errors = parser->errors.items,
		.syntaxes = parser->syntaxes.items,
		.syntaxes_count = parser->syntaxes.count,
		.comp_types = parser->comp_types.items,
		.comp_types_count = parser->comp_types.count,
		.typedatas = parser->typedatas.items,
		.typedatas_count = parser->typedatas.count,
		.functions = parser->functions.items,
		.functions_count = parser->functions.count,
		.variables = parser->variables.items,
		.variables_count = parser->variables.count,
		.lated_states = NULL,
		.lated_states_count = 0,
		.is_critical = ptcl_parser_critical(parser)};

success:
	CLEANUP_BUILDERS();
	return result;
}

bool ptcl_parser_parse_get_statement(
	ptcl_parser *parser,
	ptcl_parser_statement_info *info)
{
	for (size_t i = 0; i < PTCL_PARSER_MAX_MODIFIERS_RECURSION; i++)
	{
		bool is_modifier = true;
		ptcl_token *current = ptcl_parser_current(parser);
		switch (current->type)
		{
		case ptcl_token_prototype_type:
			PTCL_FLAGS_SET(info->modifiers, ptcl_stat_mods_prototype_flag);
			break;
		case ptcl_token_const_type:
			PTCL_FLAGS_SET(info->modifiers, ptcl_stat_mods_const_flag);
			break;
		case ptcl_token_global_type:
			PTCL_FLAGS_SET(info->modifiers, ptcl_stat_mods_global_flag);
			break;
		case ptcl_token_static_type:
			PTCL_FLAGS_SET(info->modifiers, ptcl_stat_mods_static_flag);
			break;
		case ptcl_token_auto_type:
			PTCL_FLAGS_SET(info->modifiers, ptcl_stat_mods_auto_flag);
			break;
		case ptcl_token_caret_type:
			PTCL_FLAGS_SET(info->modifiers, ptcl_stat_mods_injection_flag);
			break;
		default:
			is_modifier = false;
			break;
		}

		if (!is_modifier)
		{
			break;
		}

		ptcl_parser_skip(parser);
	}

	ptcl_token *current = ptcl_parser_current(parser);
	ptcl_statement_type *type = &info->type;
	switch (current->type)
	{
	case ptcl_token_tilde_type:
	case ptcl_token_word_word_type:
	case ptcl_token_exclamation_mark_type:
	case ptcl_token_word_type:
	{
		ptcl_token *current = ptcl_parser_current(parser);
		info->name = ptcl_parser_name(parser, false);
		if (ptcl_parser_critical(parser))
		{
			return false;
		}

		ptcl_token *next = ptcl_parser_current(parser);
		switch (next->type)
		{
		case ptcl_token_provider_type:
		case ptcl_token_colon_type:
		case ptcl_token_equals_type:
			*type = ptcl_statement_assign_type;
			break;
		case ptcl_token_dot_type:
		{
			ptcl_expression *left = ptcl_parser_func_call_or_var(parser, &info->name, current, ptcl_parser_expr_ignore_excepted_flag, false);
			if (left == NULL)
			{
				break;
			}

			info->expression = ptcl_parser_dot(parser, NULL, left, ptcl_parser_expr_flags_default(false));
			if (ptcl_parser_critical(parser))
			{
				break;
			}

			switch (ptcl_parser_current(parser)->type)
			{
			case ptcl_token_equals_type:
				*type = ptcl_statement_assign_type;
				break;
			default:
				// Can be already evaluated function with static void return type
				if (info->expression == NULL)
				{
					*type = ptcl_statement_none_type;
				}

				*type = ptcl_statement_func_call_type;
				break;
			}

			break;
		}
		default:
			*type = ptcl_statement_func_call_type;
			break;
		}

		break;
	}
	case ptcl_token_at_type:
		if (ptcl_parser_peek(parser, 1)->type == ptcl_token_left_curly_type)
		{
			*type = ptcl_statement_func_body_type;
			break;
		}

		break;
	case ptcl_token_auto_type:
	case ptcl_token_optional_type:
		*type = ptcl_statement_type_decl_type;
		break;
	case ptcl_token_return_type:
		*type = ptcl_statement_return_type;
		break;
	case ptcl_token_if_type:
		*type = ptcl_statement_if_type;
		break;
	case ptcl_token_each_type:
		*type = ptcl_statement_loop_type;
		break;
	case ptcl_token_syntax_type:
		*type = ptcl_statement_syntax_type;
		break;
	case ptcl_token_unsyntax_type:
		*type = ptcl_statement_unsyntax_type;
		break;
	case ptcl_token_undefine_type:
		*type = ptcl_statement_undefine_type;
		break;
	case ptcl_token_type_type:
		*type = ptcl_statement_type_decl_type;
		break;
	case ptcl_token_typedata_type:
		*type = ptcl_statement_typedata_decl_type;
		break;
	case ptcl_token_function_type:
		*type = ptcl_statement_func_decl_type;
		break;
	default:
		return false;
	}

	return true;
}

static ptcl_statement *ptcl_parser_syntax_stat(
	ptcl_parser *parser)
{
	size_t depth = parser->state.syntax_depth;
	ptcl_attributes attributes = ptcl_parser_parse_attributes(parser);
	if (ptcl_parser_critical(parser))
	{
		return NULL;
	}

	ptcl_location location = ptcl_parser_current(parser)->location;
	ptcl_statement *statement = NULL;
	if (!ptcl_parser_in_type(parser))
	{
		statement = ptcl_statement_create(ptcl_statement_func_body_type, ptcl_parser_root(parser), &attributes, location);
		if (statement == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			ptcl_attributes_destroy(&attributes);
			return NULL;
		}
	}

	ptcl_func_body body = ptcl_func_body_create(
		NULL, 0,
		parser->temp.inserted_body == NULL							 ? parser->temp.main_root
		: parser->temp.inserted_body->root == parser->temp.main_root ? parser->temp.inserted_body
																	 : parser->temp.main_root);
	ptcl_func_body *unique_body_pointer = malloc(sizeof(ptcl_func_body));
	if (unique_body_pointer == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		ptcl_attributes_destroy(&attributes);
		return false;
	}

	ptcl_parser_add_instance_unique_pointer(parser, unique_body_pointer);
	*unique_body_pointer = body;

	ptcl_parser_func_body_by_pointer(parser, unique_body_pointer, false, false, ptcl_parser_ignore_error(parser));
	body = *unique_body_pointer;
	if (parser->state.syntax_depth == depth)
	{
		ptcl_parser_leave_from_syntax(parser);
	}

	if (ptcl_parser_critical(parser))
	{
		free(statement);
		ptcl_attributes_destroy(&attributes);
		return NULL;
	}

	if (ptcl_parser_in_type(parser))
	{
		ptcl_func_body *previous = ptcl_parser_root(parser);
		ptcl_parser_set_root(parser, body.root);

		ptcl_func_body *root_body = ptcl_parser_root(parser);
		size_t new_count = root_body->count + body.count;
		ptcl_statement **new_statements = realloc(root_body->statements,
												  sizeof(ptcl_statement *) * new_count);
		if (new_statements == NULL && new_count > 0)
		{
			ptcl_func_body_destroy(&body);
			ptcl_parser_throw_out_of_memory(parser, location);
			ptcl_parser_set_root(parser, previous);
			return NULL;
		}

		for (size_t i = 0; i < body.count; i++)
		{
			new_statements[i + root_body->count] = body.statements[i];
		}

		free(body.statements);
		root_body->statements = new_statements;
		root_body->count = new_count;
		ptcl_parser_set_root(parser, previous);
	}
	else
	{
		statement->body = ptcl_statement_func_body_create_by_body(&body);
	}

	return statement;
}

ptcl_statement *ptcl_parser_parse_statement(
	ptcl_parser *parser)
{
	if (ptcl_parser_parse_try_syntax_usage_here(parser, true))
	{
		return ptcl_parser_syntax_stat(parser);
	}

	if (ptcl_parser_critical(parser))
	{
		return NULL;
	}

	ptcl_attributes attributes = ptcl_parser_parse_attributes(parser);
	if (ptcl_parser_critical(parser))
	{
		return NULL;
	}

	ptcl_statement *statement = NULL;
	ptcl_location location = ptcl_parser_current(parser)->location;
	ptcl_parser_statement_info info = ptcl_parser_statement_info_default(parser);
	if (!ptcl_parser_parse_get_statement(parser, &info))
	{
		ptcl_attributes_destroy(&attributes);
		ptcl_parser_throw_unknown_statement(parser, location);
		return NULL;
	}

	ptcl_statement_type type = info.type;
	// Already parsed in get_statement or just non sense things
	if (type == ptcl_statement_none_type)
	{
		ptcl_attributes_destroy(&attributes);
		return NULL;
	}

	const bool with_own_statement = ptcl_statement_with_own_statement(type);
	const ptcl_stat_mods modifiers = info.modifiers;
	ptcl_expression *expression = info.expression;
	ptcl_name name = info.name;

	// "If" will parse new statement anyway
	if (with_own_statement)
	{
		statement = ptcl_statement_create(type, ptcl_parser_root(parser), &attributes, location);
		if (statement == NULL)
		{
			ptcl_attributes_destroy(&attributes);
			ptcl_parser_throw_out_of_memory(parser, location);
			return NULL;
		}
	}

	switch (type)
	{
	case ptcl_statement_func_call_type:
	{
		bool is_null = false;
		if (!ptcl_parser_func_call_stat(parser, &statement, expression, &name, modifiers, &is_null, location))
		{
			// Statememnt, expression and name already destroyed above
			ptcl_attributes_destroy(&attributes);
			return NULL;
		}

		if (is_null)
		{
			// Attributes will be destroyed below
			statement = NULL;
		}

		// Statement is parsed in function above
		break;
	}
	case ptcl_statement_func_decl_type:
	{
		bool is_provider = false;
		if (!ptcl_parser_func_decl(parser, &is_provider, &attributes, modifiers, &statement->func_decl))
		{
			break;
		}

		if (is_provider)
		{
			// Provider saves attributes
			free(statement);
			return NULL;
		}

		break;
	}
	case ptcl_statement_typedata_decl_type:
	{
		bool is_provider = false;
		if (!ptcl_parser_typedata_decl(parser, &is_provider, &attributes, modifiers, &statement->typedata_decl))
		{
			break;
		}

		if (is_provider)
		{
			// Provider saves attributes
			free(statement);
			return NULL;
		}

		break;
	}
	case ptcl_statement_type_decl_type:
	{
		bool is_provider = false;
		if (!ptcl_parser_type_decl(parser, &is_provider, &attributes, modifiers, &statement->type_decl))
		{
			break;
		}

		if (is_provider)
		{
			// Provider saves attributes
			free(statement);
			return NULL;
		}

		break;
	}
	case ptcl_statement_assign_type:
	{
		bool is_provider = false;
		if (!ptcl_parser_assign(parser, &is_provider, &name, expression, &attributes, modifiers, &statement->assign))
		{
			break;
		}

		if (is_provider)
		{
			// Provider saves attributes
			free(statement);
			return NULL;
		}

		// By dot
		if (!ptcl_parser_critical(parser) && !statement->assign.identifier.is_name && statement->assign.identifier.value->return_type.is_static)
		{
			ptcl_expression_destroy(statement->assign.identifier.value);
			ptcl_attributes_destroy(&attributes);
			free(statement);
			return NULL;
		}

		break;
	}
	case ptcl_statement_return_type:
		ptcl_parser_return(parser, &statement->ret);
		break;
	case ptcl_statement_if_type:
		ptcl_parser_if(parser, ptcl_stat_mods_is_flags_static(modifiers), statement);
		break;
	case ptcl_statement_unsyntax_type:
		statement->type = ptcl_statement_func_body_type;
		ptcl_parser_unsyntax(parser, &statement->body);
		break;
	case ptcl_statement_loop_type:
		ptcl_parser_loop(parser, &statement->loop);
		break;
	case ptcl_statement_syntax_type:
		ptcl_parser_syntax_decl(parser);
		break;
	case ptcl_statement_undefine_type:
		ptcl_parser_undefine(parser);
		break;
	case ptcl_statement_func_body_type:
		// If we in syntax, then it was already parsed
		if (parser->state.syntax_depth == 0)
		{
			ptcl_parser_extra_body(parser, false);
		}
		else
		{
			ptcl_parser_skip(parser); // Skip '@'
			ptcl_parser_skip(parser); // Skip '{'
			ptcl_parser_skip_block_or_expression(parser);
		}

		break;
	case ptcl_statement_import_type:
	case ptcl_statement_none_type:
		break;
	}

	if (ptcl_parser_critical(parser))
	{
		if (statement != NULL)
		{
			ptcl_attributes_destroy(&attributes);
			free(statement);
		}

		return NULL;
	}

	// Syntax decl, body, static func call, if, or each
	// They don't have own statement, so we skip
	if (!with_own_statement || statement == NULL)
	{
		ptcl_attributes_destroy(&attributes);
		return NULL;
	}

	if (ptcl_parser_in_type(parser) && type != ptcl_statement_func_decl_type)
	{
		ptcl_attributes_destroy(&attributes);
		ptcl_parser_except(parser, ptcl_token_function_type);
		return NULL;
	}

	statement->root = ptcl_parser_root(parser);
	statement->attributes = attributes;
	return statement;
}

ptcl_attributes ptcl_parser_parse_attributes(
	ptcl_parser *parser)
{
	ptcl_attributes attributes = ptcl_attributes_create(NULL, 0, true);
	while (ptcl_parser_current(parser)->type == ptcl_token_at_type && ptcl_parser_peek(parser, 1)->type == ptcl_token_left_square_type)
	{
		ptcl_parser_skip(parser);
		ptcl_parser_skip(parser);
		ptcl_name name = ptcl_parser_name_word(parser);
		if (ptcl_parser_critical(parser))
		{
			return (ptcl_attributes){0};
		}

		if (ptcl_parser_not(parser, ptcl_token_right_square_type))
		{
			ptcl_name_destroy(&name);
			return (ptcl_attributes){0};
		}

		ptcl_attribute *buffer = realloc(attributes.attributes, (attributes.count + 1) * sizeof(ptcl_attribute));
		if (buffer == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			ptcl_attributes_destroy(&attributes);
			return (ptcl_attributes){0};
		}

		attributes.attributes = buffer;
		attributes.attributes[attributes.count++] = ptcl_attribute_create(&name);
	}

	return attributes;
}

bool ptcl_parser_parse_try_syntax_usage_here(
	ptcl_parser *parser,
	bool is_statement)
{
	return ptcl_parser_parse_try_syntax_usage(parser, NULL, 0, -1, false, is_statement);
}

static ptcl_expression *ptcl_parser_syntax_tokens(
	ptcl_parser *parser,
	char *end_token,
	ptcl_location location)
{
	if (!ptcl_parser_in_syntax(parser))
	{
		return false;
	}

	size_t original_capacity = 8;
	size_t original_count = 0;
	ptcl_expression **buffer = malloc(original_capacity * sizeof(ptcl_expression *));
	if (buffer == NULL && original_capacity > 0)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	while (true)
	{
		if (ptcl_parser_ended(parser))
		{
			ptcl_parser_throw_except_token(parser, end_token, location);
			return false;
		}

		ptcl_token *current = ptcl_parser_current(parser);
		ptcl_parser_skip(parser);
		if ((current->value == end_token || strcmp(current->value, end_token) != 0))
		{
			if (original_count >= original_capacity)
			{
				original_capacity *= 2;
				ptcl_expression **reallocated = realloc(buffer, original_capacity * sizeof(ptcl_expression *));
				if (reallocated == NULL)
				{
				destroying:
					for (size_t i = 0; i < original_count; i++)
					{
						ptcl_expression_destroy(buffer[i]);
					}

					free(buffer);
					ptcl_parser_throw_out_of_memory(parser, location);
					return false;
				}

				buffer = reallocated;
			}

			ptcl_expression *expression = ptcl_expression_create_token(current);
			if (expression == NULL)
			{
				goto destroying;
			}

			buffer[original_count++] = expression;
			continue;
		}

		ptcl_parser_back(parser);
		break;
	}

	ptcl_type array_type = ptcl_type_create_array(ptcl_get_token_t(), original_count, false, true, false);
	ptcl_expression *expression = ptcl_expression_create_array(
		&array_type,
		buffer,
		location);
	if (expression == NULL)
	{
		goto destroying;
	}

	return expression;
}

bool ptcl_parser_parse_try_syntax_usage(
	ptcl_parser *parser,
	ptcl_parser_syntax_node **nodes,
	size_t count,
	int down_start,
	bool skip_first,
	bool is_statement)
{
	const bool is_root = down_start < 0;
	if (is_root && !ptcl_parser_match(parser, ptcl_token_hashtag_type))
	{
		return false;
	}

	size_t stop = ptcl_parser_position(parser);
	const size_t start = is_root ? stop : (size_t)down_start;
	const size_t original_count = count;

	ptcl_parser_syntax syntax = ptcl_parser_syntax_create(
		&ptcl_name_empty,
		ptcl_parser_root(parser),
		nodes ? *nodes : NULL,
		count, 0);
	ptcl_parser_syntax result;
	bool found = false;
	while (true)
	{
		if (ptcl_parser_ended(parser))
		{
			bool can_continue = false;
			char *end_token = NULL;
			ptcl_parser_syntax *target = NULL;
			bool current_found = ptcl_parser_syntax_try_find(parser, syntax.nodes, syntax.count, &target, &can_continue, &end_token);
			if (current_found)
			{
				stop = ptcl_parser_position(parser);
				result = *target;
				found = true;
			}

			break;
		}

		if (!skip_first)
		{
			ptcl_token *current = ptcl_parser_current(parser);
			ptcl_parser_syntax_node *buffer = realloc(syntax.nodes, (syntax.count + 1) * sizeof(ptcl_parser_syntax_node));
			if (buffer == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, current->location);
				ptcl_parser_syntax_destroy(&syntax);
				ptcl_parser_set_position(parser, start);
				break;
			}

			syntax.nodes = buffer;
			const size_t position = ptcl_parser_position(parser);
			const ptcl_name name = ptcl_name_create_borrowed(current->value, false);
			ptcl_parser_syntax_node node = ptcl_parser_syntax_node_create_word(
				current->type, &name);
			ptcl_parser_skip(parser);
			syntax.nodes[syntax.count++] = node;
			// TODO: cast is danger
			if (ptcl_parser_parse_try_syntax_usage(parser, &syntax.nodes, syntax.count, (int)start, true, is_statement))
			{
				return true;
			}

			if (ptcl_parser_critical(parser))
			{
				// Already destroyed in func below
				return false;
			}

			ptcl_parser_set_position(parser, position);
			const bool last_mode = ptcl_parser_add_errors(parser);
			ptcl_parser_set_state(parser, PTCL_PARSER_ADD_ERRORS_FLAG, is_root);

			const ptcl_parser_tokens_state state = parser->state.tokens;
			ptcl_expression *value = ptcl_parser_cast(parser, NULL, true);

			ptcl_parser_set_state(parser, PTCL_PARSER_ADD_ERRORS_FLAG, last_mode);
			if (ptcl_parser_critical(parser))
			{
				ptcl_parser_disable_state(parser, PTCL_PARSER_CRITICAL_FLAG);
				break;
			}
			else
			{
				node = ptcl_parser_syntax_node_create_value(value, &state);
				syntax.nodes[syntax.count - 1] = node;
			}
		}
		else
		{
			skip_first = false;
		}

		// Try to find matching syntax
		ptcl_parser_syntax *target = NULL;
		bool can_continue = false;
		char *end_token = NULL;
		bool current_found = ptcl_parser_syntax_try_find(parser, syntax.nodes, syntax.count, &target, &can_continue, &end_token);
		if (current_found)
		{
			stop = ptcl_parser_position(parser);
			result = *target;
			found = true;
		}

		if (!can_continue)
		{
			break;
		}

		if (end_token != NULL)
		{
			ptcl_location location = ptcl_parser_current(parser)->location;
			ptcl_parser_syntax_node *nodes = realloc(syntax.nodes, (syntax.count + 1) * sizeof(ptcl_parser_syntax_node));
			if (nodes == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, location);
				ptcl_parser_syntax_destroy(&syntax);
				return false;
			}

			syntax.nodes = nodes;
			ptcl_expression *expression = ptcl_parser_syntax_tokens(parser, end_token, location);
			if (ptcl_parser_critical(parser))
			{
				ptcl_parser_syntax_destroy(&syntax);
				return false;
			}

			ptcl_parser_tokens_state tokens_state = (ptcl_parser_tokens_state){0};
			syntax.nodes[syntax.count++] = ptcl_parser_syntax_node_create_value(expression, &tokens_state);
		}
	}

	if (found)
	{
		// Handle syntax depth and memory limits
		if (parser->state.syntax_depth == 0)
		{
			parser->temp.main_root = ptcl_parser_root(parser);
		}

		if (parser->state.syntax_depth + 1 >= PTCL_PARSER_MAX_DEPTH)
		{
			ptcl_parser_throw_max_depth(parser, ptcl_parser_current(parser)->location);
			ptcl_parser_syntax_destroy(&syntax);
			return false;
		}

		// Create new function body context
		ptcl_func_body *temp = malloc(sizeof(ptcl_func_body));
		if (temp == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			ptcl_parser_syntax_destroy(&syntax);
			return false;
		}

		*temp = ptcl_func_body_create(NULL, 0, ptcl_parser_current_body(parser));
		ptcl_parser_set_root(parser, temp);

		// Process syntax variables
		for (size_t i = 0; i < result.count; i++)
		{
			ptcl_parser_syntax_node *syntax_node = &result.nodes[i];
			ptcl_parser_syntax_node *target_node = &syntax.nodes[i];
			if (syntax_node->type != ptcl_parser_syntax_node_variable_type)
			{
				ptcl_parser_syntax_node_destroy(target_node);
				continue;
			}

			ptcl_expression *expression = target_node->value.value;
			if (expression->return_type.is_static &&
				expression->return_type.type == ptcl_value_type_type &&
				expression->return_type.comp_type->invariant != NULL)
			{
				ptcl_expression_destroy(expression);
				const ptcl_parser_tokens_state previous_state = parser->state.tokens;
				ptcl_parser_set_tokens_state(parser, &target_node->value.state);
				expression = ptcl_parser_cast(parser, NULL, ptcl_parser_expr_with_word_flag | ptcl_parser_expr_change_the_value_flag);
			}

			ptcl_name name = ptcl_name_create_borrowed(syntax_node->variable.name, false);
			ptcl_parser_variable variable = ptcl_parser_variable_create(
				&name,
				&syntax_node->variable.type,
				expression,
				true,
				ptcl_parser_root(parser));
			variable.built_in = expression;
			if (expression->return_type.is_static && expression->return_type.type == ptcl_value_word_type)
			{
				PTCL_FLAGS_SET(variable.flags, PTCL_PARSER_VAR_SYNTAX_WORD_FLAG);
			}

			if (!ptcl_parser_add_instance_variable(parser, &variable))
			{
				ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
				ptcl_parser_syntax_destroy(&syntax);
				return false;
			}
		}

		if (ptcl_parser_critical(parser))
		{
			ptcl_parser_syntax_destroy(&syntax);
			return false;
		}

		ptcl_parser_set_position(parser, stop);
		size_t current = parser->state.syntax_depth++;
		parser->state.syntaxes_nodes[current] = ptcl_parser_syntax_pair_create(
			&syntax, &parser->state.tokens, temp,
			parser->state.syntax_depth == 1 ? temp->root : parser->state.syntaxes_nodes[current - 1].body);
		ptcl_parser_tokens_state *lated_body = &parser->lated_states.items[result.index];
		ptcl_parser_set_tokens_state(parser, lated_body);
		ptcl_parser_set_position(parser, 0);
	}
	else
	{
		ptcl_parser_set_position(parser, start);
		if (is_root)
		{
			ptcl_parser_throw_unknown_syntax(parser, &syntax, ptcl_parser_tokens(parser)[start].location);
			ptcl_parser_syntax_destroy(&syntax);
		}
		else
		{
			if (nodes != NULL)
			{
				*nodes = syntax.nodes;
			}

			for (size_t i = original_count; i < syntax.count; i++)
			{
				ptcl_parser_syntax_node_destroy(&syntax.nodes[i]);
			}
		}
	}

	return found;
}

void ptcl_parser_leave_from_syntax(ptcl_parser *parser)
{
	ptcl_parser_clear_scope(parser);

	ptcl_parser_syntax_pair *pair = &parser->state.syntaxes_nodes[--parser->state.syntax_depth];
	if (parser->state.tokens.is_free)
	{
		free(ptcl_parser_tokens(parser));
	}

	ptcl_parser_set_tokens_state(parser, &pair->state);
	ptcl_func_body *temp = pair->body;
	ptcl_func_body_destroy(temp);
	ptcl_parser_set_root(parser, pair->previous_body);
	free(pair->syntax.nodes);
	free(temp);
}

void ptcl_parser_leave_from_insert_state(ptcl_parser *parser)
{
	ptcl_parser_tokens_state_destroy(&parser->state.tokens);
	ptcl_parser_set_tokens_state(parser, &ptcl_parser_insert_state_at(parser, --parser->temp.insert_states_count)->state);
}

static inline ptcl_statement_func_call ptcl_handle_builtin_execution(
	ptcl_parser *parser,
	ptcl_parser_function *target,
	ptcl_statement_func_call *func_call,
	ptcl_location location,
	bool is_expression)
{
	if (ptcl_parser_ignore_error(parser) && ptcl_parser_is_error_name(&func_call->identifier.name))
	{
		return *func_call;
	}

	ptcl_expression *result = target->bind(parser, func_call->arguments, func_call->count, location, is_expression);
	if (ptcl_parser_critical(parser))
	{
		return (ptcl_statement_func_call){0};
	}

	func_call->is_built_in = true;
	func_call->built_in = result;
	return *func_call;
}

static inline ptcl_statement_func_call ptcl_handle_static_function(
	ptcl_parser *parser,
	ptcl_parser_function *target,
	ptcl_statement_func_call *func_call,
	ptcl_expression *self,
	ptcl_location location)
{
	func_call->is_self_used = false;
	ptcl_interpreter_reset(parser->interpreter);
	ptcl_expression *result = ptcl_interpreter_evaluate_function_call(parser->interpreter, func_call, false, self, &func_call->is_self_used, location);
	if (ptcl_parser_critical(parser))
	{
		ptcl_statement_func_call_destroy(func_call);
		return (ptcl_statement_func_call){0};
	}

	func_call->is_built_in = result != NULL;
	if (!func_call->is_self_used && self != NULL)
	{
		ptcl_expression_destroy(self);
	}

	if (func_call->is_built_in)
	{
		ptcl_statement_func_call_destroy(func_call);
	}

	func_call->built_in = result;
	return *func_call;
}

// TODO: remove to node
static ptcl_expression *ptcl_get_self(
	ptcl_expression *expression)
{
	return expression->type == ptcl_expression_cast_type ? ptcl_get_self(expression->cast.value) : expression;
}

static inline ptcl_statement_func_call ptcl_handle_static_return_function(
	ptcl_parser *parser,
	ptcl_parser_function *target,
	ptcl_statement_func_call *func_call,
	ptcl_expression *self,
	bool is_expression,
	ptcl_location location)
{
	ptcl_statement_func_body func_body = ptcl_statement_func_body_create(NULL, 0, NULL);
	ptcl_statement *statement = ptcl_func_body_create_stat(&func_body, parser->temp.root, location);
	if (statement == NULL)
	{
		ptcl_statement_func_call_destroy(func_call);
		ptcl_parser_throw_out_of_memory(parser, location);
		return (ptcl_statement_func_call){0};
	}

	ptcl_statement_func_body *placeholder = malloc(sizeof(ptcl_statement_func_body));
	if (placeholder == NULL)
	{
		ptcl_statement_func_call_destroy(func_call);
		free(placeholder);
		free(statement);
		ptcl_parser_throw_out_of_memory(parser, location);
		return (ptcl_statement_func_call){0};
	}

	func_call->is_self_used = false;
	const bool is_self = self != NULL && self->return_type.is_static;
	*placeholder = ptcl_statement_func_body_inserted_create(
		NULL, 0, target->root, NULL, target->func.arguments, target->func.count, is_self ? NULL : self, *func_call, true);
	statement->root = &placeholder->body;
	int self_identifier = -1;
	if (self != NULL)
	{
		ptcl_expression *value = self;
		const bool is_built_in = self->return_type.is_static;
		if (is_built_in)
		{
			value = ptcl_get_self(value);
		}

		self_identifier = (int)parser->variables.count;
		ptcl_type type = value == self ? value->return_type.comp_type->types[0].type : value->return_type;
		ptcl_parser_variable variable = ptcl_parser_variable_create(&ptcl_name_self, &type, value, is_built_in, &placeholder->body);
		if (!ptcl_parser_add_instance_variable(parser, &variable))
		{
			ptcl_statement_func_call_destroy(func_call);
			free(placeholder);
			free(statement);
			ptcl_parser_throw_out_of_memory(parser, location);
			return (ptcl_statement_func_call){0};
		}
	}

	size_t variables_index = parser->variables.count;
	for (size_t i = 0; i < func_call->count; i++)
	{
		ptcl_expression *argument = func_call->arguments[i];
		ptcl_parser_variable variable = ptcl_parser_variable_create_source(
			&target->func.arguments[i].name,
			&argument->return_type,
			argument,
			&placeholder->body);
		if (!ptcl_parser_add_instance_variable(parser, &variable))
		{
			for (size_t j = i; j < func_call->count; j++)
			{
				ptcl_expression_destroy(func_call->arguments[j]);
			}

			free(func_call->arguments);
			free(placeholder);
			free(statement);
			ptcl_parser_throw_out_of_memory(parser, location);
			if (self_identifier > -1)
			{
				PTCL_FLAGS_REMOVE(parser->variables.items[self_identifier].flags, PTCL_PARSER_VAR_BUILT_IN_FLAG);
			}

			return (ptcl_statement_func_call){0};
		}
	}

	bool last_state = ptcl_parser_except_return(parser);
	ptcl_expression *last_return_value = parser->temp.return_value;
	const ptcl_type *last_return_type = parser->temp.return_type;
	ptcl_type func_return_type = target->func.return_type;
	// static (...): static T -> inline without static return type by specification
	if (func_return_type.is_static && ptcl_stat_mods_is_flags_static(target->func.modifiers))
	{
		// TODO: func
		if (func_return_type.type != ptcl_value_word_type && func_return_type.type != ptcl_value_function_pointer_type)
		{
			func_return_type.is_static = false;
		}
	}

	ptcl_parser_enable_state(parser, PTCL_PARSER_EXCEPT_RETURN_FLAG);
	parser->temp.return_type = &func_return_type;

	ptcl_parser_tokens_state last_tokens = parser->state.tokens;
	ptcl_parser_tokens_state *body = &parser->lated_states.items[target->func.index];
	ptcl_parser_set_tokens_state(parser, body);
	ptcl_parser_set_position(parser, 0);

	ptcl_parser_func_body_by_pointer(parser, &placeholder->body, true, true, false);

	ptcl_parser_set_tokens_state(parser, &last_tokens);

	ptcl_parser_variable *self_variable = NULL;
	if (self_identifier > -1)
	{
		self_variable = &parser->variables.items[self_identifier];
		PTCL_FLAGS_REMOVE(self_variable->flags, PTCL_PARSER_VAR_BUILT_IN_FLAG);
	}

	parser->temp.return_type = last_return_type;
	ptcl_parser_set_state(
		parser, PTCL_PARSER_EXCEPT_RETURN_FLAG, last_state);
	if (ptcl_parser_critical(parser))
	{
	cleanup:
		ptcl_statement_func_call_destroy(func_call);
		if (placeholder != NULL)
		{
			free(placeholder);
		}

		free(statement);
		return (ptcl_statement_func_call){0};
	}

	if (is_expression)
	{
		placeholder->caller = parser->temp.return_value;
	}

	statement->body = *placeholder;
	statement->body.func_call = *func_call;
	free(placeholder);
	placeholder = NULL;

	ptcl_func_body *root_body = ptcl_parser_current_body(parser);
	size_t new_count = root_body->count + 1;
	ptcl_statement **new_statements = realloc(root_body->statements,
											  sizeof(ptcl_statement *) * new_count);
	if (new_statements == NULL)
	{
		goto cleanup;
	}

	new_statements[root_body->count] = statement;
	root_body->statements = new_statements;
	root_body->count = new_count;

	func_call->is_built_in = true;
	func_call->built_in = parser->temp.return_value;
	parser->temp.return_value = last_return_value;
	if (self_variable != NULL && !ptcl_parser_var_flags_is_used(self_variable->flags))
	{
		ptcl_expression_destroy(self);
		statement->body.self = NULL;
	}

	return *func_call;
}

static bool ptcl_parser_func_call_args(
	ptcl_parser *parser,
	ptcl_statement_func_call *func_call,
	ptcl_statement_func_decl *target,
	ptcl_location location)
{
	size_t expected_arguments = target->is_variadic ? target->count + 4 : target->count;
	if (expected_arguments > 0)
	{
		func_call->arguments = malloc(expected_arguments * sizeof(ptcl_expression *));
		if (func_call->arguments == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			return false;
		}
	}

	const size_t start = ptcl_parser_insert_states_count(parser);
	bool is_parsing_tokens = false;

	const bool last_state = ptcl_parser_has_state(parser, PTCL_PARSER_IN_RETURN_FLAG);
	ptcl_parser_disable_state(parser, PTCL_PARSER_IN_RETURN_FLAG);
	while (!ptcl_parser_match(parser, ptcl_token_right_par_type))
	{
		if (ptcl_parser_ended(parser))
		{
			if (is_parsing_tokens)
			{
				ptcl_parser_leave_from_insert_state(parser);
				if (ptcl_parser_insert_states_count(parser) == start)
				{
					is_parsing_tokens = false;
				}

				continue;
			}

			ptcl_parser_throw_except_token(
				parser,
				ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_par_type),
				location);
			goto leave;
		}

		ptcl_parser_tokens_state temp_state;
		bool is_toggle = ptcl_parser_try_parse_insert(parser, &temp_state, ptcl_parser_current(parser)->location);
		if (ptcl_parser_critical(parser))
		{
			goto leave;
		}

		if (ptcl_parser_insert_states_count(parser) >= PTCL_PARSER_DECL_INSERT_DEPTH)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			goto leave;
		}

		if (is_toggle)
		{
			is_parsing_tokens = true;
			ptcl_parser_insert_state insert_state = ptcl_parser_insert_state_create(&temp_state, parser->state.syntax_depth);
			ptcl_parser_insert_state_add_and_set(parser, &insert_state);
			continue;
		}

		if (!target->is_variadic && func_call->count >= target->count)
		{
			ptcl_parser_throw_wrong_arguments(parser, ptcl_name_value(&target->identifier), func_call->arguments,
											  func_call->count, target->arguments, target->count, location);
			goto leave;
		}

		if (func_call->count >= expected_arguments)
		{
			expected_arguments = expected_arguments * 2 + 4;
			ptcl_expression **buffer = realloc(func_call->arguments, expected_arguments * sizeof(ptcl_expression *));
			if (buffer == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, location);
			leave:
				ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, last_state);
				ptcl_statement_func_call_destroy(func_call);
				if (ptcl_parser_insert_states_count(parser) > 1)
				{
					for (size_t i = start + 1; i < ptcl_parser_insert_states_count(parser); i++)
					{
						free(ptcl_parser_insert_state_at(parser, i)->state.tokens);
					}

					free(parser->state.tokens.tokens);
					ptcl_parser_set_tokens_state(parser, &ptcl_parser_insert_state_at(parser, 0)->state);
				}

				return false;
			}

			func_call->arguments = buffer;
		}

		ptcl_argument *argument = (func_call->count < target->count) ? &target->arguments[func_call->count] : NULL;
		ptcl_type *excepted = (argument && !argument->is_variadic) ? &argument->type : NULL;
		func_call->arguments[func_call->count] = ptcl_parser_cast(parser, excepted, ptcl_parser_expr_flags_default(true));
		if (ptcl_parser_critical(parser))
		{
			goto leave;
		}

		func_call->count++;
		ptcl_parser_match(parser, ptcl_token_comma_type);
	}

	ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, last_state);
	const bool argument_count_mismatch = target->is_variadic
											 ? (func_call->count < target->count - 1)
											 : (func_call->count != target->count);
	if (argument_count_mismatch)
	{
		ptcl_parser_throw_wrong_arguments(parser, ptcl_name_value(&target->identifier), func_call->arguments,
										  func_call->count, target->arguments, target->count, location);
		ptcl_statement_func_call_destroy(func_call);
		return false;
	}

	return true;
}

bool ptcl_parser_func_call(
	ptcl_parser *parser,
	ptcl_parser_function *function,
	ptcl_expression *self,
	bool is_expression,
	ptcl_statement_func_call *const out)
{
	ptcl_location location = ptcl_parser_current(parser)->location;
	ptcl_identifier identifier;
	ptcl_name name;
	if (function != NULL)
	{
		name = ptcl_name_create_shallow_copy(&function->identifier);
		identifier = ptcl_identifier_create_by_name(&name);
	}
	else
	{
		const size_t start = ptcl_parser_position(parser);
		name = ptcl_parser_name_word(parser);
		if (ptcl_parser_critical(parser))
		{
			return false;
		}

		if (ptcl_parser_current(parser)->type != ptcl_token_left_par_type)
		{
			ptcl_parser_expr_flags flags = ptcl_parser_expr_flags_default(is_expression);
			ptcl_expression *variable = ptcl_parser_func_call_or_var(parser, &name, ptcl_parser_current(parser), flags, false);
			if (ptcl_parser_critical(parser))
			{
				return false;
			}

			ptcl_expression *expression = ptcl_parser_dot(parser, NULL, variable, flags);
			if (ptcl_parser_critical(parser))
			{
				ptcl_expression_destroy(variable);
				return false;
			}

			identifier = ptcl_identifier_create_by_expr(expression);
			*out = ptcl_statement_func_call_create(&identifier, expression->dot.right->func_call.function_id, NULL, 0);
			return true;
		}

		identifier = ptcl_identifier_create_by_name(&name);
	}

	if (ptcl_parser_not(parser, ptcl_token_left_par_type))
	{
		ptcl_identifier_destroy(&identifier);
		return false;
	}

	if (function == NULL)
	{
		if (!ptcl_parser_try_get_function(parser, &name, &function))
		{
			ptcl_identifier_destroy(&identifier);
			ptcl_parser_throw_unknown_function(parser, ptcl_name_value(&name), location);
			return false;
		}
	}

	ptcl_statement_func_decl *target = &function->func;
	if (is_expression && target->return_type.type == ptcl_value_void_type)
	{
		ptcl_identifier_destroy(&identifier);
		ptcl_parser_throw_except_type_specifier(parser, ptcl_parser_current(parser)->location);
		return false;
	}

	*out = ptcl_statement_func_call_create(&identifier, function - parser->functions.items, NULL, 0);
	if (!ptcl_parser_func_call_args(parser, out, target, location))
	{
		return false;
	}

	out->return_type = target->return_type;
	if (function != NULL && function->is_built_in)
	{
		*out = ptcl_handle_builtin_execution(parser, function, out, location, is_expression);
	}
	else if (target->return_type.is_static)
	{
		*out = ptcl_handle_static_return_function(parser, function, out, self, is_expression, location);
	}
	else if (ptcl_stat_mods_is_flags_static(target->modifiers))
	{
		*out = ptcl_handle_static_function(parser, function, out, self, location);
	}
	else
	{
		out->is_built_in = false;
	}

	return !ptcl_parser_critical(parser);
}

bool ptcl_parser_func_body(
	ptcl_parser *parser,
	bool with_brackets,
	bool change_root,
	bool is_ignore_error,
	ptcl_func_body *const out)
{
	*out = (ptcl_func_body){0};
	ptcl_parser_func_body_by_pointer(parser, out, with_brackets, change_root, is_ignore_error);
	return !ptcl_parser_critical(parser);
}

bool ptcl_parser_func_body_by_pointer(
	ptcl_parser *parser,
	ptcl_func_body *func_body_pointer,
	bool with_brackets,
	bool change_root,
	bool is_ignore_error)
{
	size_t placeholder = 0;
	return ptcl_parser_func_body_by_pointer_cap(parser, func_body_pointer, &placeholder, with_brackets, change_root, is_ignore_error);
}

bool ptcl_parser_func_body_by_pointer_cap(
	ptcl_parser *parser,
	ptcl_func_body *func_body_pointer,
	size_t *capacity,
	bool with_brackets,
	bool change_root,
	bool is_ignore_error)
{
	ptcl_parser_match(parser, ptcl_token_left_curly_type);

	ptcl_func_body *previous = ptcl_parser_root(parser);
	ptcl_func_body *previous_main = parser->temp.main_root;
	if (change_root)
	{
		ptcl_parser_set_root(parser, func_body_pointer);
		parser->temp.main_root = func_body_pointer;
	}

	const bool last_ignore_error = ptcl_parser_ignore_error(parser);
	ptcl_parser_set_state(parser, PTCL_PARSER_IGNORE_ERROR_FLAG, is_ignore_error);

	bool leave_from_extra_body = false;
	size_t depth = parser->state.syntax_depth;
	if (*capacity == 0)
	{
		*capacity = func_body_pointer->count;
	}

	while (depth <= parser->state.syntax_depth)
	{
		if (with_brackets && ptcl_parser_match(parser, ptcl_token_right_curly_type))
		{
			break;
		}

		if (ptcl_parser_ended(parser))
		{
			if (with_brackets)
			{
				ptcl_location location = ptcl_parser_current(parser)->location;
				char *token = ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_curly_type);
				ptcl_parser_throw_except_token(parser, token, location);
				ptcl_func_body_destroy(func_body_pointer);
			}

			break;
		}

		const size_t extra_bodies_depth = parser->state.extra_bodies_depth;
		ptcl_statement *statement = ptcl_parser_parse_statement(parser);
		if (parser->state.extra_bodies_depth > extra_bodies_depth)
		{
			leave_from_extra_body = true;
		}

		if (leave_from_extra_body)
		{
			ptcl_parser_leave_from_extra_body(parser);
			leave_from_extra_body = false;
		}

		if (ptcl_parser_critical(parser))
		{
			ptcl_parser_clear_scope(parser);
			ptcl_func_body_destroy(func_body_pointer);
			break;
		}

		if (statement == NULL)
		{
			continue;
		}

		if (ptcl_parser_except_return(parser) && statement->type == ptcl_statement_return_type)
		{
			parser->temp.return_value = statement->ret.value;
			free(statement);
			break;
		}

		if (func_body_pointer->count >= *capacity)
		{
			if (*capacity == 0)
			{
				*capacity = 4;
				if (func_body_pointer->count >= *capacity)
				{
					*capacity = func_body_pointer->count * 2;
				}
			}
			else
			{
				// body count can be changed from other side
				*capacity = func_body_pointer->count * 2;
			}

			ptcl_statement **buffer = realloc(func_body_pointer->statements, *capacity * sizeof(ptcl_statement *));
			if (buffer == NULL)
			{
				ptcl_func_body_destroy(func_body_pointer);
				ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
				break;
			}

			func_body_pointer->statements = buffer;
		}

		func_body_pointer->statements[func_body_pointer->count++] = statement;
	}

	ptcl_parser_set_state(parser, PTCL_PARSER_IGNORE_ERROR_FLAG, last_ignore_error);
	if (change_root)
	{
		ptcl_parser_clear_scope(parser);
		ptcl_parser_set_root(parser, previous);
		parser->temp.main_root = previous_main;
	}

	return !ptcl_parser_critical(parser);
}

ptcl_func_body *ptcl_parser_current_body(
	ptcl_parser *parser)
{
	return parser->temp.inserted_body == NULL							? parser->temp.main_root
		   : parser->temp.inserted_body->root == parser->temp.main_root ? parser->temp.inserted_body
																		: parser->temp.main_root;
}

static bool ptcl_parser_add_statement_to_root(
	ptcl_parser *parser,
	ptcl_statement *statement,
	ptcl_location location)
{
	ptcl_func_body *root_body = ptcl_parser_current_body(parser);
	size_t new_count = root_body->count + 1;
	ptcl_statement **new_statements = realloc(root_body->statements,
											  sizeof(ptcl_statement *) * new_count);
	if (new_statements == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	new_statements[root_body->count] = statement;
	root_body->statements = new_statements;
	root_body->count = new_count;
	return true;
}

bool ptcl_parser_extra_body(
	ptcl_parser *parser,
	bool is_syntax)
{
	if (ptcl_parser_current(parser)->type != ptcl_token_at_type || ptcl_parser_peek(parser, 1)->type != ptcl_token_left_curly_type)
	{
		return false;
	}

	ptcl_location location = ptcl_parser_current(parser)->location;
	if (parser->state.extra_bodies_depth >= PTCL_PARSER_MAX_DEPTH)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	ptcl_parser_skip(parser);
	ptcl_func_body *unique_body_pointer = malloc(sizeof(ptcl_func_body));
	if (unique_body_pointer == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	const ptcl_func_body *previous = ptcl_parser_root(parser);
	*unique_body_pointer = ptcl_func_body_create(
		NULL, 0, previous);
	ptcl_parser_set_root(parser, unique_body_pointer);
	if (!ptcl_parser_add_instance_unique_pointer(parser, unique_body_pointer))
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	parser->state.extra_bodies[parser->state.extra_bodies_depth++] = (ptcl_extra_body){
		.root = previous,
	};

	ptcl_parser_func_body_by_pointer(parser, unique_body_pointer, true, false, ptcl_parser_ignore_error(parser));
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	ptcl_statement_func_body func_body = ptcl_statement_func_body_create_by_body(unique_body_pointer);
	ptcl_statement *statement = ptcl_func_body_create_stat(&func_body, unique_body_pointer->root, location);
	if (statement == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
	cleanup:
		ptcl_parser_leave_from_extra_body(parser);
		ptcl_statement_destroy(statement);
		ptcl_parser_set_root(parser, previous);
		return false;
	}

	if (!ptcl_parser_add_statement_to_root(parser, statement, location))
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		goto cleanup;
	}

	const bool is_succesful = ptcl_parser_extra_body(parser, is_syntax);
	const bool is_critical = ptcl_parser_critical(parser);
	if (is_critical)
	{
		ptcl_parser_leave_from_extra_body(parser);
	}

	return !is_critical;
}

void ptcl_parser_leave_from_extra_body(
	ptcl_parser *parser)
{
	ptcl_extra_body *pair = &parser->state.extra_bodies[--parser->state.extra_bodies_depth];
	ptcl_parser_set_root(parser, pair->root);
}

static bool ptcl_parser_get_comp_type(
	ptcl_parser_comp_type *instance,
	ptcl_type *comp_type,
	bool *is_static,
	bool with_static,
	bool is_const)
{
	// TODO: context
	if (with_static && (*is_static || instance->is_auto_static))
	{
		if (instance->static_type == NULL)
		{
			return false;
		}

		*comp_type = ptcl_type_create_comp_type_t(instance->static_type, is_const);
		*is_static = true;
	}
	else
	{
		if (instance->comp_type == NULL)
		{
			return false;
		}

		*comp_type = ptcl_type_create_comp_type_t(instance->comp_type, is_const);
	}

	return true;
}

static bool ptcl_parser_try_get_variant(
	ptcl_parser *parser,
	ptcl_parser_provider *provider,
	const ptcl_statement_func_call *const func_call,
	bool is_static,
	ptrdiff_t *index,
	void **already_result)
{
	PTCL_VALIDATE_COND(func_call != NULL);
	for (size_t i = 0; i < provider->variants_count; i++)
	{
		bool broken = false;
		ptcl_parser_provider_variant *variant = &provider->variants[i];
		if (variant->count != func_call->count)
		{
			continue;
		}

		for (size_t j = 0; j < variant->count; j++)
		{
			if (!ptcl_expression_equals(variant->values[j], func_call->arguments[j]))
			{
				broken = true;
				break;
			}
		}

		if (broken)
		{
			continue;
		}

		if (provider->type == ptcl_statement_type_decl_type)
		{
			ptcl_parser_comp_type *comp_type = &parser->comp_types.items[variant->member];
			ptcl_type placeholder;
			// Do we have correct static or non static type
			if (!ptcl_parser_get_comp_type(comp_type, &placeholder, &is_static, true, false))
			{
				continue;
			}

			*already_result = comp_type;
			return true;
		}

		*index = variant->member;
		return true;
	}

	return false;
}

static bool ptcl_parser_try_generate_or_get_provider(
	ptcl_parser *parser,
	ptcl_statement_type type,
	ptrdiff_t provider_index,
	void **result,
	bool is_static,
	ptcl_location location)
{
	// TODO
	ptcl_parser_provider *provider = &parser->providers.items[provider_index];
	if (provider->type != type)
	{
		// TODO:
		ptcl_parser_throw_incorrect_type(parser, "typedata", "gsad", location);
		return false;
	}

	ptcl_identifier identifier = ptcl_identifier_create_by_name(&provider->identifier);
	ptcl_statement_func_call func_call = ptcl_statement_func_call_create(&identifier, 0, NULL, 0);
	if (ptcl_parser_match(parser, ptcl_token_left_par_type))
	{
		if (!ptcl_parser_func_call_args(parser, &func_call, &provider->func_decl, location))
		{
			return false;
		}
	}

	ptrdiff_t index = 0;
	if (provider->is_provider)
	{
		void *already_result = NULL;
		if (ptcl_parser_try_get_variant(parser, provider, &func_call, is_static, &index, &already_result))
		{
			ptcl_statement_func_call_destroy(&func_call);
			if (already_result != NULL)
			{
				*result = already_result;
				return true;
			}

			goto result_by_index;
		}
	}

	char *count = ptcl_from_long(provider - parser->providers.items);
	// We are counting from unused
	char *variant_index;
	if (provider->is_provider)
	{
		variant_index = ptcl_from_long(provider->variants_count);
	}
	else
	{
		variant_index = ptcl_from_long(parser->providers.generators_count++);
	}

	if (count == NULL || variant_index == NULL)
	{
		free(count);
		free(variant_index);
		ptcl_statement_func_call_destroy(&func_call);
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	ptcl_name name = ptcl_name_create_owned(ptcl_string(ptcl_name_value(&provider->identifier), count, variant_index, NULL), false);
	free(count);
	free(variant_index);
	if (ptcl_name_value(&name) == NULL)
	{
		ptcl_statement_func_call_destroy(&func_call);
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	ptcl_parser_temp *temp = &parser->temp;
	ptcl_parser_tokens_state *late = &parser->lated_states.items[provider->index];
	ptcl_parser_tokens_state state = parser->state.tokens;
	ptcl_parser_set_tokens_state(parser, late);
	ptcl_parser_set_position(parser, 0);

	// TODO: func_call and this to one func
	// TODO: also variadic
	size_t variables_index = parser->variables.count;
	for (size_t i = 0; i < func_call.count; i++)
	{
		ptcl_expression *argument = func_call.arguments[i];
		ptcl_parser_variable variable = ptcl_parser_variable_create(
			&provider->func_decl.arguments[i].name,
			&argument->return_type,
			argument,
			true,
			ptcl_parser_root(parser));
		if (!ptcl_parser_add_instance_variable(parser, &variable))
		{
			ptcl_statement_func_call_destroy(&func_call);
			ptcl_name_destroy(&name);
			ptcl_parser_throw_out_of_memory(parser, location);
			return false;
		}
	}

	ptcl_statement_func_decl func_decl;
	ptcl_statement_typedata_decl typedata_decl;
	ptcl_statement_type_decl type_decl;
	ptcl_statement_assign assign_decl;
	bool is_succesful = true;
	switch (type)
	{
	case ptcl_statement_func_decl_type:
		is_succesful = ptcl_parser_func_body_and_storage(parser, &name, &func_decl, NULL, false, provider->modifiers);
		break;
	case ptcl_statement_typedata_decl_type:
		is_succesful = ptcl_parser_typedata_body_and_storage(parser, &name, &typedata_decl, provider->modifiers);
		break;
	case ptcl_statement_type_decl_type:
		is_succesful = ptcl_parser_type_body_and_storage(parser, &name, &type_decl, provider->modifiers);
		break;
	case ptcl_statement_assign_type:
	{
		ptcl_identifier identifier = ptcl_identifier_create_by_name(&name);
		is_succesful = ptcl_parser_assign_var_and_storage(parser, &identifier, &assign_decl, true, provider->modifiers, location);
		break;
	}
	default:
		return false;
	}

	ptcl_parser_set_tokens_state(parser, &state);
	if (!is_succesful)
	{
	cleanup:
		for (size_t i = 0; i < func_call.count; i++)
		{
			ptcl_parser_variable *variable = &parser->variables.items[i + variables_index];
			PTCL_FLAGS_REMOVE(variable->flags, PTCL_PARSER_VAR_BUILT_IN_FLAG);
		}

		ptcl_statement_func_call_destroy(&func_call);
		return false;
	}

	// TODO: make func for root
	ptcl_statement *statement = ptcl_statement_create(
		type,
		ptcl_stat_mods_is_flags_global(provider->modifiers) ? NULL : ptcl_parser_root(parser),
		&provider->attributes,
		location);
	if (statement == NULL)
	{
		// Storaged
	cleanup2:
		switch (type)
		{
		case ptcl_statement_func_decl_type:
			ptcl_parser_function_destroy(&parser->functions.items[--parser->functions.count]);
			break;
		case ptcl_statement_typedata_decl_type:
			ptcl_parser_typedata_destroy(&parser->typedatas.items[--parser->typedatas.count]);
			break;
		case ptcl_statement_type_decl_type:
			ptcl_parser_comp_type_destroy(&parser->comp_types.items[--parser->comp_types.count]);
			break;
		case ptcl_statement_assign_type:
			ptcl_parser_variable_destroy(&parser->variables.items[--parser->variables.count]);
			break;
		default:
			return false;
		}

		switch (type)
		{
		case ptcl_statement_func_decl_type:
			ptcl_statement_func_decl_destroy(&func_decl);
			break;
		case ptcl_statement_typedata_decl_type:
			ptcl_statement_typedata_decl_destroy(&typedata_decl);
			break;
		case ptcl_statement_type_decl_type:
			ptcl_statement_type_decl_destroy(&type_decl);
			break;
		case ptcl_statement_assign_type:
			ptcl_statement_assign_destroy(&assign_decl);
			break;
		default:
			return false;
		}

		goto cleanup;
	}

	// Cuz it's last parsed
	switch (type)
	{
	case ptcl_statement_func_decl_type:
		index = parser->functions.count - 1;
		break;
	case ptcl_statement_typedata_decl_type:
		index = parser->typedatas.count - 1;
		break;
	case ptcl_statement_type_decl_type:
		index = parser->comp_types.count - 1;
		break;
	case ptcl_statement_assign_type:
		index = parser->variables.count - 1;
		break;
	default:
		return false;
	}

	ptcl_parser_provider_variant variant = ptcl_parser_provider_variant_create(func_call.arguments, func_call.count, index);
	if (provider->is_provider)
	{
		if (!ptcl_parser_provider_add_variant(provider, &variant))
		{
			// Storaged
			ptcl_statement_destroy(statement);
			goto cleanup2;
		}
	}
	else 
	{
		if (!ptcl_parser_provider_add_generator(provider, &variant))
		{
			// Storaged
			ptcl_statement_destroy(statement);
			goto cleanup2;
		}
	}

	switch (type)
	{
	case ptcl_statement_func_decl_type:
		statement->func_decl = func_decl;
		break;
	case ptcl_statement_typedata_decl_type:
		statement->typedata_decl = typedata_decl;
		break;
	case ptcl_statement_type_decl_type:
		statement->type_decl = type_decl;
		break;
	case ptcl_statement_assign_type:
		statement->assign = assign_decl;
		break;
	default:
		return false;
	}

	if (!ptcl_parser_add_statement_to_root(parser, statement, location))
	{
		// Free calls on variant.values anyway even if no members
		variant.count--;
		// Arguments values will be destroyed on parsing result destroying
		ptcl_statement_destroy(statement);
		goto cleanup2;
	}

	for (size_t i = 0; i < func_call.count; i++)
	{
		ptcl_parser_variable *variable = &parser->variables.items[i + variables_index];
		PTCL_FLAGS_REMOVE(variable->flags, PTCL_PARSER_VAR_BUILT_IN_FLAG);
	}

result_by_index:
	switch (type)
	{
	case ptcl_statement_func_decl_type:
		*result = &parser->functions.items[index];
		break;
	case ptcl_statement_typedata_decl_type:
		*result = parser->typedatas.items[index].typedata;
		break;
	case ptcl_statement_type_decl_type:
		*result = &parser->comp_types.items[index];
		break;
	case ptcl_statement_assign_type:
		*result = &parser->variables.items[index];
		break;
	default:
		return false;
	}

	return true;
}

static bool ptcl_parser_get_instance(
	ptcl_parser *parser,
	ptcl_statement_type type,
	ptrdiff_t provider_index,
	void **result,
	bool is_static,
	ptcl_location location)
{
	if (provider_index >= 0)
	{
		return ptcl_parser_try_generate_or_get_provider(parser, type, provider_index, result, is_static, location);
	}

	return false;
}

bool ptcl_parser_type(
	ptcl_parser *parser,
	bool with_word,
	bool with_any,
	bool with_static,
	ptcl_type *const out)
{
	PTCL_VALIDATE_COND(out != NULL);
	const bool with_syntax = ptcl_parser_parse_try_syntax_usage_here(parser, false);
	size_t depth = parser->state.syntax_depth;
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	const bool is_succesful = ptcl_parser_extra_body(parser, with_syntax);
	if (!is_succesful)
	{
		if (ptcl_parser_critical(parser))
		{
			if (with_syntax && parser->state.syntax_depth == depth)
			{
				ptcl_parser_leave_from_syntax(parser);
			}

			return NULL;
		}
	}

	bool is_const = ptcl_parser_match(parser, ptcl_token_const_type);
	bool is_static = ptcl_parser_match(parser, ptcl_token_static_type);
	if (is_static && !with_static)
	{
		ptcl_parser_throw_not_allowed_token(
			parser,
			ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_const_type),
			ptcl_parser_current(parser)->location);
		if (with_syntax && parser->state.syntax_depth == depth)
		{
			ptcl_parser_leave_from_syntax(parser);
		}

		return false;
	}

	ptcl_token *current = ptcl_parser_current(parser);
	ptcl_parser_skip(parser);

	ptcl_type target = {0};
	bool parse_success = true;

	switch (current->type)
	{
	case ptcl_token_word_word_type:
		if (with_word)
		{
			target = ptcl_type_word;
			break;
		}

		ptcl_parser_throw_except_type_specifier(parser, current->location);
		break;
	case ptcl_token_character_word_type:
		target = ptcl_type_character;
		break;
	case ptcl_token_type_type:
		target = ptcl_type_any_type;
		break;
	case ptcl_token_double_type:
		target = ptcl_type_double;
		break;
	case ptcl_token_float_type:
		target = ptcl_type_float;
		break;
	case ptcl_token_integer_type:
		target = ptcl_type_integer;
		break;
	case ptcl_token_void_type:
		target = ptcl_type_void;
		break;
	case ptcl_token_pointer_type:
		target = ptcl_type_any_pointer;
		break;
	case ptcl_token_left_par_type:
	{
		ptcl_parse_arguments_result arguments = ptcl_parser_parse_arguments(parser, current->location, false, ptcl_parser_args_func_pointer_type);
		if (ptcl_parser_critical(parser))
		{
			return false;
		}

		if (ptcl_parser_not(parser, ptcl_token_colon_type))
		{
			ptcl_parse_arguments_result_destroy(&arguments);
			return false;
		}

		ptcl_type *return_type = malloc(sizeof(ptcl_type));
		if (return_type == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, current->location);
			ptcl_parse_arguments_result_destroy(&arguments);
			return false;
		}

		if (!ptcl_parser_type(parser, false, false, is_static, return_type))
		{
			ptcl_parse_arguments_result_destroy(&arguments);
			free(return_type);
			return false;
		}

		ptcl_type function = ptcl_type_create_func(return_type, NULL, 0, false, arguments.has_variadic);
		function.function_pointer.arguments = arguments.arguments;
		function.function_pointer.count = arguments.count;
		target = function;
		target.is_static = is_static;
		break;
	}
	case ptcl_token_any_type:
		if (is_static || with_any)
		{
			target = ptcl_type_any;
			break;
		}
		else
		{
			parse_success = false;
			ptcl_parser_throw_unknown_type(parser, current->value, current->location);
			break;
		}
	case ptcl_token_exclamation_mark_type:
		ptcl_parser_back(parser);
		ptcl_name name = ptcl_parser_name_word_or_type(parser);
		if (ptcl_parser_critical(parser))
		{
			break;
		}
	case ptcl_token_word_type:
		// TODO: search type by name in one loop
		if (current->type == ptcl_token_word_type)
		{
			name = ptcl_name_create_borrowed(current->value, false);
		}

		ptcl_parser_typedata *typedata;
		ptcl_parser_comp_type *comp_type;
		ptcl_parser_variable *variable;
		if (ptcl_parser_try_get_typedata(parser, &name, &typedata))
		{
			ptcl_name_destroy(&name);
			ptcl_type_typedata *result;
			if (ptcl_parser_get_instance(parser, ptcl_statement_typedata_decl_type, typedata->provider_index, &result, is_static, current->location))
			{
				target = ptcl_type_create_typedata(result, is_const);
			}
			else
			{
				if (ptcl_parser_critical(parser))
				{
					parse_success = false;
				}
				else
				{
					target = ptcl_type_create_typedata(typedata->typedata, is_const);
				}
			}

			break;
		}
		else if (ptcl_parser_try_get_comp_type(parser, &name, is_static, &comp_type))
		{
			ptcl_name_destroy(&name);
			if (!ptcl_parser_get_instance(parser, ptcl_statement_type_decl_type, comp_type->provider_index, &comp_type, is_static, current->location))
			{
				if (ptcl_parser_critical(parser))
				{
					parse_success = false;
					break;
				}
			}

			if (!ptcl_parser_get_comp_type(comp_type, &target, &is_static, with_static, is_const))
			{
				ptcl_parser_throw_except_type_specifier(parser, current->location);
				parse_success = false;
				break;
			}

			break;
		}

		if (current->type == ptcl_token_exclamation_mark_type)
		{
			if (ptcl_parser_try_get_variable(parser, &name, &variable))
			{
				if (variable->type.type == ptcl_value_object_type_type)
				{
					ptcl_name_destroy(&name);
					bool is_out_of_memory = false;
					target = ptcl_type_copy(&variable->built_in->object_type.type, &is_out_of_memory);
					if (is_out_of_memory)
					{
						ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
						parse_success = false;
					}

					break;
				}
			}
		}

		ptcl_name_destroy(&name);
	default:
		parse_success = false;
		ptcl_parser_throw_unknown_type(parser, current->value, current->location);
		break;
	}

	if (is_succesful)
	{
		ptcl_parser_leave_from_extra_body(parser);
	}

	if (with_syntax && parser->state.syntax_depth == depth)
	{
		ptcl_parser_leave_from_syntax(parser);
	}

	if (!parse_success)
	{
		return false;
	}

	if (is_static)
	{
		target.is_static = true;
	}

	if (is_const)
	{
		target.is_const = true;
	}

	if (target.type != ptcl_value_word_type)
	{
		if (!ptcl_parser_pointers(parser, &target))
		{
			return false;
		}
	}
	else
	{
		if (!is_static)
		{
			ptcl_parser_throw_must_be_static(parser, current->location);
			target.is_static = true;
		}
	}

	*out = target;
	return true;
}

static ptcl_type *ptcl_parser_register_function(
	ptcl_parser *parser,
	ptcl_statement_func_decl *const func_decl,
	ptcl_parser_function *prototype_function)
{
	ptcl_assert(func_decl != NULL);
	const ptcl_stat_mods modifiers = func_decl->modifiers;
	const ptcl_location location = ptcl_parser_current(parser)->location;
	ptcl_parser_function function = ptcl_parser_function_create(
		ptcl_stat_mods_is_flags_global(modifiers) ? NULL : ptcl_parser_root(parser),
		func_decl);
	ptcl_type *variable_return_type = NULL;
	if (!ptcl_parser_in_type(parser))
	{
		if (prototype_function != NULL)
		{
			// TODO: make arguments checks from prototype
			prototype_function->func = *func_decl;
		}
		else
		{
			bool added = true;
			if (!func_decl->return_type.is_static)
			{
				ptcl_name variable_name = function.identifier;
				ptcl_name_make_immutable(&variable_name);

				ptcl_type *variable_return_type = malloc(sizeof(ptcl_type));
				if (variable_return_type == NULL)
				{
					ptcl_statement_func_decl_destroy(func_decl);
					ptcl_parser_throw_out_of_memory(parser, location);
					return NULL;
				}

				*variable_return_type = func_decl->return_type;
				ptcl_type variable_type = ptcl_type_create_func(variable_return_type, func_decl->arguments, func_decl->count, true, func_decl->is_variadic);
				variable_type.is_static = true;
				ptcl_parser_variable function_pointer = ptcl_parser_func_variable_create(&variable_name, &variable_type, function.root);
				added = ptcl_parser_add_instance_variable(parser, &function_pointer);
			}

			if (!added || !ptcl_parser_add_instance_function(parser, &function))
			{
				ptcl_statement_func_decl_destroy(func_decl);
				free(variable_return_type);
				ptcl_parser_throw_out_of_memory(parser, location);
				return NULL;
			}
		}
	}

	return variable_return_type;
}

static bool ptcl_parser_parse_dynamic_body(
	ptcl_parser *parser,
	ptcl_statement_func_decl *func_decl,
	ptcl_type *return_type,
	const size_t variable_index,
	const size_t function_index)
{
	ptcl_location location = ptcl_parser_current(parser)->location;
	func_decl->func_body = malloc(sizeof(ptcl_func_body));
	if (func_decl->func_body == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
	cleanup:
		free(func_decl->func_body);
		ptcl_statement_func_decl_destroy(func_decl);
		free(return_type);
		if (!ptcl_parser_in_type(parser))
		{
			parser->variables.items[variable_index].is_out_of_scope = true;
			parser->functions.items[function_index].is_out_of_scope = true;
		}

		return false;
	}

	*func_decl->func_body = ptcl_func_body_create(NULL, 0, ptcl_parser_root(parser));
	const ptcl_type *previous_type = parser->temp.return_type;
	parser->temp.return_type = return_type;
	for (size_t i = 0; i < func_decl->count; i++)
	{
		ptcl_argument *argument = &func_decl->arguments[i];
		ptcl_parser_variable variable = ptcl_parser_variable_create(&argument->name, &argument->type, NULL, false, func_decl->func_body);
		variable.type.is_static = variable.type.is_static;
		if (!ptcl_parser_add_instance_variable(parser, &variable))
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			goto cleanup;
		}
	}

	const bool is_static = ptcl_stat_mods_is_flags_static(func_decl->modifiers);
	const bool last_state = ptcl_parser_in_type(parser);
	ptcl_parser_disable_state(parser, PTCL_PARSER_IN_TYPE_FLAG);
	ptcl_parser_func_body_by_pointer(parser, func_decl->func_body, true, true, is_static);

	ptcl_parser_set_state(parser, PTCL_PARSER_IN_TYPE_FLAG, last_state);
	if (ptcl_parser_critical(parser))
	{
		parser->temp.return_type = previous_type;
		goto cleanup;
	}

	PTCL_FLAGS_REMOVE(func_decl->modifiers, ptcl_stat_mods_prototype_flag);
	if (!ptcl_parser_in_type(parser))
	{
		ptcl_parser_function *original = &parser->functions.items[function_index];
		original->func = *func_decl;
	}

	parser->temp.return_type = previous_type;
	return true;
}

static bool ptcl_parser_parse_static_body(
	ptcl_parser *parser,
	ptcl_statement_func_decl *func_decl,
	size_t function_identifier)
{
	size_t start = ptcl_parser_position(parser);
	ptcl_parser_skip(parser);
	ptcl_parser_skip_block_or_expression(parser);
	if (ptcl_parser_critical(parser))
	{
	cleanup:
		ptcl_statement_func_decl_destroy(func_decl);
		if (!ptcl_parser_in_type(parser))
		{
			parser->functions.items[function_identifier].is_out_of_scope = true;
		}

		return false;
	}

	const ptcl_location location = ptcl_parser_current(parser)->location;
	// With curly
	const size_t tokens_count = (ptcl_parser_position(parser) - start);
	const size_t index = ptcl_parser_add_lated_body(parser, start, tokens_count, false, location);
	if (ptcl_parser_critical(parser))
	{
		goto cleanup;
	}

	func_decl->index = index;
	if (!ptcl_parser_in_type(parser))
	{
		ptcl_parser_function *original = &parser->functions.items[function_identifier];
		original->func = *func_decl;
	}

	return true;
}

static void ptcl_parser_func_decl_set_from_args(
	ptcl_statement_func_decl *func_decl,
	const ptcl_parse_arguments_result *const arguments)
{
	func_decl->arguments = arguments->arguments;
	func_decl->count = arguments->count;
	func_decl->is_variadic = arguments->has_variadic;
	func_decl->with_self = arguments->with_self;
	func_decl->is_self_const = arguments->is_const;
}

bool ptcl_parser_func_name(
	ptcl_parser *parser,
	ptcl_name *name,
	bool *is_provider,
	bool *is_constructor,
	ptcl_parser_function **prototype,
	ptcl_attributes *const attributes,
	ptcl_stat_mods modifiers)
{
	PTCL_VALIDATE_COND(attributes != NULL);
	ptcl_parser_match(parser, ptcl_token_function_type);

	ptcl_token *current = ptcl_parser_current(parser);
	ptcl_location *location = &current->location;
	*is_constructor = current->type == ptcl_token_left_par_type;

	*name = ptcl_name_empty;
	if (!*is_constructor)
	{
		*name = ptcl_parser_name_word(parser);
		if (ptcl_parser_critical(parser))
		{
			return false;
		}

		const bool provider = ptcl_parser_try_provider_name(parser, name, ptcl_statement_func_decl_type, attributes, modifiers);
		if (ptcl_parser_critical(parser))
		{
			return false;
		}

		if (provider)
		{
			ptcl_parser_function instance = ptcl_parser_function_create_provider(
				ptcl_stat_mods_is_flags_global(modifiers) ? NULL : ptcl_parser_root(parser), name, parser->providers.count - 1);
			if (!ptcl_parser_add_instance_function(parser, &instance))
			{
				ptcl_parser_provider_destroy(&parser->providers.items[--parser->providers.count]);
				ptcl_name_destroy(name);
				return false;
			}

			*is_provider = true;
			return true;
		}
	}
	else
	{
		if (!ptcl_parser_in_type(parser))
		{
			ptcl_parser_except(parser, ptcl_token_word_type);
			return false;
		}
	}

	if (!is_constructor &&
		ptcl_parser_try_get_function(parser, name, prototype) &&
		!ptcl_stat_mods_is_flags_prototype((*prototype)->func.modifiers))
	{
		ptcl_parser_throw_redefination(parser, ptcl_name_value(&name), *location);
	}

	return true;
}

bool ptcl_parser_func_body_and_storage(
	ptcl_parser *parser,
	ptcl_name *name,
	ptcl_statement_func_decl *decl,
	ptcl_parser_function *prototype,
	bool is_constructor,
	ptcl_stat_mods modifiers)
{
	PTCL_VALIDATE_COND(name != NULL);
	if (ptcl_parser_not(parser, ptcl_token_left_par_type))
	{
		ptcl_name_destroy(name);
		return false;
	}

	// We set prototype, because on freeing we dont have body
	*decl = ptcl_statement_func_decl_create(
		modifiers | ptcl_stat_mods_prototype_flag,
		parser->temp.root,
		name,
		NULL,
		0,
		NULL,
		ptcl_type_integer,
		false,
		is_constructor);
	ptcl_parse_arguments_result arguments = ptcl_parser_parse_arguments(
		parser,
		ptcl_parser_current(parser)->location,
		!ptcl_stat_mods_is_flags_global(modifiers) && ptcl_parser_in_type(parser),
		ptcl_parser_args_func_type);
	if (ptcl_parser_critical(parser))
	{
		ptcl_name_destroy(name);
		free(decl->func_body);
		return false;
	}

	if (ptcl_parser_not(parser, ptcl_token_colon_type))
	{
	prototype_cleanup:
		ptcl_parse_arguments_result_destroy(&arguments);
		ptcl_name_destroy(name);
		free(decl->func_body);
		return false;
	}

	ptcl_type return_type;
	if (!ptcl_parser_type(parser, true, true, true, &return_type))
	{
		goto prototype_cleanup;
	}

	decl->return_type = return_type;
	ptcl_parser_func_decl_set_from_args(decl, &arguments);

	const size_t variable_identifier = parser->variables.count;
	const size_t function_identifier = parser->functions.count;
	ptcl_type *variable_return_type = ptcl_parser_register_function(parser, decl, prototype);
	if (ptcl_parser_critical(parser))
	{
		// Already destroyed above
		return false;
	}

	if (!ptcl_stat_mods_is_flags_prototype(modifiers))
	{
		if (!decl->return_type.is_static)
		{
			if (!ptcl_parser_parse_dynamic_body(parser, decl, variable_return_type, variable_identifier, function_identifier))
			{
				// Already destroyed above
				return false;
			}
		}
		else
		{
			if (!ptcl_parser_parse_static_body(parser, decl, function_identifier))
			{
				// Already destroyed above
				return false;
			}
		}
	}

	return true;
}

bool ptcl_parser_func_decl(
	ptcl_parser *parser,
	bool *is_provider,
	ptcl_attributes *const attributes,
	ptcl_stat_mods modifiers,
	ptcl_statement_func_decl *const out)
{
	PTCL_VALIDATE_COND(attributes);
	ptcl_name name;
	bool is_constructor;
	ptcl_parser_function *prototype = NULL;
	if (!ptcl_parser_func_name(parser, &name, is_provider, &is_constructor, &prototype, attributes, modifiers))
	{
		return false;
	}

	if (*is_provider)
	{
		return true;
	}

	if (!ptcl_parser_func_body_and_storage(parser, &name, out, prototype, is_constructor, modifiers))
	{
		return false;
	}

	return true;
}

bool ptcl_parser_typedata_name(
	ptcl_parser *parser,
	ptcl_name *name,
	bool *is_provider,
	ptcl_attributes *const attributes,
	ptcl_stat_mods modifiers)
{
	PTCL_VALIDATE_COND(attributes != NULL);
	ptcl_parser_match(parser, ptcl_token_typedata_type);

	ptcl_location location = ptcl_parser_current(parser)->location;
	*name = ptcl_parser_name_word(parser);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	const bool provider = ptcl_parser_try_provider_name(parser, name, ptcl_statement_typedata_decl_type, attributes, modifiers);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	if (provider)
	{
		ptcl_parser_typedata instance = ptcl_parser_typedata_create_provider(
			ptcl_stat_mods_is_flags_global(modifiers) ? NULL : ptcl_parser_root(parser), name, parser->providers.count - 1);
		if (!ptcl_parser_add_instance_typedata(parser, &instance))
		{
			ptcl_parser_provider_destroy(&parser->providers.items[--parser->providers.count]);
			ptcl_name_destroy(name);
			return false;
		}

		*is_provider = true;
		return true;
	}

	if (ptcl_parser_is_typedata_defined(parser, name))
	{
		ptcl_parser_throw_redefination(parser, ptcl_name_value(&name), location);
	}

	return true;
}

bool ptcl_parser_typedata_body_and_storage(
	ptcl_parser *parser,
	ptcl_name *const name,
	ptcl_statement_typedata_decl *decl,
	ptcl_stat_mods modifiers)
{
	PTCL_VALIDATE_COND(name != NULL);
	ptcl_location location = ptcl_parser_current(parser)->location;
	*decl = ptcl_statement_typedata_decl_create(modifiers, name, NULL, 0, true, true);
	if (ptcl_stat_mods_is_flags_prototype(decl->modifiers))
	{
		return true;
	}

	if (ptcl_parser_not(parser, ptcl_token_left_par_type))
	{
		ptcl_statement_typedata_decl_destroy(decl);
		return false;
	}

	decl->is_prototype = false;
	ptcl_parse_arguments_result members = ptcl_parser_parse_arguments(parser, ptcl_parser_current(parser)->location, false, ptcl_parser_args_typedata_type);
	if (ptcl_parser_critical(parser))
	{
		ptcl_statement_typedata_decl_destroy(decl);
		return false;
	}

	decl->members = members.arguments;
	decl->count = members.count;
	decl->is_static = !members.has_unstatic_member;

	ptcl_type_typedata *typedata = malloc(sizeof(ptcl_type_typedata));
	if (typedata == NULL)
	{
		ptcl_statement_typedata_decl_destroy(decl);
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	*typedata = ptcl_type_typedata_create(name, decl->members, decl->count, !members.has_unstatic_member);
	ptcl_parser_typedata instance = ptcl_parser_typedata_create(
		ptcl_stat_mods_is_flags_global(decl->modifiers) ? NULL : ptcl_parser_root(parser),
		typedata);
	if (!ptcl_parser_add_instance_typedata(parser, &instance))
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		ptcl_statement_typedata_decl_destroy(decl);
		free(typedata);
		return false;
	}

	return true;
}

bool ptcl_parser_typedata_decl(
	ptcl_parser *parser,
	bool *is_provider,
	ptcl_attributes *const attributes,
	ptcl_stat_mods modifiers,
	ptcl_statement_typedata_decl *const out)
{
	PTCL_VALIDATE_COND(attributes != NULL);
	ptcl_name name;
	if (!ptcl_parser_typedata_name(parser, &name, is_provider, attributes, modifiers))
	{
		return false;
	}

	if (*is_provider)
	{
		return true;
	}

	if (!ptcl_parser_typedata_body_and_storage(parser, &name, out, modifiers))
	{
		return false;
	}

	return true;
}

static bool ptcl_parser_type_decl_types(
	ptcl_parser *parser,
	ptcl_statement_type_decl *decl,
	ptcl_location location)
{
	while (true)
	{
		ptcl_type_member *buffer = realloc(decl->types, (decl->types_count + 1) * sizeof(ptcl_type_member));
		if (buffer == NULL)
		{
			ptcl_statement_type_decl_destroy(decl);
			ptcl_parser_throw_out_of_memory(parser, location);
			return false;
		}

		decl->types = buffer;
		bool is_up = ptcl_parser_match(parser, ptcl_token_up_type);
		ptcl_type type;
		if (!ptcl_parser_type(parser, false, false, true, &type))
		{
			ptcl_statement_type_decl_destroy(decl);
			return false;
		}

		if (decl->types_count > 0)
		{
			ptcl_type_member *expected = &decl->types[decl->types_count - 1];
			if (!ptcl_type_is_castable(&expected->type, &type))
			{
				ptcl_parser_throw_fast_incorrect_type(parser, &expected->type, &type, location);
				ptcl_statement_type_decl_destroy(decl);
				return false;
			}
		}

		decl->types[decl->types_count++] = ptcl_type_member_create(&type, is_up);
		if (!ptcl_parser_match(parser, ptcl_token_comma_type))
		{
			break;
		}
	}

	return true;
}

static bool ptcl_parser_type_decl_body(
	ptcl_parser *parser,
	ptcl_statement_type_decl *decl,
	ptcl_type_comp_type *comp_type,
	ptcl_location location)
{
	ptcl_func_body *body = malloc(sizeof(ptcl_func_body));
	if (body == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		ptcl_statement_type_decl_destroy(decl);
		return false;
	}

	*body = ptcl_func_body_create(NULL, 0, ptcl_parser_root(parser));
	comp_type->functions = body;
	ptcl_type this_type = ptcl_type_create_comp_type_t(comp_type, false);
	ptcl_parser_variable this_var = ptcl_parser_variable_not_static_create(
		&ptcl_name_this,
		&this_type,
		NULL,
		false,
		body);
	this_var.type.is_static = this_var.type.is_static;
	this_var.type.is_static = false;
	ptcl_parser_variable self_var = ptcl_parser_variable_not_static_create(
		&ptcl_name_self,
		&decl->types[0].type,
		NULL,
		false,
		body);
	self_var.type.is_static = self_var.type.is_static;
	if (!ptcl_parser_add_instance_variable(parser, &this_var) || !ptcl_parser_add_instance_variable(parser, &self_var))
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		ptcl_statement_type_decl_destroy(decl);
		ptcl_func_body_destroy(body);
		free(body);
		return false;
	}

	const bool last_state = ptcl_parser_in_type(parser);
	ptcl_parser_enable_state(parser, PTCL_PARSER_IN_TYPE_FLAG);

	ptcl_parser_func_body_by_pointer(parser, body, true, true, false);

	ptcl_parser_set_state(parser, PTCL_PARSER_IN_TYPE_FLAG, last_state);

	if (ptcl_parser_critical(parser))
	{
		free(body);
		ptcl_statement_type_decl_destroy(decl);
		return false;
	}

	decl->body = body;
	decl->functions = comp_type->functions;
	return true;
}

bool ptcl_parser_type_name(
	ptcl_parser *parser,
	ptcl_name *name,
	bool *is_provider,
	ptcl_attributes *const attributes,
	ptcl_stat_mods modifiers)
{
	PTCL_VALIDATE_COND(attributes != NULL);
	ptcl_parser_match(parser, ptcl_token_type_type);
	*name = ptcl_parser_name_word(parser);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	const bool provider = ptcl_parser_try_provider_name(parser, name, ptcl_statement_type_decl_type, attributes, modifiers);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	if (provider)
	{
		ptcl_parser_comp_type instance = ptcl_parser_comp_type_create_provider(
			ptcl_stat_mods_is_flags_global(modifiers) ? NULL : ptcl_parser_root(parser), name, parser->providers.count - 1);
		if (!ptcl_parser_add_instance_comp_type(parser, &instance))
		{
			ptcl_parser_provider_destroy(&parser->providers.items[--parser->providers.count]);
			ptcl_name_destroy(name);
			return false;
		}

		*is_provider = true;
		return true;
	}

	return true;
}

bool ptcl_parser_type_body_and_storage(
	ptcl_parser *parser,
	ptcl_name *const name,
	ptcl_statement_type_decl *decl,
	ptcl_stat_mods modifiers)
{
	PTCL_VALIDATE_COND(name != NULL);
	ptcl_location location = ptcl_parser_current(parser)->location;
	if (ptcl_parser_not(parser, ptcl_token_colon_type))
	{
		ptcl_name_destroy(name);
		return false;
	}

	*decl = ptcl_statement_type_decl_create(modifiers, name, NULL, 0, NULL, 0);
	if (!ptcl_parser_type_decl_types(parser, decl, location))
	{
		return false;
	}

	const bool is_static = decl->types[0].type.is_static;
	if (ptcl_parser_is_comp_type_defined(parser, name, is_static))
	{
		ptcl_parser_throw_redefination(parser, ptcl_name_value(&name), location);
	}

	ptcl_type_comp_type *comp_type = malloc(sizeof(ptcl_type_comp_type));
	if (comp_type == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		ptcl_statement_type_decl_destroy(decl);
		return false;
	}

	*comp_type = ptcl_type_create_comp_type(decl, is_static);
	ptcl_parser_comp_type instance = ptcl_parser_comp_type_create(
		ptcl_stat_mods_is_flags_global(modifiers) ? NULL : ptcl_parser_root(parser),
		name,
		comp_type,
		comp_type->is_static,
		ptcl_stat_mods_is_flags_auto(modifiers) && comp_type->is_static);
	if (!ptcl_parser_add_instance_comp_type(parser, &instance))
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		ptcl_statement_type_decl_destroy(decl);
		return false;
	}

	if (ptcl_parser_match(parser, ptcl_token_left_curly_type))
	{
		if (!ptcl_parser_type_decl_body(parser, decl, comp_type, location))
		{
			return false;
		}
	}

	return true;
}

bool ptcl_parser_type_decl(
	ptcl_parser *parser,
	bool *is_provider,
	ptcl_attributes *const attributes,
	ptcl_stat_mods modifiers,
	ptcl_statement_type_decl *const out)
{
	PTCL_VALIDATE_COND(attributes != NULL);
	ptcl_name name;
	if (!ptcl_parser_type_name(parser, &name, is_provider, attributes, modifiers))
	{
		return false;
	}

	if (*is_provider)
	{
		return true;
	}

	if (!ptcl_parser_type_body_and_storage(parser, &name, out, modifiers))
	{
		return false;
	}

	return true;
}

bool ptcl_parser_assign_name(
	ptcl_parser *parser,
	ptcl_expression *left,
	ptcl_name *const name,
	ptcl_parser_assign_dot_data *dot,
	ptcl_attributes *const attributes,
	ptcl_stat_mods modifiers)
{
	PTCL_VALIDATE_COND(name != NULL && attributes != NULL);
	// Parse left side: name or dot expression
	if (left == NULL)
	{
		*dot->identifier = ptcl_identifier_create_by_name(name);
		const bool provider = ptcl_parser_try_provider_name(parser, name, ptcl_statement_assign_type, attributes, modifiers);
		if (ptcl_parser_critical(parser))
		{
			return false;
		}

		if (provider)
		{
			ptcl_parser_variable instance = ptcl_parser_variable_create_provider(
				ptcl_stat_mods_is_flags_global(modifiers) ? NULL : ptcl_parser_root(parser), name, parser->providers.count - 1);
			if (!ptcl_parser_add_instance_variable(parser, &instance))
			{
				ptcl_parser_provider_destroy(&parser->providers.items[--parser->providers.count]);
				ptcl_attributes_destroy(attributes);
				ptcl_name_destroy(name);
				return false;
			}

			*dot->is_provider = true;
			return true;
		}
	}
	else
	{
		*dot->identifier = ptcl_identifier_create_by_expr(left);
		if (dot->identifier->value->return_type.type == ptcl_value_type_type && dot->identifier->value->return_type.is_static)
		{
			dot->ctor = ptcl_parser_get_ctor_from_dot(parser, dot->identifier->value, true);
			if (!ptcl_type_get_member(dot->ctor->return_type.typedata, &dot->identifier->value->dot.name, &dot->typedata_member, &dot->ctor_member_index))
			{
				ptcl_parser_throw_unknown_member(parser, ptcl_name_value(&name), ptcl_parser_current(parser)->location);
				ptcl_attributes_destroy(attributes);
				ptcl_expression_destroy(dot->identifier->value);
				return false;
			}
		}
	}

	if (ptcl_parser_critical(parser))
	{
		ptcl_attributes_destroy(attributes);
		ptcl_identifier_destroy(dot->identifier);
		return false;
	}

	return true;
}

static bool ptcl_parser_assign_dot_expr(
	ptcl_parser *parser,
	ptcl_identifier *const identifier,
	ptcl_statement_assign *assign,
	ptcl_parser_assign_dot_data dot,
	ptcl_stat_mods modifiers)
{
	ptcl_parser_except(parser, ptcl_token_equals_type);
	if (ptcl_parser_critical(parser))
	{
		ptcl_identifier_destroy(identifier);
		return false;
	}

	ptcl_type expected_type;
	bool has_expected_type = false;
	if (dot.typedata_member != NULL)
	{
		expected_type = dot.typedata_member->type;
		has_expected_type = true;
	}
	else
	{
		if (identifier->value)
		{
			expected_type = identifier->value->return_type;
			has_expected_type = true;
		}
	}

	if (!has_expected_type)
	{
		expected_type = ptcl_type_any;
		expected_type.is_static = false;
		expected_type.is_const = true;
	}

	ptcl_parser_expr_flags flags = ptcl_parser_expr_flags_default(true);
	if (expected_type.type == ptcl_value_word_type)
	{
		PTCL_FLAGS_SET(flags, ptcl_parser_expr_with_word_flag);
	}

	if (!has_expected_type)
	{
		flags |= ptcl_parser_expr_ignore_excepted_flag;
	}

	ptcl_expression *value = ptcl_parser_cast(parser, &expected_type, flags);
	if (ptcl_parser_critical(parser))
	{
		ptcl_identifier_destroy(identifier);
		if (value)
		{
			ptcl_expression_destroy(value);
		}

		return false;
	}

	if (dot.ctor != NULL)
	{
		// TODO: Add references counter to avoid memory leaks
		dot.ctor->ctor.values[dot.ctor_member_index] = value;
		*assign = ptcl_statement_assign_create(
			modifiers, identifier, &expected_type, true, value, false, 0);
	}
	else
	{
		*assign = ptcl_statement_assign_create(
			modifiers, identifier, &expected_type, false, value, false, 0);
	}

	return true;
}

static bool ptcl_parser_can_declare(
	ptcl_parser *parser,
	const ptcl_name *const name)
{
	PTCL_VALIDATE_COND(name != NULL);
	for (size_t j = 0; j < parser->variables.count; j++)
	{
		const size_t index = parser->variables.count - 1 - j;
		ptcl_parser_variable *variable = &parser->variables.items[index];
		if (variable->is_out_of_scope || !ptcl_name_compare(&variable->identifier, name) || !ptcl_func_body_can_access(variable->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		return false;
	}

	return true;
}

bool ptcl_parser_assign_var_and_storage(
	ptcl_parser *parser,
	ptcl_identifier *const identifier,
	ptcl_statement_assign *assign,
	ptcl_stat_mods modifiers,
	bool is_decl_expected,
	ptcl_location location)
{
	PTCL_VALIDATE_COND(identifier != NULL);
	ptcl_name name = identifier->name;
	ptcl_type type;
	bool has_explicit_type = ptcl_parser_match(parser, ptcl_token_colon_type);
	bool is_auto = false;
	bool is_new_variable = true;
	ptcl_parser_variable *existing_variable = NULL;
	if (has_explicit_type)
	{
		if (!ptcl_parser_can_declare(parser, &name))
		{
			ptcl_parser_throw_variable_redefinition(parser, ptcl_name_value(&name), location);
		}

		if (ptcl_parser_current(parser)->type == ptcl_token_equals_type)
		{
			is_auto = true;
			has_explicit_type = false;
		}
		else
		{
			if (!ptcl_parser_type(parser, true, false, true, &type))
			{
				ptcl_identifier_destroy(identifier);
				return false;
			}

			type.is_const = ptcl_stat_mods_is_flags_const(modifiers);
		}
	}
	else
	{
		if (is_decl_expected)
		{
			ptcl_parser_except(parser, ptcl_token_colon_type);
			ptcl_identifier_destroy(identifier);
			return false;
		}

		if (!ptcl_parser_try_get_variable(parser, &name, &existing_variable))
		{
			ptcl_parser_throw_unknown_variable_or_type(parser, ptcl_name_value(&name), location);
			ptcl_identifier_destroy(identifier);
			return false;
		}

		has_explicit_type = true;
		is_new_variable = false;
		type = existing_variable->type;
	}

	if (ptcl_parser_not(parser, ptcl_token_equals_type))
	{
		if (has_explicit_type)
		{
			ptcl_type_destroy(&type);
		}

		ptcl_identifier_destroy(identifier);
		return false;
	}

	bool is_static_type = false;
	ptcl_type expected;
	ptcl_parser_expr_flags flags = ptcl_parser_expr_flags_default(true);
	PTCL_FLAGS_REMOVE(flags, ptcl_parser_expr_with_word_flag);
	if (has_explicit_type && !is_auto)
	{
		if (type.is_static)
		{
			if (type.type == ptcl_value_type_type && type.comp_type->invariant != NULL)
			{
				expected = type;
				expected.comp_type = type.comp_type->invariant;
				expected.is_static = false;
			}
			else
			{
				if (type.type == ptcl_value_word_type)
				{
					PTCL_FLAGS_SET(flags, ptcl_parser_expr_with_word_flag);
				}

				expected = type;
			}
		}
		else
		{
			expected = type;
		}

		is_static_type = expected.is_static;
	}
	else
	{
		expected = ptcl_type_any;
		expected.is_static = false;
		expected.is_const = true;
		flags |= ptcl_parser_expr_ignore_excepted_flag;
	}

	ptcl_expression *value = ptcl_parser_cast(parser, &expected, flags);
	if (ptcl_parser_critical(parser))
	{
		if (has_explicit_type)
		{
			ptcl_type_destroy(&type);
		}

		ptcl_identifier_destroy(identifier);
		return false;
	}

	if (has_explicit_type)
	{
		ptcl_parser_set_arrays_length(&type, &value->return_type);
	}
	else
	{
		type = value->return_type;
		if (type.type == ptcl_value_pointer_type && type.pointer.is_null)
		{
			type.pointer.is_any = true;
			type.pointer.is_null = false;
		}
		else if (is_new_variable)
		{
			if (type.type == ptcl_value_typedata_type)
			{
				type.is_static = type.typedata->is_static;
			}
			else if (type.type != ptcl_value_type_type)
			{
				type.is_static = !ptcl_type_is_castable_to_unstatic(&type);
			}
		}
	}

	*assign = ptcl_statement_assign_create(
		modifiers, identifier, &type, has_explicit_type, value, is_new_variable, 0);
	if (identifier->is_name && existing_variable == NULL)
	{
		ptcl_parser_variable variable = ptcl_parser_variable_create(
			&name,
			&type,
			value,
			false,
			ptcl_stat_mods_is_flags_global(modifiers) ? NULL : ptcl_parser_root(parser));
		size_t variable_index = parser->variables.count;
		if (!ptcl_parser_add_instance_variable(parser, &variable))
		{
			ptcl_statement_assign_destroy(assign);
			ptcl_parser_throw_out_of_memory(parser, location);
			return false;
		}

		assign->variable_id = variable_index;
	}
	else if (existing_variable != NULL)
	{
		assign->variable_id = existing_variable - parser->variables.items;
		existing_variable->built_in = value;
		if (is_static_type && !value->return_type.is_static)
		{
			ptcl_parser_make_unstatic(&parser->variables.items[assign->variable_id]);
		}
	}

	return true;
}

bool ptcl_parser_assign(
	ptcl_parser *parser,
	bool *is_provider,
	ptcl_name *const name,
	ptcl_expression *left,
	ptcl_attributes *const attributes,
	ptcl_stat_mods modifiers,
	ptcl_statement_assign *const out)
{
	PTCL_VALIDATE_COND(attributes != NULL && out != NULL);
	ptcl_identifier identifier;
	ptcl_parser_assign_dot_data dot = {
		.is_provider = is_provider,
		.identifier = &identifier};
	if (!ptcl_parser_assign_name(parser, left, name, &dot, attributes, modifiers))
	{
		return false;
	}

	if (*is_provider)
	{
		return true;
	}

	ptcl_location location = ptcl_parser_current(parser)->location;
	if (identifier.is_name)
	{
		if (!ptcl_parser_assign_var_and_storage(parser, &identifier, out, false, modifiers, location))
		{
			return false;
		}
	}
	else
	{
		if (!ptcl_parser_assign_dot_expr(parser, &identifier, out, dot, modifiers))
		{
			return false;
		}
	}

	return true;
}

bool ptcl_parser_return(
	ptcl_parser *parser,
	ptcl_statement_return *const out)
{
	ptcl_parser_match(parser, ptcl_token_return_type);
	if (parser->temp.return_type != NULL)
	{
		if (parser->temp.return_type->type == ptcl_value_void_type)
		{
			ptcl_parser_match(parser, ptcl_token_none_type);
			*out = ptcl_statement_return_create(NULL);
			return true;
		}
	}

	const bool last_state = ptcl_parser_has_state(parser, PTCL_PARSER_IN_RETURN_FLAG);
	ptcl_parser_enable_state(parser, PTCL_PARSER_IN_RETURN_FLAG);
	ptcl_expression *value = ptcl_parser_cast(parser, parser->temp.return_type, ptcl_parser_expr_flags_default(true));

	ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, last_state);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	if (value->return_type.type == ptcl_value_void_type)
	{
		ptcl_parser_throw_except_type_specifier(parser, ptcl_parser_current(parser)->location);
		return false;
	}

	*out = ptcl_statement_return_create(value);
	return true;
}

static bool ptcl_parser_static_if_body(
	ptcl_parser *parser,
	ptcl_expression *condition,
	ptcl_statement *out)
{
	ptcl_func_body result;
	const bool condition_value = condition->integer_n;
	ptcl_location location = condition->location;
	ptcl_expression_destroy(condition);
	if (condition_value)
	{
		if (!ptcl_parser_func_body(parser, true, false, ptcl_parser_ignore_error(parser), &result))
		{
			return false;
		}

		if (parser->temp.return_value != NULL)
		{
			return false;
		}

		if (ptcl_parser_match(parser, ptcl_token_else_type))
		{
			if (ptcl_parser_not(parser, ptcl_token_left_curly_type))
			{
				ptcl_func_body_destroy(&result);
				return false;
			}

			ptcl_parser_skip_block_or_expression(parser);
			if (ptcl_parser_critical(parser))
			{
				ptcl_func_body_destroy(&result);
				return false;
			}
		}
	}
	else
	{
		ptcl_parser_skip(parser);
		ptcl_parser_skip_block_or_expression(parser);
		if (ptcl_parser_critical(parser))
		{
			return false;
		}

		if (ptcl_parser_match(parser, ptcl_token_else_type))
		{
			if (!ptcl_parser_func_body(parser, true, false, ptcl_parser_ignore_error(parser), &result))
			{
				return false;
			}

			if (parser->temp.return_value != NULL)
			{
				return false;
			}
		}

		ptcl_statement_func_body func_body = ptcl_statement_func_body_create(NULL, 0, ptcl_parser_root(parser));
		ptcl_statement *statement = ptcl_func_body_create_stat(&func_body, func_body.body.root, location);
		if (statement == NULL)
		{
			ptcl_func_body_destroy(&result);
			return false;
		}

		*out = *statement;
		return true;
	}

	ptcl_statement_func_body func_body = ptcl_statement_func_body_create_by_body(&result);
	ptcl_statement *statement = ptcl_func_body_create_stat(&func_body, result.root, location);
	if (statement == NULL)
	{
		ptcl_func_body_destroy(&result);
		return false;
	}

	*out = *statement;
	return true;
}

bool ptcl_parser_if(
	ptcl_parser *parser,
	bool is_static,
	ptcl_statement *const out)
{
	ptcl_parser_match(parser, ptcl_token_if_type);

	ptcl_type type = ptcl_type_integer;
	type.is_static = is_static;
	ptcl_expression *condition = ptcl_parser_cast(parser, &type, ptcl_parser_expr_flags_default(true));
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	if (is_static || condition->return_type.is_static)
	{
		// TODO: roots
		return ptcl_parser_static_if_body(parser, condition, out);
	}

	const bool last_state = ptcl_parser_in_if(parser);
	ptcl_parser_enable_state(parser, PTCL_PARSER_IN_IF_FLAG);

	ptcl_func_body body;
	if (!ptcl_parser_func_body(parser, true, true, ptcl_parser_ignore_error(parser), &body))
	{
		ptcl_parser_set_state(parser, PTCL_PARSER_IN_IF_FLAG, last_state);
		ptcl_expression_destroy(condition);
		return false;
	}

	ptcl_func_body else_body = (ptcl_func_body){0};
	ptcl_statement_if if_stat = ptcl_statement_if_create(condition, &body, false, &else_body);
	if (ptcl_parser_match(parser, ptcl_token_else_type))
	{
		if (!ptcl_parser_func_body(parser, true, true, ptcl_parser_ignore_error(parser), &if_stat.else_body))
		{
			ptcl_parser_set_state(parser, PTCL_PARSER_IN_IF_FLAG, last_state);
			ptcl_statement_if_destroy(&if_stat);
			return false;
		}

		if_stat.with_else = true;
	}

	ptcl_parser_set_state(parser, PTCL_PARSER_IN_IF_FLAG, last_state);
	ptcl_attributes attributes = ptcl_attributes_create(NULL, 0, true);
	ptcl_statement *statement = ptcl_statement_create(ptcl_statement_if_type, ptcl_parser_root(parser), &attributes, condition->location);
	if (statement == NULL)
	{
		ptcl_statement_if_destroy(&if_stat);
		ptcl_parser_throw_out_of_memory(parser, condition->location);
		return false;
	}

	statement->if_stat = if_stat;
	*out = *statement;
	return true;
}

static ptcl_expression *ptcl_parser_static_if_expr_body(
	ptcl_parser *parser,
	ptcl_expression *condition)
{
	ptcl_expression *result;

	int condition_value = condition->integer_n;
	ptcl_expression_destroy(condition);
	if (condition_value)
	{
		result = ptcl_parser_cast(parser, NULL, false);
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}

		if (ptcl_parser_not(parser, ptcl_token_right_curly_type) ||
			ptcl_parser_not(parser, ptcl_token_else_type) ||
			ptcl_parser_not(parser, ptcl_token_left_curly_type))
		{
			ptcl_expression_destroy(result);
			return NULL;
		}

		ptcl_parser_skip_block_or_expression(parser);
		if (ptcl_parser_critical(parser))
		{
			ptcl_expression_destroy(result);
			return NULL;
		}
	}
	else
	{
		ptcl_parser_skip_block_or_expression(parser);
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}

		if (ptcl_parser_not(parser, ptcl_token_else_type) || ptcl_parser_not(parser, ptcl_token_left_curly_type))
		{
			return NULL;
		}

		result = ptcl_parser_cast(parser, NULL, false);
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}

		if (ptcl_parser_not(parser, ptcl_token_right_curly_type))
		{
			ptcl_expression_destroy(result);
			return NULL;
		}
	}

	return result;
}

bool ptcl_parser_if_expression(
	ptcl_parser *parser,
	bool is_static,
	ptcl_expression **const out)
{
	ptcl_parser_match(parser, ptcl_token_if_type);
	ptcl_type type = ptcl_type_integer;
	type.is_static = is_static;

	ptcl_expression *condition = ptcl_parser_cast(parser, &type, false);
	if (ptcl_parser_critical(parser))
	{
		*out = NULL;
		return false;
	}

	if (ptcl_parser_not(parser, ptcl_token_left_curly_type))
	{
		*out = NULL;
		return false;
	}

	if (condition->return_type.is_static || is_static)
	{
		return ptcl_parser_static_if_expr_body(parser, condition);
	}

	const bool last_state = ptcl_parser_in_if(parser);
	ptcl_parser_enable_state(parser, PTCL_PARSER_IN_IF_FLAG);

	ptcl_expression *body = ptcl_parser_cast(parser, NULL, false);
	if (ptcl_parser_critical(parser))
	{
		ptcl_parser_set_state(parser, PTCL_PARSER_IN_IF_FLAG, last_state);
		ptcl_expression_destroy(condition);
		*out = NULL;
		return false;
	}

	if (ptcl_parser_not(parser, ptcl_token_right_curly_type) ||
		ptcl_parser_not(parser, ptcl_token_else_type) ||
		ptcl_parser_not(parser, ptcl_token_left_curly_type))
	{
		ptcl_parser_set_state(parser, PTCL_PARSER_IN_IF_FLAG, last_state);
		ptcl_expression_destroy(condition);
		ptcl_expression_destroy(body);
		*out = NULL;
		return false;
	}

	ptcl_expression *else_expression = ptcl_parser_cast(parser, &body->return_type, false);
	ptcl_parser_set_state(parser, PTCL_PARSER_IN_IF_FLAG, last_state);
	if (ptcl_parser_critical(parser))
	{
		ptcl_expression_destroy(condition);
		ptcl_expression_destroy(body);
		*out = NULL;
		return false;
	}

	if (ptcl_parser_not(parser, ptcl_token_right_curly_type))
	{
		ptcl_expression_destroy(condition);
		ptcl_expression_destroy(body);
		ptcl_expression_destroy(else_expression);
		*out = NULL;
		return false;
	}

	ptcl_expression_if if_expr = ptcl_expression_if_create(condition, body, else_expression);
	ptcl_expression *expression = ptcl_expression_create(ptcl_expression_if_type, &if_expr.body->return_type, condition->location);
	if (expression == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
		ptcl_expression_if_destroy(&if_expr);
		*out = NULL;
		return false;
	}

	expression->if_expr = if_expr;
	*out = expression;
	return true;
}

bool ptcl_parser_unsyntax(
	ptcl_parser *parser,
	ptcl_statement_func_body *const out)
{
	ptcl_parser_match(parser, ptcl_token_unsyntax_type);
	ptcl_parser_disable_state(parser, PTCL_PARSER_IN_SYNTAX_FLAG);

	ptcl_func_body body;
	const bool result = ptcl_parser_func_body(parser, true, false, ptcl_parser_ignore_error(parser), &body);

	ptcl_parser_enable_state(parser, PTCL_PARSER_IN_SYNTAX_FLAG);
	*out = ptcl_statement_func_body_create_by_body(&body);
	return result;
}

static bool ptcl_parser_syntax_try_this(
	ptcl_parser *parser,
	ptcl_parser_syntax *const syntax,
	ptcl_token *const next,
	ptcl_token *const current,
	ptcl_parser_syntax_node *node)
{
	PTCL_VALIDATE_COND(syntax != NULL && next != NULL && current != NULL);
	if (strcmp(next->value, "this") == 0)
	{
		if (syntax->count != 0)
		{
			ptcl_parser_throw_not_allowed_token(parser, current->value, current->location);
			ptcl_parser_syntax_destroy(syntax);
			return false;
		}

		if (ptcl_parser_not(parser, ptcl_token_right_square_type))
		{
			ptcl_parser_syntax_destroy(syntax);
			return false;
		}

		if (ptcl_parser_not(parser, ptcl_token_left_curly_type))
		{
			ptcl_parser_throw_except_token(parser, ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_curly_type), current->location);
			ptcl_parser_syntax_destroy(syntax);
			return false;
		}

		return true;
	}

	return false;
}

static bool ptcl_parser_syntax_arg(
	ptcl_parser *parser,
	ptcl_parser_syntax *const syntax,
	ptcl_token *const next,
	ptcl_token *const current,
	ptcl_parser_syntax_node *node)
{
	PTCL_VALIDATE_COND(syntax != NULL && next != NULL && current != NULL);
	ptcl_parser_variable *variable = NULL;
	if (ptcl_parser_match(parser, ptcl_token_colon_type))
	{
		if (ptcl_parser_match(parser, ptcl_token_elipsis_type))
		{
			if (ptcl_parser_not(parser, ptcl_token_right_square_type))
			{
				ptcl_parser_syntax_destroy(syntax);
				return false;
			}

			ptcl_token *end_token = ptcl_parser_current(parser);
			if (end_token->type == ptcl_token_left_curly_type)
			{
				ptcl_parser_throw_not_allowed_token(parser, current->value, current->location);
				ptcl_parser_syntax_destroy(syntax);
				return false;
			}

			*node = ptcl_parser_syntax_node_create_end_token(next->value, ptcl_get_token_t());
			return true;
		}
		else
		{
			ptcl_type type;
			if (!ptcl_parser_type(parser, true, true, true, &type))
			{
				ptcl_parser_syntax_destroy(syntax);
				return false;
			}

			if (ptcl_parser_not(parser, ptcl_token_right_square_type))
			{
				ptcl_type_destroy(&type);
				ptcl_parser_syntax_destroy(syntax);
				return false;
			}

			*node = ptcl_parser_syntax_node_create_variable(&type, next->value);
			return true;
		}
	}

	ptcl_parser_syntax_destroy(syntax);
	ptcl_parser_throw_not_allowed_token(parser, current->value, current->location);
	return false;
}

static bool ptcl_parser_syntax_expansion(
	ptcl_parser *parser,
	ptcl_parser_syntax *const syntax,
	ptcl_token *const next,
	ptcl_token *const current,
	ptcl_parser_syntax_node *node)
{
	PTCL_VALIDATE_COND(syntax != NULL && next != NULL && current != NULL);
	ptcl_parser_variable *variable;
	size_t start = ptcl_parser_position(parser);
	ptcl_parser_back(parser); // comeback to '!'
	ptcl_name word = ptcl_parser_name_word(parser);
	if (ptcl_parser_critical(parser))
	{
		ptcl_parser_syntax_destroy(syntax);
		return false;
	}

	if (ptcl_parser_try_get_variable(parser, &word, &variable) && variable->type.type == ptcl_value_object_type_type)
	{
		*node = ptcl_parser_syntax_node_create_object_type(ptcl_type_get_target(&variable->type));
		ptcl_name_destroy(&word);
		return true;
	}

	ptcl_name_destroy(&word);
	ptcl_parser_syntax_destroy(syntax);
	ptcl_parser_set_position(parser, start);
	ptcl_parser_throw_not_allowed_token(parser, current->value, current->location);
	return false;
}

static bool ptcl_parser_syntax_token(
	ptcl_parser *parser,
	ptcl_parser_syntax *const syntax,
	ptcl_token *const next,
	ptcl_token *const current,
	ptcl_parser_syntax_node *node)
{
	PTCL_VALIDATE_COND(syntax != NULL && next != NULL && current != NULL);
	if (ptcl_parser_not(parser, ptcl_token_right_square_type))
	{
		ptcl_parser_syntax_destroy(syntax);
		return false;
	}

	ptcl_name name = ptcl_name_create_borrowed(next->value, false);
	*node = ptcl_parser_syntax_node_create_word(current->type, &name);
	return true;
}

static bool ptcl_parser_syntax_format(
	ptcl_parser *parser,
	ptcl_parser_syntax *syntax,
	bool *is_injector,
	ptcl_location location)
{
	ptcl_location last_location = {0};
	size_t depth = 0;
	bool except_end_token = false;
	while (true)
	{
		if (ptcl_parser_ended(parser))
		{
			ptcl_parser_throw_except_token(parser, ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_left_curly_type), location);
			ptcl_parser_syntax_destroy(syntax);
			return false;
		}

		ptcl_token *current = ptcl_parser_current(parser);
		ptcl_parser_skip(parser);
		if (current->type == ptcl_token_left_curly_type)
		{
			break;
		}
		else if (current->type == ptcl_token_right_curly_type)
		{
			ptcl_parser_throw_not_allowed_token(parser, current->value, current->location);
			ptcl_parser_syntax_destroy(syntax);
			return false;
		}

		bool breaked = false;
		ptcl_parser_syntax_node node = {0};
		switch (current->type)
		{
		case ptcl_token_left_square_type:
		{
			ptcl_token *next = ptcl_parser_current(parser);
			ptcl_parser_skip(parser);
			switch (next->type)
			{
			case ptcl_token_word_type:
			{
				if (ptcl_parser_syntax_try_this(parser, syntax, next, current, &node))
				{
					*is_injector = true;
					breaked = true;
					break;
				}
				else if (ptcl_parser_critical(parser) || !ptcl_parser_syntax_arg(parser, syntax, next, current, &node))
				{
					// Already destroyed in 'this' parsing or above
					return false;
				}

				break;
			}
			case ptcl_token_exclamation_mark_type:
			{
				if (!ptcl_parser_syntax_expansion(parser, syntax, next, current, &node))
				{
					// Already destroyed above
					return false;
				}

				break;
			}
			case ptcl_token_left_curly_type:
			{
				if (!ptcl_parser_syntax_token(parser, syntax, next, current, &node))
				{
					// Already destroyed above
					return false;
				}

				depth++;
				last_location = next->location;
				break;
			}
			case ptcl_token_right_curly_type:
			{
				if (depth == 0)
				{
					ptcl_parser_throw_except_token(parser, ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_curly_type), next->location);
					ptcl_parser_syntax_destroy(syntax);
					return false;
				}

				if (!ptcl_parser_syntax_token(parser, syntax, next, current, &node))
				{
					// Already destroyed above
					return false;
				}

				depth--;
				break;
			}
			default:
			{
				ptcl_parser_back(parser);
				ptcl_name name = ptcl_name_create_borrowed(current->value, false);
				node = ptcl_parser_syntax_node_create_word(current->type, &name);
				break;
			}
			}

			break;
		}
		default:
		{
			ptcl_name name = ptcl_name_create_borrowed(current->value, false);
			node = ptcl_parser_syntax_node_create_word(current->type, &name);
			break;
		}
		}

		if (breaked)
		{
			break;
		}

		if (except_end_token)
		{
			if (node.type != ptcl_parser_syntax_node_word_type)
			{
				ptcl_parser_throw_not_allowed_token(parser, current->value, current->location);
				ptcl_parser_syntax_node_destroy(&node);
				ptcl_parser_syntax_destroy(syntax);
				return false;
			}

			except_end_token = false;
		}
		else
		{
			except_end_token = node.type == ptcl_parser_syntax_node_variable_type && node.variable.is_variadic;
		}

		ptcl_parser_syntax_node *buffer = realloc(syntax->nodes, (syntax->count + 1) * sizeof(ptcl_parser_syntax_node));
		if (buffer == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			ptcl_parser_syntax_node_destroy(&node);
			ptcl_parser_syntax_destroy(syntax);
			return false;
		}

		syntax->nodes = buffer;
		syntax->nodes[syntax->count++] = node;
	}

	if (depth != 0)
	{
		ptcl_parser_throw_except_token(parser, ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_right_curly_type), last_location);
		ptcl_parser_syntax_destroy(syntax);
		return false;
	}

	return true;
}

bool ptcl_parser_syntax_decl(
	ptcl_parser *parser)
{
	ptcl_parser_match(parser, ptcl_token_syntax_type);
	ptcl_location location = ptcl_parser_current(parser)->location;
	ptcl_parser_syntax syntax = ptcl_parser_syntax_create(&ptcl_name_empty, ptcl_parser_root(parser), NULL, 0, 0);
	bool is_injector = false;
	if (!ptcl_parser_syntax_format(parser, &syntax, &is_injector, location))
	{
		return false;
	}

	size_t start = ptcl_parser_position(parser);
	ptcl_parser_skip_block_or_expression(parser);
	if (ptcl_parser_critical(parser))
	{
		ptcl_parser_syntax_destroy(&syntax);
		return false;
	}

	size_t tokens_count = (ptcl_parser_position(parser) - start);
	if (tokens_count > 0)
	{
		// -1 because of curly
		tokens_count -= 1;
	}

	size_t index = ptcl_parser_add_lated_body(parser, start, tokens_count, false, location);
	if (ptcl_parser_critical(parser))
	{
		ptcl_parser_syntax_destroy(&syntax);
		return false;
	}

	syntax.index = index;
	if (!is_injector)
	{
		if (!ptcl_parser_add_instance_syntax(parser, &syntax))
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			ptcl_parser_syntax_destroy(&syntax);
			return false;
		}
	}
	else
	{
		ptcl_parser_this_s_pair pair = ptcl_parser_this_s_pair_create(parser->temp.main_root, syntax.index);
		if (!ptcl_parser_add_this_pair(parser, &pair))
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			ptcl_parser_syntax_destroy(&syntax);
			return false;
		}
	}

	return true;
}

bool ptcl_parser_loop(
	ptcl_parser *parser,
	ptcl_statement_loop *const out)
{
	ptcl_parser_match(parser, ptcl_token_each_type);
	ptcl_parser_match(parser, ptcl_token_left_par_type);

	ptcl_location location = ptcl_parser_current(parser)->location;
	ptcl_name name = ptcl_parser_name_word(parser);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	if (ptcl_parser_not(parser, ptcl_token_comma_type))
	{
		return false;
	}

	ptcl_type array = ptcl_type_create_pointer(&ptcl_type_any, true, true, false);
	ptcl_expression *value = ptcl_parser_cast(parser, &array, false);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	ptcl_parser_match(parser, ptcl_token_right_par_type);
	const ptcl_type *return_type = ptcl_type_get_target(&value->return_type);
	ptcl_func_body empty = {
		.statements = NULL,
		.count = 0,
		.root = ptcl_parser_root(parser)};
	size_t position = ptcl_parser_position(parser);
	size_t capacity = 0;
	ptcl_parser_variable variable = ptcl_parser_variable_create(&name, return_type, NULL, false, &empty);
	size_t idenitifer = parser->variables.count;
	if (!ptcl_parser_add_instance_variable(parser, &variable))
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		ptcl_expression_destroy(value);
		ptcl_func_body_destroy(ptcl_parser_root(parser));
		return false;
	}

	for (size_t i = 0; i < value->array.count; i++)
	{
		ptcl_expression *expression = value->array.expressions[i];
		expression->is_original = false;

		parser->variables.items[idenitifer].built_in = expression;

		// TODO: root problem that can be copies of types with same name, and we need to remember about anonymous vars
		ptcl_parser_func_body_by_pointer_cap(parser, &empty, &capacity, true, false, ptcl_parser_ignore_error(parser));
		if (i != value->array.count - 1)
		{
			ptcl_parser_set_position(parser, position);
		}

		expression->is_original = true;
		if (ptcl_parser_critical(parser))
		{
			ptcl_expression_destroy(value);
			return false;
		}
	}

	*out = ptcl_statement_loop_create(value, &empty);
	return true;
}

bool ptcl_parser_undefine(
	ptcl_parser *parser)
{
	ptcl_parser_match(parser, ptcl_token_undefine_type);
	ptcl_parser_match(parser, ptcl_token_left_par_type);
	ptcl_name identifier = ptcl_parser_name_word(parser);
	if (ptcl_parser_critical(parser))
	{
		return false;
	}

	for (size_t j = 0; j < parser->syntaxes.count; j++)
	{
		const size_t index = parser->syntaxes.count - 1 - j;
		ptcl_parser_syntax *instance = &parser->syntaxes.items[index];
		if (!ptcl_name_compare(&instance->identifier, &identifier) || !ptcl_func_body_can_access(instance->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		instance->is_out_of_scope = true;
		goto here;
	}

	for (size_t j = 0; j < parser->comp_types.count; j++)
	{
		const size_t index = parser->comp_types.count - 1 - j;
		ptcl_parser_comp_type *instance = &parser->comp_types.items[index];
		if (!ptcl_name_compare(&instance->identifier, &identifier) || !ptcl_func_body_can_access(instance->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		instance->is_out_of_scope = true;
		goto here;
	}

	for (size_t j = 0; j < parser->typedatas.count; j++)
	{
		const size_t index = parser->typedatas.count - 1 - j;
		ptcl_parser_typedata *instance = &parser->typedatas.items[index];
		if (!ptcl_name_compare(&instance->typedata->identifier, &identifier) || !ptcl_func_body_can_access(instance->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		instance->is_out_of_scope = true;
		goto here;
	}

	for (size_t j = 0; j < parser->variables.count; j++)
	{
		const size_t index = parser->variables.count - 1 - j;
		ptcl_parser_variable *instance = &parser->variables.items[index];
		if (!ptcl_name_compare(&instance->identifier, &identifier) || !ptcl_func_body_can_access(instance->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		instance->is_out_of_scope = true;
		goto here;
	}

here:
	ptcl_parser_match(parser, ptcl_token_right_par_type);
	return true;
}

ptcl_expression *ptcl_parser_cast(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	ptcl_parser_expr_flags flags)
{
	if (ptcl_parser_match(parser, ptcl_token_auto_type))
	{
		ptcl_location location = ptcl_parser_current(parser)->location;
		ptcl_expression *value = ptcl_parser_cast(parser, expected, flags);
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}

		ptcl_expression *cast = ptcl_expression_cast_create(value, expected, false, location);
		if (cast == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			ptcl_expression_destroy(value);
			return NULL;
		}

		return ptcl_expression_static_cast(cast);
	}

	ptcl_expression *left = ptcl_parser_binary(parser, expected, flags);
	if (ptcl_parser_critical(parser))
	{
		return NULL;
	}

	while (true)
	{
		if (ptcl_parser_ended(parser) ||
			ptcl_parser_current(parser)->type != ptcl_token_colon_type ||
			ptcl_parser_peek(parser, 1)->type != ptcl_token_colon_type)
		{
			break;
		}

		ptcl_location location = ptcl_parser_current(parser)->location;
		ptcl_parser_skip(parser);
		ptcl_parser_skip(parser);

		ptcl_type type;
		if (!ptcl_parser_type(parser, true, false, true, &type))
		{
			ptcl_expression_destroy(left);
			return NULL;
		}

		if (type.type == ptcl_value_type_type)
		{
			ptcl_type base = type.comp_type->types[0].type;
			if (!ptcl_type_is_castable(&base, &left->return_type))
			{
				ptcl_parser_throw_fast_incorrect_type(parser, &base, &left->return_type, location);
				ptcl_expression_destroy(left);
				ptcl_type_destroy(&type);
				return NULL;
			}
		}

		ptcl_expression *cast = ptcl_expression_cast_create(left, &type, true, location);
		if (cast == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			ptcl_expression_destroy(left);
			ptcl_type_destroy(&type);
			return NULL;
		}

		left = ptcl_expression_static_cast(cast);
	}

	if (expected != NULL && !ptcl_type_is_castable(expected, &left->return_type))
	{
		ptcl_parser_throw_fast_incorrect_type(parser, expected, &left->return_type, left->location);
		ptcl_expression_destroy(left);
		return NULL;
	}

	return left;
}

ptcl_expression *ptcl_parser_binary(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	ptcl_parser_expr_flags flags)
{
	ptcl_expression *left = ptcl_parser_additive(parser, expected, flags);
	if (ptcl_parser_critical(parser))
	{
		return left;
	}

	while (true)
	{
		if (ptcl_parser_ended(parser))
		{
			break;
		}

		ptcl_binary_operator_type type = ptcl_binary_operator_type_from_token(ptcl_parser_current(parser)->type);
		if (type == ptcl_binary_operator_none_type ||
			type == ptcl_binary_operator_multiplicative_type ||
			type == ptcl_binary_operator_division_type ||
			type == ptcl_binary_operator_plus_type ||
			type == ptcl_binary_operator_minus_type)
		{
			break;
		}

		if (ptcl_parser_peek(parser, 1)->type == ptcl_token_equals_type)
		{
			switch (type)
			{
			case ptcl_binary_operator_equals_type:
				type = ptcl_binary_operator_equals_type;
				break;
			case ptcl_binary_operator_negation_type:
				type = ptcl_binary_operator_not_equals_type;
				break;
			case ptcl_binary_operator_greater_than_type:
				type = ptcl_binary_operator_greater_equals_than_type;
				break;
			case ptcl_binary_operator_less_than_type:
				type = ptcl_binary_operator_less_equals_than_type;
				break;
			default:
				break;
			}

			ptcl_parser_skip(parser);
		}
		else
		{
			if (type == ptcl_binary_operator_equals_type || type == ptcl_binary_operator_negation_type)
			{
				break;
			}
		}

		ptcl_type *expected = &left->return_type;
		if (type == ptcl_binary_operator_type_equals_type)
		{
			*expected = ptcl_type_any_type;
		}

		ptcl_parser_skip(parser);
		const bool last_state = ptcl_parser_has_state(parser, PTCL_PARSER_IN_RETURN_FLAG);
		ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, !left->return_type.is_static);
		ptcl_expression *right = ptcl_parser_cast(parser, NULL, ptcl_parser_expr_flags_default(true));

		ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, last_state);
		if (ptcl_parser_critical(parser))
		{
			ptcl_expression_destroy(left);
			return NULL;
		}

		if (!ptcl_type_is_castable(expected, &right->return_type))
		{
			ptcl_parser_throw_fast_incorrect_type(parser, &left->return_type, &right->return_type, left->location);
			ptcl_expression_destroy(right);
			ptcl_expression_destroy(left);
			return NULL;
		}

		left = ptcl_expression_binary_static_evaluate(type, left, right);
		if (left == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			return left;
		}
	}

	return left;
}

ptcl_expression *ptcl_parser_additive(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	ptcl_parser_expr_flags flags)
{
	ptcl_expression *left = ptcl_parser_multiplicative(parser, expected, flags);
	if (ptcl_parser_critical(parser) || !ptcl_value_is_number(left->return_type.type))
	{
		return left;
	}

	while (true)
	{
		if (ptcl_parser_ended(parser))
		{
			break;
		}

		ptcl_binary_operator_type type = ptcl_binary_operator_type_from_token(ptcl_parser_current(parser)->type);
		if (type != ptcl_binary_operator_plus_type && type != ptcl_binary_operator_minus_type)
		{
			break;
		}

		ptcl_parser_skip(parser);
		const bool last_state = ptcl_parser_has_state(parser, PTCL_PARSER_IN_RETURN_FLAG);
		ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, !left->return_type.is_static);
		ptcl_expression *right = ptcl_parser_multiplicative(parser, NULL, ptcl_parser_expr_flags_default(true));

		ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, last_state);
		if (ptcl_parser_critical(parser))
		{
			ptcl_expression_destroy(left);
			return NULL;
		}

		if (!ptcl_type_is_castable(&left->return_type, &right->return_type))
		{
			ptcl_parser_throw_fast_incorrect_type(parser, &left->return_type, &right->return_type, left->location);
			ptcl_expression_destroy(right);
			ptcl_expression_destroy(left);
			return NULL;
		}

		left = ptcl_expression_binary_static_evaluate(type, left, right);
		if (left == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			return left;
		}
	}

	return left;
}

ptcl_expression *ptcl_parser_multiplicative(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	ptcl_parser_expr_flags flags)
{
	ptcl_expression *left = ptcl_parser_unary(parser, expected, flags);
	if (ptcl_parser_critical(parser) || !ptcl_value_is_number(left->return_type.type))
	{
		return left;
	}

	while (true)
	{
		if (ptcl_parser_ended(parser))
		{
			break;
		}

		ptcl_binary_operator_type type = ptcl_binary_operator_type_from_token(ptcl_parser_current(parser)->type);
		if (type != ptcl_binary_operator_multiplicative_type && type != ptcl_binary_operator_division_type)
		{
			break;
		}

		ptcl_parser_skip(parser);
		const bool last_state = ptcl_parser_has_state(parser, PTCL_PARSER_IN_RETURN_FLAG);
		ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, !left->return_type.is_static);
		ptcl_expression *right = ptcl_parser_unary(parser, NULL, ptcl_parser_expr_flags_default(true));

		ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, last_state);
		if (ptcl_parser_critical(parser))
		{
			ptcl_expression_destroy(left);
			return NULL;
		}

		if (!ptcl_type_is_castable(&left->return_type, &right->return_type))
		{
			ptcl_parser_throw_fast_incorrect_type(parser, &left->return_type, &right->return_type, left->location);
			ptcl_expression_destroy(right);
			ptcl_expression_destroy(left);
			return NULL;
		}

		left = ptcl_expression_binary_static_evaluate(type, left, right);
		if (left == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			return left;
		}
	}

	return left;
}

ptcl_expression *ptcl_parser_unary(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	ptcl_parser_expr_flags flags)
{
	if (ptcl_parser_ended(parser))
	{
		return ptcl_parser_dot(parser, expected, NULL, flags);
	}

	ptcl_binary_operator_type type = ptcl_binary_operator_type_from_token(ptcl_parser_current(parser)->type);
	if (type != ptcl_binary_operator_none_type &&
		type != ptcl_binary_operator_equals_type &&
		type != ptcl_binary_operator_greater_than_type &&
		type != ptcl_binary_operator_less_than_type &&
		type != ptcl_binary_operator_greater_equals_than_type &&
		type != ptcl_binary_operator_less_equals_than_type)
	{
		ptcl_parser_skip(parser);
		ptcl_expression *value = ptcl_parser_cast(parser, NULL, false);
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}

		ptcl_type type_value = value->return_type;
		if (type == ptcl_binary_operator_multiplicative_type)
		{
			if (ptcl_parser_expr_flags_is_only_value(flags))
			{
				return value;
			}

			if ((value->return_type.type != ptcl_value_pointer_type ||
				 value->return_type.pointer.is_any || value->return_type.pointer.is_null) ||
				value->return_type.is_static)
			{
				ptcl_parser_throw_fast_incorrect_type(parser, &ptcl_type_any_pointer, &value->return_type, value->location);
				ptcl_expression_destroy(value);
				return NULL;
			}

			type = ptcl_binary_operator_dereference_type;
			type_value = *ptcl_type_get_target(&value->return_type);
		}
		else if (type == ptcl_binary_operator_reference_type)
		{
			ptcl_type *target = malloc(sizeof(ptcl_type));
			if (target == NULL)
			{
				ptcl_expression_destroy(value);
				ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
				return NULL;
			}

			*target = value->return_type;
			type_value = ptcl_type_create_pointer(target, target->is_const, false, true);
			if (target->type != ptcl_value_pointer_type)
			{
				type_value.pointer.is_const = false;
			}
		}

		if (type == ptcl_binary_operator_reference_type || type == ptcl_binary_operator_dereference_type)
		{
			// TODO: we can get address from cast with rvalue
			bool is_not_variable = value->type != ptcl_expression_variable_type &&
								   value->type != ptcl_expression_cast_type &&
								   (value->type != ptcl_expression_unary_type &&
									value->unary.type != ptcl_binary_operator_reference_type &&
									value->unary.type != ptcl_binary_operator_dereference_type);
			const ptcl_parser_variable *variable = &parser->variables.items[value->variable.variable_id];
			if (is_not_variable || (value->type == ptcl_expression_variable_type && ptcl_parser_var_flags_is_built_in(variable->flags)))
			{
				ptcl_parser_throw_must_be_variable(parser, value->location);
			}
		}

		ptcl_expression *evaluated = ptcl_expression_unary_static_evaluate(type, value);
		if (evaluated == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, ptcl_parser_current(parser)->location);
			return NULL;
		}

		evaluated->return_type = type_value;
		evaluated->return_type.is_static = value->return_type.is_static;
		return evaluated;
	}

	return ptcl_parser_dot(parser, expected, NULL, flags);
}

static ptcl_expression *ptcl_parser_type_dot(
	ptcl_parser *parser,
	ptcl_expression *left,
	const ptcl_name *const identifier,
	bool is_expression,
	ptcl_location location)
{
	ptcl_assert(identifier != NULL);
	if (ptcl_parser_current(parser)->type != ptcl_token_left_par_type)
	{
		ptcl_parser_throw_except_token(parser, ptcl_lexer_configuration_get_value(parser->configuration, ptcl_token_left_par_type), location);
		ptcl_expression_destroy(left);
		ptcl_parser_disable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
		return NULL;
	}

	ptcl_type_comp_type *comp_type = left->return_type.comp_type;
	ptcl_statement_func_decl *function;
	bool function_found = false;
	if (comp_type->functions != NULL)
	{
		for (size_t i = 0; i < comp_type->functions->count; i++)
		{
			function = &comp_type->functions->statements[i]->func_decl;
			const bool is_compatible = function->is_self_const ||
									   (!function->is_self_const && !left->return_type.is_const);
			if (!is_compatible || !ptcl_name_compare(&function->identifier, identifier))
			{
				continue;
			}

			function_found = true;
			break;
		}
	}

	if (!function_found)
	{
		ptcl_parser_throw_unknown_function(parser, ptcl_name_value(&identifier), location);
		ptcl_expression_destroy(left);
		ptcl_parser_disable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
		return NULL;
	}

	ptcl_parser_function instance = ptcl_parser_function_create(function->root, function);
	ptcl_statement_func_call func_call;
	if (!ptcl_parser_func_call(parser, &instance, left, is_expression, &func_call))
	{
		ptcl_expression_destroy(left);
		ptcl_parser_disable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
		return NULL;
	}

	if (func_call.is_built_in)
	{
		return func_call.built_in;
	}

	func_call.is_built_in = false;
	func_call.return_type = function->return_type;
	ptcl_expression *func = ptcl_expression_create(
		ptcl_expression_func_call_type,
		&func_call.return_type,
		location);
	if (func == NULL)
	{
		ptcl_statement_func_call_destroy(&func_call);
		ptcl_expression_destroy(left);
		ptcl_parser_disable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	func->func_call = func_call;
	ptcl_expression *expression = ptcl_expression_create(
		ptcl_expression_dot_type,
		&func_call.return_type,
		location);
	if (expression == NULL)
	{
		ptcl_expression_destroy(func);
		ptcl_expression_destroy(left);
		ptcl_parser_disable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	expression->dot = ptcl_expression_dot_expression_create(left, func);
	return expression;
}

static ptcl_expression *ptcl_parser_typedata_dot(
	ptcl_parser *parser,
	ptcl_expression *left,
	const ptcl_name *const identifier,
	bool is_expression,
	ptcl_location location)
{
	ptcl_assert(identifier != NULL);
	ptcl_argument *member;
	size_t index;
	if (!ptcl_type_get_member(left->return_type.typedata, identifier, &member, &index))
	{
		ptcl_parser_throw_unknown_member(parser, ptcl_name_value(&identifier), location);
		ptcl_expression_destroy(left);
		ptcl_parser_disable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
		return NULL;
	}

	const bool is_static = left->return_type.is_static;
	if (is_expression)
	{
		ptcl_expression *target = NULL;
		if (left->type == ptcl_expression_ctor_type)
		{
			target = left->ctor.values[index];
		}

		if (member->type.is_static)
		{
			if (target == NULL)
			{
				target = member->default_value;
			}
			else
			{
				if (!is_static)
				{
					target = NULL;
				}
			}
		}

		if (target != NULL)
		{
			ptcl_expression *static_value = ptcl_expression_copy(target, location);
			if (static_value == NULL)
			{
				ptcl_expression_destroy(left);
				ptcl_parser_throw_out_of_memory(parser, location);
				ptcl_parser_disable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
				return NULL;
			}

			static_value->return_type.is_static = true;
			ptcl_expression_destroy(left);
			return static_value;
		}
	}

	ptcl_expression *dot_expr = ptcl_expression_create(
		ptcl_expression_dot_type,
		&member->type,
		left->location);
	if (dot_expr == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, left->location);
		ptcl_expression_destroy(left);
		ptcl_parser_disable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
		return NULL;
	}

	if (!is_expression && is_static)
	{
		dot_expr->return_type.is_static = true;
	}

	dot_expr->dot = ptcl_expression_dot_create(left, identifier);
	return dot_expr;
}

ptcl_expression *ptcl_parser_dot(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	ptcl_expression *left,
	ptcl_parser_expr_flags flags)
{
	if (left == NULL)
	{
		left = ptcl_parser_array_element(parser, expected, NULL, flags);
	}

	if (ptcl_parser_critical(parser) ||
		ptcl_parser_ended(parser) ||
		(left->return_type.type != ptcl_value_typedata_type &&
		 left->return_type.type != ptcl_value_type_type))
	{
		return left;
	}

	if (ptcl_parser_match(parser, ptcl_token_dot_type))
	{
		ptcl_name name = ptcl_parser_name_word(parser);
		ptcl_location location = ptcl_parser_current(parser)->location;
		if (ptcl_parser_critical(parser))
		{
			ptcl_expression_destroy(left);
			return NULL;
		}

		ptcl_expression *result;
		ptcl_parser_enable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
		if (left->return_type.type == ptcl_value_type_type)
		{
			result = ptcl_parser_type_dot(parser, left, &name, ptcl_parser_expr_flags_is_require_expr(flags), location);
		}
		else
		{
			result = ptcl_parser_typedata_dot(parser, left, &name, ptcl_parser_expr_flags_is_require_expr(flags), location);
		}

		ptcl_parser_disable_state(parser, PTCL_PARSER_IN_DOT_FLAG);
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}

		left = result;
		ptcl_token *current = ptcl_parser_current(parser);
		if (current->type == ptcl_token_left_square_type)
		{
			left = ptcl_parser_array_element(parser, expected, left, flags);
		}
		else if (current->type == ptcl_token_dot_type)
		{
			left = ptcl_parser_dot(parser, expected, left, flags);
		}
	}

	return left;
}

ptcl_expression *ptcl_parser_array_element(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	ptcl_expression *left,
	ptcl_parser_expr_flags flags)
{
	if (left == NULL)
	{
		left = ptcl_parser_value(parser, expected, flags);
	}

	if (ptcl_parser_critical(parser) ||
		(left->return_type.type != ptcl_value_array_type && left->return_type.type != ptcl_value_pointer_type) ||
		ptcl_parser_ended(parser))
	{
		return left;
	}

	if (ptcl_parser_match(parser, ptcl_token_left_square_type))
	{
		if (left->return_type.type == ptcl_value_pointer_type && left->return_type.pointer.is_any)
		{
			ptcl_parser_throw_except_type_specifier(parser, ptcl_parser_current(parser)->location);
			ptcl_expression_destroy(left);
			return NULL;
		}

		const bool last_state = ptcl_parser_has_state(parser, PTCL_PARSER_IN_RETURN_FLAG);
		ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, !left->return_type.is_static);
		ptcl_expression *index = ptcl_parser_cast(parser, expected, false);

		ptcl_parser_set_state(parser, PTCL_PARSER_IN_RETURN_FLAG, last_state);
		if (ptcl_parser_critical(parser))
		{
			ptcl_expression_destroy(left);
			return NULL;
		}

		if (ptcl_parser_not(parser, ptcl_token_right_square_type))
		{
			ptcl_expression_destroy(left);
			ptcl_expression_destroy(index);
			return NULL;
		}

		ptcl_type target;
		if (left->return_type.type == ptcl_value_pointer_type)
		{
			target = *left->return_type.pointer.target;
		}
		else
		{
			target = *left->return_type.array.target;
		}

		target.is_static = left->return_type.is_static;
		return ptcl_expression_array_element_create(left, &target, index, index->location);
	}

	return left;
}

static ptcl_expression *ptcl_parser_try_name(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_name word_value = ptcl_parser_name_word(parser);
	if (ptcl_parser_critical(parser))
	{
		return NULL;
	}

	ptcl_expression *word = ptcl_expression_word_create(&word_value, location);
	if (word == NULL)
	{
		ptcl_name_destroy(&word_value);
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	word->return_type.is_static = true;
	return word;
}

static ptcl_expression *ptcl_parser_at(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	bool with_word,
	ptcl_location location)
{
	if (ptcl_parser_peek(parser, 1)->type == ptcl_token_left_curly_type)
	{
		const bool is_succesful = ptcl_parser_extra_body(parser, parser->state.syntax_depth > 0);
		if (!is_succesful)
		{
			if (ptcl_parser_critical(parser))
			{
				return NULL;
			}
		}

		if (is_succesful)
		{
			ptcl_parser_leave_from_extra_body(parser);
		}

		return ptcl_parser_cast(parser, expected, with_word);
	}

	ptcl_parser_throw_unknown_expression(parser, location);
	return NULL;
}

static ptcl_expression *ptcl_parser_lated_body_void(
	ptcl_parser *parser,
	ptcl_location location)
{
	const ptcl_type *previous = parser->temp.return_type;
	parser->temp.return_type = &ptcl_type_void;
	size_t block_start = ptcl_parser_position(parser) - 1;

	ptcl_parser_skip_block_or_expression(parser);
	if (ptcl_parser_critical(parser))
	{
		return NULL;
	}

	parser->temp.return_type = previous;
	size_t token_count = ptcl_parser_position(parser) - block_start;
	size_t index = ptcl_parser_add_lated_body(parser, block_start, token_count, false, location);
	if (ptcl_parser_critical(parser))
	{
		return NULL;
	}

	ptcl_type func = ptcl_type_create_func(&ptcl_type_void, NULL, 0, false, false);
	func.is_static = true;
	ptcl_expression *result = ptcl_expression_create(ptcl_expression_lated_func_body_type, &func, location);
	if (result == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	result->lated_body.index = index;
	return result;
}

static ptcl_expression *ptcl_parser_string(
	ptcl_parser *parser,
	const ptcl_token *const current)
{
	ptcl_assert(current != NULL);
	size_t length = strlen(current->value);
	ptcl_expression **expressions = malloc((length + 1) * sizeof(ptcl_expression *));
	if (expressions == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, current->location);
		return NULL;
	}

	for (size_t i = 0; i < length; i++)
	{
		ptcl_expression *character = ptcl_expression_create_character(current->value[i], current->location);
		if (character == NULL)
		{
			for (size_t j = 0; j < i; j++)
			{
				ptcl_expression_destroy(expressions[j]);
			}

			free(expressions);
			ptcl_parser_throw_out_of_memory(parser, current->location);
			return NULL;
		}

		expressions[i] = character;
	}

	ptcl_expression *eof = ptcl_expression_create_character('\0', current->location);
	if (eof == NULL)
	{
		for (size_t i = 0; i < length; i++)
		{
			ptcl_expression_destroy(expressions[i]);
		}

		free(expressions);
		ptcl_parser_throw_out_of_memory(parser, current->location);
		return NULL;
	}

	expressions[length] = eof;
	ptcl_expression *result = ptcl_expression_create_characters(expressions, length + 1, current->location);
	if (result == NULL)
	{
		for (size_t i = 0; i < length; i++)
		{
			ptcl_expression_destroy(expressions[i]);
		}

		free(expressions);
		ptcl_parser_throw_out_of_memory(parser, current->location);
		return NULL;
	}

	result->return_type.is_static = true;
	return result;
}

static ptcl_expression *ptcl_parser_number(
	ptcl_parser *parser,
	const ptcl_token *const current)
{
	ptcl_assert(current != NULL);
	ptcl_expression *result = NULL;
	ptcl_token *suffix = ptcl_parser_current(parser);
	bool is_float_suffix = suffix->type == ptcl_token_word_type && suffix->value[0] == 'f';
	if (is_float_suffix)
	{
		double value = atof(current->value);
		if (is_float_suffix)
		{
			result = ptcl_expression_create_float((float)value, current->location);
		}
		else
		{
			result = ptcl_expression_create_double(value, current->location);
		}
	}
	else
	{
		long value = strtol(current->value, NULL, 10);
		if (is_float_suffix)
		{
			result = ptcl_expression_create_float((float)value, current->location);
		}
		else
		{
			result = ptcl_expression_create_integer(value, current->location);
		}
	}

	if (result == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, current->location);
		return NULL;
	}

	if (is_float_suffix)
	{
		ptcl_parser_skip(parser);
	}

	return result;
}

static ptcl_expression *ptcl_parser_type_object(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_type object_type;
	if (!ptcl_parser_type(parser, false, false, true, &object_type))
	{
		return NULL;
	}

	ptcl_type *target = malloc(sizeof(ptcl_type));
	if (target == NULL)
	{
		ptcl_type_destroy(&object_type);
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	*target = object_type;
	ptcl_type type = ptcl_type_create_object_type(target, false, true);
	ptcl_expression *result = ptcl_expression_create_object_type(&type, &object_type, location);
	if (result == NULL)
	{
		ptcl_type_destroy(&object_type);
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	result->return_type.is_static = true;
	return result;
}

static ptcl_expression *ptcl_parse_none(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	ptcl_location location)
{
	ptcl_expression *result = NULL;
	if (ptcl_parser_match(parser, ptcl_token_left_par_type))
	{
		ptcl_expression *default_value;
		if (ptcl_parser_match(parser, ptcl_token_greater_than_type))
		{
			ptcl_type type;
			if (!ptcl_parser_type(parser, false, false, true, &type))
			{
				return NULL;
			}

			default_value = ptcl_parser_get_default(parser, &type, location);
			ptcl_type_destroy(&type);
		}
		else
		{
			ptcl_expression *excepted = ptcl_parser_cast(parser, NULL, false);
			if (ptcl_parser_critical(parser))
			{
				return NULL;
			}

			default_value = ptcl_parser_get_default(parser, &excepted->return_type, location);
			ptcl_expression_destroy(excepted);
		}

		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}

		if (ptcl_parser_not(parser, ptcl_token_right_par_type))
		{
			ptcl_expression_destroy(default_value);
			return NULL;
		}

		result = default_value;
	}
	else
	{
		if (expected == NULL)
		{
			ptcl_parser_throw_none_type(parser, location);
			return NULL;
		}

		result = ptcl_parser_get_default(parser, expected, location);
	}

	if (ptcl_parser_critical(parser))
	{
		return NULL;
	}

	if (result == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
	}

	return result;
}

ptcl_expression *ptcl_parser_value(
	ptcl_parser *parser,
	const ptcl_type *const expected,
	ptcl_parser_expr_flags flags)
{
	if (ptcl_parser_parse_try_syntax_usage_here(parser, false))
	{
		size_t depth = parser->state.syntax_depth;
		ptcl_expression *expression = ptcl_parser_cast(parser, expected, flags);
		if (parser->state.syntax_depth == depth)
		{
			ptcl_parser_leave_from_syntax(parser);
		}

		return expression;
	}

	if (ptcl_parser_critical(parser))
	{
		return NULL;
	}

	const bool is_anonymous = ptcl_parser_match(parser, ptcl_token_tilde_type);
	ptcl_token *current = ptcl_parser_current(parser);
	ptcl_location *location = &current->location;

	ptcl_parser_skip(parser);
	switch (current->type)
	{
	case ptcl_token_at_type:
		ptcl_parser_back(parser);
		return ptcl_parser_at(parser, expected, ptcl_parser_expr_flags_is_with_word(flags), *location);
	case ptcl_token_left_curly_type:
		return ptcl_parser_lated_body_void(parser, *location);
	case ptcl_token_exclamation_mark_type:
		ptcl_parser_back(parser);
		ptcl_name word_name = ptcl_parser_name_word(parser);
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}

		bool with_word = ptcl_parser_expr_flags_is_with_word(flags);
	case ptcl_token_word_type:
		if (current->type != ptcl_token_exclamation_mark_type)
		{
			ptcl_parser_back(parser);
			word_name = ptcl_parser_name_word(parser);
			with_word = ptcl_parser_expr_flags_is_with_word(flags);
		}

		return ptcl_parser_func_call_or_var(parser, &word_name, current, flags,
											(expected == NULL && ptcl_parser_expr_flags_is_change_value(flags)) || (expected != NULL && !expected->is_static));
	case ptcl_token_word_word_type:
		ptcl_parser_back(parser);
		ptcl_name value = ptcl_parser_name_word(parser);
		ptcl_expression *word_expr = ptcl_expression_word_create(&value, current->location);
		if (word_expr == NULL)
		{
			ptcl_name_destroy(&value);
			ptcl_parser_throw_out_of_memory(parser, *location);
			return NULL;
		}

		word_expr->return_type.is_static = true;
		return word_expr;
	case ptcl_token_string_type:
		return ptcl_parser_string(parser, current);
	case ptcl_token_number_type:
		return ptcl_parser_number(parser, current);
	case ptcl_token_character_type:
	{
		ptcl_expression *character = ptcl_expression_create_character(current->value[0], current->location);
		if (character == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, *location);
		}

		return character;
	}
	case ptcl_token_static_type:
		if (ptcl_parser_not(parser, ptcl_token_if_type))
		{
			return NULL;
		}
	case ptcl_token_if_type:
	{
		bool is_static = current->type == ptcl_token_static_type;
		ptcl_expression *expression;
		ptcl_parser_if_expression(parser, is_static, &expression);
		return expression;
	}
	case ptcl_token_left_par_type:
	{
		with_word = ptcl_parser_expr_flags_is_with_word(flags);

		// we don't pass expected type, because after (...) can be casting to expected type
		ptcl_expression *inside = ptcl_parser_cast(parser, NULL, flags);
		if (ptcl_parser_critical(parser))
		{
			return NULL;
		}

		if (ptcl_parser_not(parser, ptcl_token_right_par_type))
		{
			ptcl_expression_destroy(inside);
			return NULL;
		}

		return inside;
	}
	case ptcl_token_greater_than_type:
		return ptcl_parser_type_object(parser, *location);
	case ptcl_token_null_type:
	{
		ptcl_expression *null_expr = ptcl_expression_create_null(*location);
		if (null_expr == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, *location);
		}

		return null_expr;
	}
	case ptcl_token_none_type:
		return ptcl_parse_none(parser, expected, *location);
	default:
	{
		const bool last_state = ptcl_parser_add_errors(parser);
		ptcl_parser_disable_state(parser, PTCL_PARSER_ADD_ERRORS_FLAG);

		ptcl_parser_back(parser);
		ptcl_expression *type = ptcl_parser_type_object(parser, *location);

		ptcl_parser_set_state(parser, PTCL_PARSER_ADD_ERRORS_FLAG, last_state);
		if (ptcl_parser_critical(parser))
		{
			ptcl_parser_throw_unknown_expression(parser, *location);
		}

		return type;
	}
	}

	return NULL;
}

bool ptcl_parser_try_provider_name(
	ptcl_parser *parser,
	ptcl_name *const name,
	ptcl_statement_type type,
	ptcl_attributes *const attributes,
	ptcl_stat_mods modifiers)
{
	PTCL_VALIDATE_COND(name != NULL && attributes != NULL);
	bool is_provider = ptcl_parser_match(parser, ptcl_token_provider_type);
	if (!is_provider)
	{
		if (!ptcl_parser_match(parser, ptcl_token_generator_type))
		{
			return false;
		}

		is_provider = false;
	}

	ptcl_parser_except(parser, ptcl_token_left_par_type);
	if (ptcl_parser_critical(parser))
	{
		ptcl_name_destroy(name);
		return false;
	}

	ptcl_location location = ptcl_parser_current(parser)->location;
	ptcl_parse_arguments_result result = ptcl_parser_parse_arguments(parser, location, false, ptcl_parser_args_func_type);
	if (ptcl_parser_critical(parser))
	{
		ptcl_name_destroy(name);
		return false;
	}

	ptcl_parser_except(parser, ptcl_token_left_curly_type);
	if (ptcl_parser_critical(parser))
	{
		ptcl_name_destroy(name);
		ptcl_parse_arguments_result_destroy(&result);
		return false;
	}

	const size_t start = ptcl_parser_position(parser);
	ptcl_parser_skip_block_or_expression(parser);
	if (ptcl_parser_critical(parser))
	{
		ptcl_name_destroy(name);
		ptcl_parse_arguments_result_destroy(&result);
		return false;
	}

	size_t tokens_count = (ptcl_parser_position(parser) - start);
	if (tokens_count > 0)
	{
		// -1 because of curly
		tokens_count -= 1;
	}

	size_t late_index = ptcl_parser_add_lated_body(parser, start, tokens_count, true, location);
	if (ptcl_parser_critical(parser))
	{
		ptcl_name_destroy(name);
		ptcl_parse_arguments_result_destroy(&result);
		return false;
	}

	ptcl_parser_provider provider = ptcl_parser_provider_create(
		type, attributes, modifiers, name, ptcl_parser_root(parser), result.arguments, result.count, result.has_variadic, late_index, is_provider);
	provider.attributes.is_free = false;
	if (!ptcl_parser_add_instance_provider(parser, &provider))
	{
		ptcl_name_destroy(name);
		ptcl_parse_arguments_result_destroy(&result);
		ptcl_parser_tokens_state_destroy(&parser->lated_states.items[--parser->lated_states.count]);
		ptcl_parser_throw_out_of_memory(parser, location);
		return false;
	}

	return true;
}

ptcl_name ptcl_parser_name_word(ptcl_parser *parser)
{
	return ptcl_parser_name(parser, false);
}

ptcl_name ptcl_parser_name_word_or_type(ptcl_parser *parser)
{
	return ptcl_parser_name(parser, true);
}

static ptcl_name ptcl_name_expansion(ptcl_parser *parser, bool with_type)
{
	ptcl_parser_skip(parser);
	const bool is_anonymous = ptcl_parser_match(parser, ptcl_token_tilde_type);
	ptcl_token *current = ptcl_parser_current(parser);
	ptcl_parser_skip(parser);
	if (current->type == ptcl_token_less_than_type)
	{
		ptcl_name left = ptcl_parser_name(parser, false);
		if (ptcl_parser_critical(parser))
		{
			return (ptcl_name){0};
		}

		ptcl_name right = ptcl_parser_name(parser, false);
		if (ptcl_parser_critical(parser))
		{
			ptcl_name_destroy(&left);
			return (ptcl_name){0};
		}

		if (ptcl_parser_not(parser, ptcl_token_greater_than_type))
		{
			ptcl_name_destroy(&left);
			ptcl_name_destroy(&right);
			return (ptcl_name){0};
		}

		char *concatenated = ptcl_string(ptcl_name_value(&left), ptcl_name_value(&right), NULL);
		ptcl_name_destroy(&left);
		ptcl_name_destroy(&right);
		if (concatenated == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, current->location);
			return (ptcl_name){0};
		}

		return ptcl_name_create_owned(concatenated, false);
	}

	ptcl_parser_variable *instance;
	ptcl_parser_function *func_instance;
	ptcl_name name;
	if (current->type == ptcl_token_exclamation_mark_type)
	{
		return ptcl_parser_name(parser, with_type);
	}
	else
	{
		name = ptcl_name_create_borrowed(current->value, is_anonymous);
		if (!ptcl_parser_try_get_variable(parser, &name, &instance) || (instance->type.type != ptcl_value_object_type_type && instance->type.type != ptcl_value_word_type))
		{
			if (ptcl_parser_try_get_left_par(parser))
			{
				if (!ptcl_parser_try_get_function(parser, &name, &func_instance))
				{
					ptcl_parser_throw_unknown_function(parser, ptcl_name_value(&name), current->location);
				cleanup:
					return (ptcl_name){0};
				}

				if (!ptcl_parser_get_instance(parser, ptcl_statement_func_decl_type, func_instance->provider_index, &func_instance, false, current->location))
				{
					if (ptcl_parser_critical(parser))
					{
						goto cleanup;
					}
				}

				const ptcl_type *const return_type = parser->temp.return_type;
				parser->temp.return_type = &ptcl_type_word;

				ptcl_statement_func_call func_call;
				if (!ptcl_parser_func_call(parser, func_instance, NULL, true, &func_call))
				{
					parser->temp.return_type = return_type;
					goto cleanup;
				}

				parser->temp.return_type = return_type;
				if (!func_call.is_built_in)
				{
					ptcl_parser_throw_fast_incorrect_type(parser, &ptcl_type_word, &parser->functions.items[func_call.function_id].func.return_type, current->location);
					ptcl_statement_func_call_destroy(&func_call);
					goto cleanup;
				}

				ptcl_name result = func_call.built_in->word;
				ptcl_expression_shallow_destroy(func_call.built_in, true);
				return result;
			}

			ptcl_parser_throw_unknown_variable(parser, ptcl_name_value(&name), current->location);
			return (ptcl_name){0};
		}

		if (instance->type.is_static)
		{
			switch (instance->type.type)
			{
			case ptcl_value_object_type_type:
				if (!with_type)
				{
					break;
				}

				ptcl_name *result = &instance->identifier;
			case ptcl_value_word_type:
				if (instance->type.type == ptcl_value_word_type)
				{
					result = &instance->built_in->word;
				}

				return ptcl_name_create_borrowed(ptcl_name_value(&result), false);
			default:
				break;
			}
		}

		ptcl_parser_throw_fast_incorrect_type(parser, &ptcl_type_word, &instance->type, current->location);
		return (ptcl_name){0};
	}
}

static ptcl_name ptcl_parser_to_word(
	ptcl_parser *parser)
{
	ptcl_parser_skip(parser);
	ptcl_location location = ptcl_parser_current(parser)->location;
	if (ptcl_parser_not(parser, ptcl_token_left_par_type))
	{
		return (ptcl_name){0};
	}

	ptcl_expression *object_type = NULL;
	ptcl_name target;
	if (ptcl_parser_current(parser)->type == ptcl_token_greater_than_type)
	{
		object_type = ptcl_parser_value(parser, NULL, ptcl_parser_expr_require_expression_flag);
		if (ptcl_parser_critical(parser))
		{
			return (ptcl_name){0};
		}

		if (ptcl_parser_not(parser, ptcl_token_right_par_type))
		{
			ptcl_expression_destroy(object_type);
			return (ptcl_name){0};
		}

		char *value = ptcl_type_to_word_copy(&object_type->object_type.type);
		ptcl_expression_destroy(object_type);
		if (value == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			return (ptcl_name){0};
		}

		return ptcl_name_create_owned(value, false);
	}
	else
	{
		bool last_state = ptcl_parser_add_errors(parser);
		ptcl_parser_disable_state(parser, PTCL_PARSER_ADD_ERRORS_FLAG);

		target = ptcl_parser_name_word(parser);
		ptcl_parser_set_state(parser, PTCL_PARSER_ADD_ERRORS_FLAG, last_state);

		if (ptcl_parser_critical(parser))
		{
			ptcl_parser_disable_state(parser, PTCL_PARSER_CRITICAL_FLAG);
			ptcl_type type;
			if (!ptcl_parser_type(parser, true, true, true, &type))
			{
				return (ptcl_name){0};
			}

			if (ptcl_parser_not(parser, ptcl_token_right_par_type))
			{
				ptcl_type_destroy(&type);
				return (ptcl_name){0};
			}

			char *value = ptcl_type_to_word_copy(&type);
			ptcl_type_destroy(&type);
			if (value == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, location);
				ptcl_type_destroy(&type);
				return (ptcl_name){0};
			}

			return ptcl_name_create_owned(value, false);
		}
	}

	if (ptcl_parser_not(parser, ptcl_token_right_par_type))
	{
		ptcl_name_destroy(&target);
		return (ptcl_name){0};
	}

	ptcl_parser_variable *variable;
	if (!ptcl_parser_try_get_variable(parser, &target, &variable))
	{
		ptcl_parser_throw_unknown_variable(parser, ptcl_name_value(&target), location);
		ptcl_name_destroy(&target);
		return (ptcl_name){0};
	}

	bool incorrect_type = false;
	bool is_free = false;
	const char *value;
	if (!variable->type.is_static)
	{
		incorrect_type = true;
	}
	else
	{
		switch (variable->type.type)
		{
		case ptcl_value_object_type_type:
			is_free = true;
			value = ptcl_type_to_word_copy(&variable->built_in->object_type.type);
			if (value == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, location);
				ptcl_name_destroy(&target);
				return (ptcl_name){0};
			}

			break;
		case ptcl_value_integer_type:
			is_free = true;
			char *number = ptcl_from_int(variable->built_in->integer_n);
			if (number == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, location);
				ptcl_name_destroy(&target);
				return (ptcl_name){0};
			}

			value = ptcl_string("_", number, NULL);
			free(number);
			if (value == NULL)
			{
				ptcl_parser_throw_out_of_memory(parser, location);
				ptcl_name_destroy(&target);
				return (ptcl_name){0};
			}

			break;
		case ptcl_value_word_type:
			value = ptcl_name_value(&variable->built_in->word);
			break;
		default:
			incorrect_type = true;
			break;
		}
	}

	if (incorrect_type)
	{
		ptcl_parser_throw_fast_incorrect_type(parser, &ptcl_type_word, &variable->type, location);
		ptcl_name_destroy(&target);
		return (ptcl_name){0};
	}

	ptcl_name_destroy(&target);
	if (is_free) 
	{
		return ptcl_name_create_owned(value, false);
	}

	return ptcl_name_create_borrowed(value, false);
}

ptcl_name ptcl_parser_name(
	ptcl_parser *parser,
	bool with_type)
{
	ptcl_token *operator = ptcl_parser_current(parser);
	switch (operator->type)
	{
	case ptcl_token_exclamation_mark_type:
		return ptcl_name_expansion(parser, with_type);
	case ptcl_token_word_word_type:
		return ptcl_parser_to_word(parser);
	case ptcl_token_hashtag_type:
	{
		size_t start = ptcl_parser_position(parser);
		if (!ptcl_parser_parse_try_syntax_usage_here(parser, false))
		{
			return (ptcl_name){0};
		}

		ptcl_parser_set_position(parser, start);
		ptcl_name name = ptcl_parser_name(parser, with_type);
		ptcl_parser_leave_from_syntax(parser);
		return name;
	}
	default:
		break;
	}

	const bool is_anonymous = ptcl_parser_match(parser, ptcl_token_tilde_type);
	ptcl_token *token = ptcl_parser_except(parser, ptcl_token_word_type);
	if (ptcl_parser_critical(parser))
	{
		return (ptcl_name){0};
	}

	ptcl_name result = ptcl_name_create_optional(token->value, is_anonymous, false);
	// TODO: remake casts
	if (ptcl_parser_current(parser)->type == ptcl_token_colon_type && ptcl_parser_peek(parser, 1)->type == ptcl_token_colon_type)
	{
		ptcl_parser_skip(parser);
		ptcl_parser_skip(parser);
		ptcl_name first = ptcl_parser_name(parser, with_type);
		ptcl_name *next = ptcl_name_create_as_next(&first);
		if (next == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, operator->location);
			ptcl_name_destroy(&result);
			return (ptcl_name){0};
		}

		result = ptcl_name_create_borrowed_with_next(ptcl_name_value(&result), next, true);
	}

	return result;
}

ptcl_expression *ptcl_parser_get_default(
	ptcl_parser *parser,
	const ptcl_type *const type,
	ptcl_location location)
{
	PTCL_VALIDATE_COND(type != NULL);
	ptcl_expression *result = NULL;
	switch (type->type)
	{
	case ptcl_value_void_type:
	case ptcl_value_array_type:
	case ptcl_value_type_type:
		if (type->comp_type->count == 0)
		{
			ptcl_parser_throw_fast_incorrect_type(parser, type, &ptcl_type_any, location);
			return NULL;
		}

		return ptcl_parser_get_default(parser, &type->comp_type->types[0].type, location);
	case ptcl_value_typedata_type:
	{
		ptcl_type_typedata *instance = type->typedata;
		ptcl_expression **expressions = malloc(instance->count * sizeof(ptcl_expression *));
		if (expressions == NULL)
		{
			return NULL;
		}

		for (size_t i = 0; i < instance->count; i++)
		{
			ptcl_expression *default_value = ptcl_parser_get_default(parser, &instance->members[i].type, location);
			if (ptcl_parser_critical(parser))
			{
				for (size_t j = 0; j < i; j++)
				{
					ptcl_expression_destroy(expressions[j]);
				}

				free(expressions);
				return NULL;
			}

			expressions[i] = default_value;
		}

		ptcl_name typedata_name = instance->identifier;
		ptcl_name_make_immutable(&typedata_name);

		ptcl_type typedata = ptcl_type_create_typedata(type->typedata, false);
		ptcl_expression *ctor = ptcl_expression_create(ptcl_expression_ctor_type, &typedata, location);
		if (ctor == NULL)
		{
			for (size_t i = 0; i < instance->count; i++)
			{
				ptcl_expression_destroy(expressions[i]);
			}

			free(expressions);
			return NULL;
		}

		ctor->ctor = ptcl_expression_ctor_create(&typedata_name, expressions, instance->members, instance->count);
		return ctor;
	}
	case ptcl_value_function_pointer_type:
	case ptcl_value_pointer_type:
		result = ptcl_expression_create_null(location);
		break;
	case ptcl_value_any_type:
	case ptcl_value_object_type_type:
	{
		ptcl_type object_type = ptcl_type_create_object_type(&ptcl_type_any_type, false, false);
		result = ptcl_expression_create_object_type(&object_type, &ptcl_type_any_type, location);
		break;
	}
	case ptcl_value_word_type:
		result = ptcl_expression_word_create(&ptcl_name_empty, location);
		break;
	case ptcl_value_double_type:
		result = ptcl_expression_create_double(0, location);
		break;
	case ptcl_value_float_type:
		result = ptcl_expression_create_float(0, location);
		break;
	case ptcl_value_integer_type:
		result = ptcl_expression_create_integer(0, location);
		break;
	case ptcl_value_character_type:
		result = ptcl_expression_create_character('\0', location);
		break;
	}

	if (result == NULL)
	{
		ptcl_parser_throw_out_of_memory(parser, location);
	}

	return result;
}

ptcl_token *ptcl_parser_except(
	ptcl_parser *parser,
	ptcl_token_type token_type)
{
	ptcl_token *current = ptcl_parser_current(parser);
	if (current->type == token_type)
	{
		ptcl_parser_skip(parser);
		return current;
	}

	char *value;
	switch (token_type)
	{
	case ptcl_token_word_type:
		value = "variable, function or type";
		break;
	default:
		value = ptcl_lexer_configuration_get_value(parser->configuration, token_type);
		break;
	}

	ptcl_parser_throw_except_token(parser, value, current->location);
	return NULL;
}

bool ptcl_parser_not(
	ptcl_parser *parser,
	ptcl_token_type token_type)
{
	return ptcl_parser_except(parser, token_type) == NULL;
}

bool ptcl_parser_match(
	ptcl_parser *parser,
	ptcl_token_type token_type)
{
	if (ptcl_parser_count(parser) == 0)
	{
		return false;
	}

	if (ptcl_parser_current(parser)->type == token_type)
	{
		ptcl_parser_skip(parser);
		return true;
	}

	return false;
}

ptcl_token *ptcl_parser_peek(
	ptcl_parser *parser,
	size_t offset)
{
	size_t position = ptcl_parser_position(parser) + offset;
	size_t count = ptcl_parser_count(parser) - 1;
	if (position > count)
	{
		return &parser->state.tokens.tokens[count];
	}

	return &parser->state.tokens.tokens[position];
}

ptcl_token *ptcl_parser_current(
	ptcl_parser *parser)
{
	ptcl_token *tokens = ptcl_parser_tokens(parser);
	size_t count = ptcl_parser_count(parser);
	if (ptcl_parser_ended(parser))
	{
		if (ptcl_parser_insert_states_count(parser) > 0)
		{
			// Don't need - 1 cuz we have only inserted root at 0
			// TODO: reake syntaxes for first
			ptcl_parser_insert_state *state = ptcl_parser_insert_state_at(parser, ptcl_parser_insert_states_count(parser));
			if (state->syntax_depth == parser->state.syntax_depth)
			{
				ptcl_parser_leave_from_insert_state(parser);
				return &ptcl_parser_tokens(parser)[ptcl_parser_position(parser)];
			}
		}

		if (parser->state.syntax_depth > 0)
		{
			ptcl_parser_leave_from_syntax(parser);
			return ptcl_parser_current(parser);
		}

		if (count > 0)
		{
			return &tokens[count - 1];
		}
	}

	return &tokens[ptcl_parser_position(parser)];
}

ptcl_parser_tokens_state ptcl_parser_get_tokens_state(
	ptcl_parser *parser)
{
	return parser->state.tokens;
}

void ptcl_parser_set_tokens_state(
	ptcl_parser *parser,
	const ptcl_parser_tokens_state *const state)
{
	PTCL_VALIDATE_COND(state != NULL);
	parser->state.tokens = *state;
}

ptcl_token *ptcl_parser_tokens(
	ptcl_parser *parser)
{
	return parser->state.tokens.tokens;
}

void ptcl_parser_set_tokens(
	ptcl_parser *parser,
	ptcl_token *tokens)
{
	parser->state.tokens.tokens = tokens;
}

size_t ptcl_parser_count(
	ptcl_parser *parser)
{
	return parser->state.tokens.count;
}

void ptcl_parser_set_count(
	ptcl_parser *parser,
	size_t count)
{
	parser->state.tokens.count = count;
}

size_t ptcl_parser_position(
	ptcl_parser *parser)
{
	return parser->state.tokens.position;
}

void ptcl_parser_set_position(
	ptcl_parser *parser,
	size_t position)
{
	parser->state.tokens.position = position;
}

inline void ptcl_parser_skip(
	ptcl_parser *parser)
{
	parser->state.tokens.position++;
}

void ptcl_parser_back(
	ptcl_parser *parser)
{
	parser->state.tokens.position--;
}

bool ptcl_parser_ended(
	ptcl_parser *parser)
{
	if (ptcl_parser_count(parser) == 0)
	{
		return true;
	}

	return ptcl_parser_position(parser) > ptcl_parser_count(parser) - 1;
}

bool ptcl_parser_critical(
	ptcl_parser *parser)
{
	return ptcl_parser_has_state(parser, PTCL_PARSER_CRITICAL_FLAG);
}

bool ptcl_parser_ignore_error(
	ptcl_parser *parser)
{
	return ptcl_parser_has_state(parser, PTCL_PARSER_IGNORE_ERROR_FLAG);
}

bool ptcl_parser_in_type(
	ptcl_parser *parser)
{
	return ptcl_parser_has_state(parser, PTCL_PARSER_IN_TYPE_FLAG);
}

bool ptcl_parser_add_errors(
	ptcl_parser *parser)
{
	return ptcl_parser_has_state(parser, PTCL_PARSER_ADD_ERRORS_FLAG);
}

bool ptcl_parser_except_return(
	ptcl_parser *parser)
{
	return ptcl_parser_has_state(parser, PTCL_PARSER_EXCEPT_RETURN_FLAG);
}

bool ptcl_parser_in_syntax(
	ptcl_parser *parser)
{
	return ptcl_parser_has_state(parser, PTCL_PARSER_IN_SYNTAX_FLAG);
}

bool ptcl_parser_in_if(
	ptcl_parser *parser)
{
	return ptcl_parser_has_state(parser, PTCL_PARSER_IN_IF_FLAG);
}

bool ptcl_parser_in_return(
	ptcl_parser *parser)
{
	return ptcl_parser_has_state(parser, PTCL_PARSER_IN_RETURN_FLAG);
}

bool ptcl_parser_in_dot(
	ptcl_parser *parser)
{
	return ptcl_parser_has_state(parser, PTCL_PARSER_IN_DOT_FLAG);
}

size_t ptcl_parser_insert_states_count(
	ptcl_parser *parser)
{
	return parser->temp.insert_states_count;
}

ptcl_parser_insert_state *ptcl_parser_insert_state_at(
	ptcl_parser *parser,
	size_t index)
{
	return &parser->temp.insert_states[index];
}

void ptcl_parser_insert_state_add_and_set(
	ptcl_parser *parser,
	const ptcl_parser_insert_state *const state)
{
	PTCL_VALIDATE_COND(state != NULL);
	if (parser->temp.insert_states_count == 0)
	{
		parser->temp.insert_states[0] = ptcl_parser_insert_state_create(&parser->state.tokens, parser->state.syntax_depth);
	}

	parser->temp.insert_states[++parser->temp.insert_states_count] = *state;
	ptcl_parser_set_tokens_state(parser, &state->state);
}

ptcl_func_body *ptcl_parser_root(
	ptcl_parser *parser)
{
	return parser->temp.root;
}

void ptcl_parser_set_root(
	ptcl_parser *parser,
	ptcl_func_body *root)
{
	parser->temp.root = root;
}

size_t ptcl_parser_variables_count(
	ptcl_parser *parser)
{
	return parser->variables.count;
}

ptcl_parser_variable *ptcl_parser_variables(
	ptcl_parser *parser)
{
	return parser->variables.items;
}

void ptcl_parser_set_state(
	ptcl_parser *parser,
	ptcl_parser_flags flag,
	bool value)
{
	PTCL_FLAGS_TOGGLE(parser->state.states, flag, value);
}

void ptcl_parser_enable_state(
	ptcl_parser *parser,
	ptcl_parser_flags flag)
{
	PTCL_FLAGS_SET(parser->state.states, flag);
}

void ptcl_parser_disable_state(
	ptcl_parser *parser,
	ptcl_parser_flags flag)
{
	PTCL_FLAGS_REMOVE(parser->state.states, flag);
}

bool ptcl_parser_has_state(
	ptcl_parser *parser,
	ptcl_parser_flags flag)
{
	return PTCL_FLAGS_HAS(parser->state.states, flag);
}

bool ptcl_parser_add_this_pair(
	ptcl_parser *parser,
	const ptcl_parser_this_s_pair *const instance)
{
	PTCL_VALIDATE_COND(instance != NULL);
	ptcl_parser_this_s_pair *buffer = realloc(parser->this_pairs.items, (parser->this_pairs.count + 1) * sizeof(ptcl_parser_this_s_pair));
	if (buffer == NULL)
	{
		return false;
	}

	parser->this_pairs.items = buffer;
	parser->this_pairs.items[parser->this_pairs.count++] = *instance;
	return true;
}

ptcl_statement *ptcl_parser_insert_pairs(
	ptcl_parser *parser,
	ptcl_statement *statement,
	ptcl_func_body *body)
{
	for (size_t j = 0; j < parser->this_pairs.count; j++)
	{
		const size_t index = parser->this_pairs.count - 1 - j;
		ptcl_parser_this_s_pair *target = &parser->this_pairs.items[index];
		if (!ptcl_func_body_can_access(body, target->body))
		{
			continue;
		}

		ptcl_location location = ptcl_parser_current(parser)->location;
		ptcl_func_body *content = &statement->body.body;
		ptcl_statement_func_body func_body = ptcl_statement_func_body_create(NULL, 2, statement->root);
		ptcl_statement *statement_body = ptcl_func_body_create_stat(
			&func_body,
			ptcl_parser_root(parser),
			location);
		if (statement_body == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			ptcl_statement_destroy(statement);
			return NULL;
		}

		content = &statement_body->body.body;
		content->statements = malloc(content->count * sizeof(ptcl_statement *));
		if (content->statements == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			ptcl_statement_destroy(statement);
			free(body);
			return NULL;
		}

		content->statements[0] = statement;
		statement = statement_body;

		ptcl_statement_func_body pair_func_body = ptcl_statement_func_body_create(NULL, 0, statement->root);
		ptcl_statement *paired_statement = ptcl_func_body_create_stat(
			&pair_func_body,
			ptcl_parser_root(parser),
			location);
		if (paired_statement == NULL)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			ptcl_statement_destroy(statement);
			return NULL;
		}

		ptcl_parser_tokens_state last_state = parser->state.tokens;
		ptcl_func_body *last_body = parser->temp.inserted_body;

		ptcl_parser_set_tokens_state(parser, &parser->lated_states.items[target->index]);
		ptcl_parser_set_position(parser, 0);
		parser->temp.inserted_body = &paired_statement->body.body;

		ptcl_parser_func_body_by_pointer(parser, parser->temp.inserted_body, false, false, ptcl_parser_ignore_error(parser));

		ptcl_parser_set_tokens_state(parser, &last_state);
		parser->temp.inserted_body = last_body;

		if (ptcl_parser_critical(parser))
		{
			content->count--;
			free(paired_statement);
			ptcl_statement_destroy(statement);
			return NULL;
		}

		content->statements[content->count - 1] = paired_statement;
	}

	return statement;
}

bool ptcl_parser_add_instance_unique_pointer(
	ptcl_parser *parser,
	ptcl_func_body *const instance)
{
	if (parser->unique_root_pointers.count >= parser->unique_root_pointers.capacity)
	{
		parser->unique_root_pointers.capacity *= 2;
		ptcl_func_body **buffer = realloc(parser->unique_root_pointers.items, parser->unique_root_pointers.capacity * sizeof(ptcl_func_body *));
		if (buffer == NULL)
		{
			return false;
		}

		parser->unique_root_pointers.items = buffer;
	}

	parser->unique_root_pointers.items[parser->unique_root_pointers.count++] = instance;
	return true;
}

bool ptcl_parser_add_instance_provider(
	ptcl_parser *parser,
	ptcl_parser_provider *const instance)
{
	PTCL_VALIDATE_COND(instance != NULL);
	if (instance->root != NULL && (parser->state.syntax_depth > 0 || parser->state.extra_bodies_depth > 0) && !instance->identifier.is_anonymous)
	{
		instance->root = parser->temp.main_root;
	}

	if (parser->providers.count >= parser->providers.capacity)
	{
		parser->providers.capacity *= 2;
		ptcl_parser_provider *buffer = realloc(parser->providers.items, parser->providers.capacity * sizeof(ptcl_parser_provider));
		if (buffer == NULL)
		{
			return false;
		}

		parser->providers.items = buffer;
	}

	parser->providers.items[parser->providers.count++] = *instance;
	return true;
}

bool ptcl_parser_add_instance_syntax(
	ptcl_parser *parser,
	ptcl_parser_syntax *const instance)
{
	PTCL_VALIDATE_COND(instance != NULL);
	if (instance->root != NULL && (parser->state.syntax_depth > 0 || parser->state.extra_bodies_depth > 0) && !instance->identifier.is_anonymous)
	{
		instance->root = parser->temp.main_root;
	}

	if (parser->syntaxes.count >= parser->syntaxes.capacity)
	{
		parser->syntaxes.capacity *= 2;
		ptcl_parser_syntax *buffer = realloc(parser->syntaxes.items, parser->syntaxes.capacity * sizeof(ptcl_parser_syntax));
		if (buffer == NULL)
		{
			return false;
		}

		parser->syntaxes.items = buffer;
	}

	parser->syntaxes.items[parser->syntaxes.count++] = *instance;
	return true;
}

bool ptcl_parser_add_instance_comp_type(
	ptcl_parser *parser,
	ptcl_parser_comp_type *const instance)
{
	PTCL_VALIDATE_COND(instance != NULL);
	if (instance->root != NULL && (parser->state.syntax_depth > 0 || parser->state.extra_bodies_depth > 0) && !instance->identifier.is_anonymous)
	{
		instance->root = parser->temp.main_root;
	}

	ptcl_parser_comp_type *target;
	// TODO: optimize in type_decl
	if (ptcl_parser_try_get_any_comp_type(parser, &instance->identifier, &target))
	{
		if (instance->comp_type == NULL)
		{
			if (target->static_type != NULL)
			{
				return false;
			}

			target->static_type = instance->static_type;
			target->static_type->invariant = target->comp_type;
			target->is_auto_static = instance->is_auto_static;
		}
		else
		{
			if (target->comp_type != NULL)
			{
				return false;
			}

			target->comp_type = instance->comp_type;
			if (target->static_type != NULL)
			{
				target->static_type->invariant = target->comp_type;
			}
		}

		return true;
	}

	if (parser->comp_types.count >= parser->comp_types.capacity)
	{
		parser->comp_types.capacity *= 2;
		ptcl_parser_comp_type *buffer = realloc(parser->comp_types.items, parser->comp_types.capacity * sizeof(ptcl_parser_comp_type));
		if (buffer == NULL)
		{
			return false;
		}

		parser->comp_types.items = buffer;
	}

	parser->comp_types.items[parser->comp_types.count++] = *instance;
	return true;
}

bool ptcl_parser_add_instance_typedata(
	ptcl_parser *parser,
	ptcl_parser_typedata *const instance)
{
	PTCL_VALIDATE_COND(instance != NULL);
	if (instance->root != NULL && (parser->state.syntax_depth > 0 || parser->state.extra_bodies_depth > 0) && !instance->typedata->identifier.is_anonymous)
	{
		instance->root = parser->temp.main_root;
	}

	if (parser->typedatas.count >= parser->typedatas.capacity)
	{
		parser->typedatas.capacity *= 2;
		ptcl_parser_typedata *buffer = realloc(parser->typedatas.items, parser->typedatas.capacity * sizeof(ptcl_parser_typedata));
		if (buffer == NULL)
		{
			return false;
		}

		parser->typedatas.items = buffer;
	}

	parser->typedatas.items[parser->typedatas.count++] = *instance;
	return true;
}

bool ptcl_parser_add_instance_function(
	ptcl_parser *parser,
	ptcl_parser_function *const instance)
{
	PTCL_VALIDATE_COND(instance != NULL);
	if (instance->root != NULL && (parser->state.syntax_depth > 0 || parser->state.extra_bodies_depth > 0) && !instance->identifier.is_anonymous)
	{
		instance->root = parser->temp.main_root;
	}

	if (parser->functions.count >= parser->functions.capacity)
	{
		parser->functions.capacity *= 2;
		ptcl_parser_function *buffer = realloc(parser->functions.items, parser->functions.capacity * sizeof(ptcl_parser_function));
		if (buffer == NULL)
		{
			return false;
		}

		parser->functions.items = buffer;
	}

	parser->functions.items[parser->functions.count++] = *instance;
	return true;
}

bool ptcl_parser_add_instance_variable(
	ptcl_parser *parser,
	ptcl_parser_variable *const instance)
{
	PTCL_VALIDATE_COND(instance != NULL);
	if (
		instance->root != NULL && (parser->state.syntax_depth > 0 || parser->state.extra_bodies_depth > 0) && 
		!instance->identifier.is_anonymous && 
		(!ptcl_parser_var_flags_is_built_in(instance->flags) && !ptcl_parser_var_flags_is_source(instance->flags)))
	{
		instance->root = parser->temp.main_root;
	}

	if (parser->variables.count >= parser->variables.capacity)
	{
		parser->variables.capacity *= 2;
		ptcl_parser_variable *buffer = realloc(parser->variables.items, parser->variables.capacity * sizeof(ptcl_parser_variable));
		if (buffer == NULL)
		{
			return false;
		}

		parser->variables.items = buffer;
	}

	parser->variables.items[parser->variables.count++] = *instance;
	return true;
}

bool ptcl_parser_try_get_syntax(
	ptcl_parser *parser,
	const ptcl_name *const name,
	ptcl_parser_syntax **instance)
{
	PTCL_VALIDATE_COND(name != NULL);
	for (size_t j = 0; j < parser->syntaxes.count; j++)
	{
		const size_t index = parser->syntaxes.count - 1 - j;
		ptcl_parser_syntax *syntax = &parser->syntaxes.items[index];
		if (syntax->is_out_of_scope || !ptcl_name_compare(&syntax->identifier, name) || !ptcl_func_body_can_access(syntax->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		*instance = syntax;
		return true;
	}

	return false;
}

bool ptcl_parser_try_get_comp_type(
	ptcl_parser *parser,
	const ptcl_name *const name,
	bool is_static,
	ptcl_parser_comp_type **instance)
{
	PTCL_VALIDATE_COND(name != NULL);
	for (size_t j = 0; j < parser->comp_types.count; j++)
	{
		const size_t index = parser->comp_types.count - 1 - j;
		ptcl_parser_comp_type *comp_type = &parser->comp_types.items[index];
		if (comp_type->is_out_of_scope)
		{
			continue;
		}

		ptcl_type_comp_type *target = is_static ? comp_type->static_type : comp_type->comp_type;
		if ((target == NULL && comp_type->provider_index < 0) ||
			!ptcl_name_compare(&comp_type->identifier, name) ||
			!ptcl_func_body_can_access(comp_type->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		*instance = comp_type;
		return true;
	}

	return false;
}

bool ptcl_parser_try_get_any_comp_type(
	ptcl_parser *parser,
	const ptcl_name *const name,
	ptcl_parser_comp_type **instance)
{
	PTCL_VALIDATE_COND(name != NULL);
	for (size_t j = 0; j < parser->comp_types.count; j++)
	{
		const size_t index = parser->comp_types.count - 1 - j;
		ptcl_parser_comp_type *comp_type = &parser->comp_types.items[index];
		if (comp_type->is_out_of_scope || !ptcl_name_compare(&comp_type->identifier, name) || !ptcl_func_body_can_access(comp_type->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		*instance = comp_type;
		return true;
	}

	return false;
}

bool ptcl_parser_try_get_typedata(
	ptcl_parser *parser,
	const ptcl_name *const name,
	ptcl_parser_typedata **instance)
{
	PTCL_VALIDATE_COND(name != NULL);
	for (size_t j = 0; j < parser->typedatas.count; j++)
	{
		const size_t index = parser->typedatas.count - 1 - j;
		ptcl_parser_typedata *typedata = &parser->typedatas.items[index];
		if (typedata->is_out_of_scope || !ptcl_name_compare(&typedata->identifier, name) || !ptcl_func_body_can_access(typedata->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		*instance = typedata;
		return true;
	}

	return false;
}

bool ptcl_parser_try_get_function(
	ptcl_parser *parser,
	const ptcl_name *const name,
	ptcl_parser_function **instance)
{
	PTCL_VALIDATE_COND(name != NULL);
	for (size_t j = 0; j < parser->functions.count; j++)
	{
		const size_t index = parser->functions.count - 1 - j;
		ptcl_parser_function *function = &parser->functions.items[index];
		if (function->is_out_of_scope || !ptcl_name_compare(&function->identifier, name) || !ptcl_func_body_can_access(function->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		*instance = function;
		return true;
	}

	return false;
}

bool ptcl_parser_try_get_variable(
	ptcl_parser *parser,
	const ptcl_name *name,
	ptcl_parser_variable **instance)
{
	PTCL_VALIDATE_COND(name != NULL);
	for (size_t j = 0; j < parser->variables.count; j++)
	{
		const size_t index = parser->variables.count - 1 - j;
		ptcl_parser_variable *variable = &parser->variables.items[index];
		if (variable->is_out_of_scope || !ptcl_name_compare(&variable->identifier, name) || !ptcl_func_body_can_access(variable->root, ptcl_parser_root(parser)))
		{
			continue;
		}

		*instance = variable;
		return true;
	}

	return false;
}

bool ptcl_parser_try_get_function_by_id(
	ptcl_parser *parser,
	size_t index,
	ptcl_parser_function **function)
{
	if (parser->functions.count == 0 || index > parser->functions.count - 1)
	{
		return false;
	}

	*function = &parser->functions.items[index];
	return true;
}

size_t ptcl_parser_add_lated_body(
	ptcl_parser *parser,
	size_t start,
	size_t tokens_count,
	bool is_free,
	ptcl_location location)
{
	ptcl_token *body_tokens = NULL;
	if (tokens_count > 0)
	{
		body_tokens = malloc(sizeof(ptcl_token) * tokens_count);
		if (body_tokens == NULL && tokens_count > 0)
		{
			ptcl_parser_throw_out_of_memory(parser, location);
			return -1;
		}

		for (size_t i = 0; i < tokens_count; i++)
		{
			body_tokens[i] = ptcl_parser_tokens(parser)[start + i];
		}
	}

	if (parser->lated_states.count >= parser->lated_states.capacity)
	{
		size_t new_capacity = parser->lated_states.capacity == 0 ? 8 : parser->lated_states.capacity * 2;
		ptcl_parser_tokens_state *buffer = realloc(parser->lated_states.items,
												   sizeof(ptcl_parser_tokens_state) * new_capacity);
		if (buffer == NULL)
		{
			for (size_t i = 0; i < tokens_count; i++)
			{
				ptcl_token_destroy(body_tokens[i]);
			}

			free(body_tokens);
			ptcl_parser_throw_out_of_memory(parser, location);
			return -1;
		}

		parser->lated_states.items = buffer;
		parser->lated_states.capacity = new_capacity;
	}

	ptcl_parser_tokens_state *body = &parser->lated_states.items[parser->lated_states.count];
	body->tokens = body_tokens;
	body->count = tokens_count;
	body->is_free = is_free;
	return parser->lated_states.count++;
}

bool ptcl_parser_is_syntax_defined(
	ptcl_parser *parser,
	const ptcl_name *const name)
{
	PTCL_VALIDATE_COND(name != NULL);
	ptcl_parser_syntax *placeholder = NULL;
	return ptcl_parser_try_get_syntax(parser, name, &placeholder);
}

bool ptcl_parser_is_comp_type_defined(
	ptcl_parser *parser,
	const ptcl_name *const name,
	bool is_static)
{
	PTCL_VALIDATE_COND(name != NULL);
	ptcl_parser_comp_type *placeholder = NULL;
	return ptcl_parser_try_get_comp_type(parser, name, is_static, &placeholder);
}

bool ptcl_parser_is_typedata_defined(
	ptcl_parser *parser,
	const ptcl_name *const name)
{
	PTCL_VALIDATE_COND(name != NULL);
	ptcl_parser_typedata *placeholder = NULL;
	return ptcl_parser_try_get_typedata(parser, name, &placeholder);
}

bool ptcl_parser_is_function_defined(
	ptcl_parser *parser,
	const ptcl_name *const name)
{
	PTCL_VALIDATE_COND(name != NULL);
	ptcl_parser_function *placeholder = NULL;
	return ptcl_parser_try_get_function(parser, name, &placeholder);
}

bool ptcl_parser_is_variable_defined(
	ptcl_parser *parser,
	const ptcl_name *const name)
{
	PTCL_VALIDATE_COND(name != NULL);
	ptcl_parser_variable *placeholder = NULL;
	return ptcl_parser_try_get_variable(parser, name, &placeholder);
}

bool ptcl_parser_is_defined(
	ptcl_parser *parser,
	const ptcl_name *const name)
{
	PTCL_VALIDATE_COND(name != NULL);
	void *placeholder = NULL;
	if (ptcl_parser_try_get_syntax(parser, name, &placeholder))
	{
		return true;
	}
	else if (ptcl_parser_try_get_any_comp_type(parser, name, &placeholder))
	{
		return true;
	}
	else if (ptcl_parser_try_get_typedata(parser, name, &placeholder))
	{
		return true;
	}
	else if (ptcl_parser_try_get_function(parser, name, &placeholder))
	{
		return true;
	}
	else if (ptcl_parser_try_get_variable(parser, name, &placeholder))
	{
		return true;
	}

	return false;
}

bool ptcl_parser_check_arguments(
	ptcl_parser *parser,
	ptcl_parser_function *function,
	ptcl_expression **arguments,
	size_t count)
{
	if (function->func.is_variadic)
	{
		if (function->func.count - 1 > count)
		{
			return false;
		}
	}
	else
	{
		if (function->func.count > count)
		{
			return false;
		}
	}

	for (size_t j = 0; j < function->func.count; j++)
	{
		ptcl_argument *argument = &function->func.arguments[j];
		if (argument->is_variadic)
		{
			break;
		}

		if (ptcl_type_is_castable(&argument->type, &arguments[j]->return_type))
		{
			continue;
		}

		return false;
	}

	return true;
}

static bool compare_syntax_nodes(
	ptcl_parser_syntax_node *left,
	ptcl_parser_syntax_node *right)
{
	switch (left->type)
	{
	case ptcl_parser_syntax_node_word_type:
		if (right->type != ptcl_parser_syntax_node_word_type)
		{
			return false;
		}

		return ptcl_name_compare(&left->word.identifier, &right->word.identifier);
	case ptcl_parser_syntax_node_variable_type:
		return right->type == ptcl_parser_syntax_node_value_type &&
			   ptcl_type_is_castable(&left->variable.type, &right->value.value->return_type);
	case ptcl_parser_syntax_node_object_type_type:
		if (right->type != ptcl_parser_syntax_node_value_type)
		{
			return false;
		}

		return ptcl_type_is_castable(&left->object_type, &right->value.value->return_type);
	default:
		return false;
	}
}

bool ptcl_parser_syntax_try_find(
	ptcl_parser *parser,
	ptcl_parser_syntax_node *nodes,
	size_t count,
	ptcl_parser_syntax **syntax,
	bool *can_continue,
	const char **end_token)
{
	*syntax = NULL;
	*can_continue = false;
	*end_token = NULL;
	bool full_match_found = false;

	for (size_t j = 0; j < parser->syntaxes.count; j++)
	{
		const size_t index = parser->syntaxes.count - 1 - j;
		ptcl_parser_syntax *target = &parser->syntaxes.items[index];
		if (target->is_out_of_scope || target->count != count)
		{
			continue;
		}

		bool match = true;
		for (size_t k = 0; k < count; k++)
		{
			if (!compare_syntax_nodes(&target->nodes[k], &nodes[k]))
			{
				match = false;
				break;
			}
		}

		if (match)
		{
			*syntax = target;
			full_match_found = true;
			break;
		}
	}

	for (size_t j = 0; j < parser->syntaxes.count; j++)
	{
		const size_t index = parser->syntaxes.count - 1 - j;
		ptcl_parser_syntax *target = &parser->syntaxes.items[index];
		if (target->is_out_of_scope || target->count <= count)
		{
			continue;
		}

		bool partial_match = true;
		for (size_t k = 0; k < count; k++)
		{
			if (!compare_syntax_nodes(&target->nodes[k], &nodes[k]))
			{
				partial_match = false;
				break;
			}
		}

		if (partial_match)
		{
			if (target->count > count)
			{
				ptcl_parser_syntax_node last = target->nodes[count];
				if (last.type == ptcl_parser_syntax_node_variable_type && last.variable.is_variadic)
				{
					*end_token = ptcl_name_value(&target->nodes[count + 1].word.identifier);
				}
			}

			*can_continue = true;
			break;
		}
	}

	return full_match_found;
}

bool ptcl_parser_try_get_typedata_member(ptcl_parser *parser, const ptcl_name *const name, const char *member_name, ptcl_argument **member, size_t *index)
{
	PTCL_VALIDATE_COND(name != NULL);
	ptcl_parser_typedata *typedata;
	if (!ptcl_parser_try_get_typedata(parser, name, &typedata))
	{
		return false;
	}

	for (size_t i = 0; i < typedata->typedata->count; i++)
	{
		ptcl_argument *typedata_member = &typedata->typedata->members[i];
		if (strcmp(ptcl_name_value(&typedata_member->name), member_name) != 0)
		{
			continue;
		}

		*member = typedata_member;
		*index = i;
		return true;
	}

	return false;
}

bool ptcl_parser_res_try_get_variable_by_id(
	ptcl_parser_result *result,
	size_t index,
	ptcl_parser_variable **variable)
{
	if (result->variables_count == 0 || index > result->variables_count - 1)
	{
		return false;
	}

	*variable = &result->variables[index];
	return true;
}

void ptcl_parser_clear_scope(ptcl_parser *parser)
{
	for (size_t j = 0; j < parser->syntaxes.count; j++)
	{
		const size_t index = parser->syntaxes.count - 1 - j;
		ptcl_parser_syntax *instance = &parser->syntaxes.items[index];
		if (instance->root != ptcl_parser_root(parser))
		{
			continue;
		}

		instance->is_out_of_scope = true;
	}

	for (size_t j = 0; j < parser->comp_types.count; j++)
	{
		const size_t index = parser->comp_types.count - 1 - j;
		ptcl_parser_comp_type *instance = &parser->comp_types.items[index];
		if (instance->root != ptcl_parser_root(parser))
		{
			continue;
		}

		instance->is_out_of_scope = true;
	}

	for (size_t j = 0; j < parser->typedatas.count; j++)
	{
		const size_t index = parser->typedatas.count - 1 - j;
		ptcl_parser_typedata *instance = &parser->typedatas.items[index];
		if (instance->root != ptcl_parser_root(parser))
		{
			continue;
		}

		instance->is_out_of_scope = true;
	}

	for (size_t j = 0; j < parser->variables.count; j++)
	{
		const size_t index = parser->variables.count - 1 - j;
		ptcl_parser_variable *instance = &parser->variables.items[index];
		if (instance->root != ptcl_parser_root(parser))
		{
			continue;
		}

		instance->is_out_of_scope = true;
	}
}

ptcl_expression *ptcl_parser_get_ctor_from_dot(
	ptcl_parser *parser,
	ptcl_expression *dot,
	bool is_root)
{
	if (dot->type != ptcl_expression_dot_type || !dot->dot.is_name)
	{
		return NULL;
	}

	ptcl_location location = ptcl_parser_current(parser)->location;
	ptcl_expression *left = dot->dot.left;
	if (left->type != ptcl_expression_ctor_type)
	{
		return ptcl_parser_get_ctor_from_dot(parser, left, false);
	}

	ptcl_name *name = &dot->dot.name;
	ptcl_expression_ctor *ctor = &left->ctor;
	ptcl_argument *member;
	size_t index;
	if (!ptcl_type_get_member(left->return_type.typedata, name, &member, &index))
	{
		ptcl_parser_throw_unknown_member(parser, ptcl_name_value(&name), location);
		ptcl_expression_destroy(left);
		return NULL;
	}

	if (is_root)
	{
		return left;
	}

	ptcl_expression *static_value = ptcl_expression_copy(ctor->values[index], location);
	if (static_value == NULL)
	{
		ptcl_expression_destroy(left);
		ptcl_parser_throw_out_of_memory(parser, location);
		return NULL;
	}

	static_value->return_type.is_static = true;
	ptcl_expression_destroy(left);
	return static_value;
}

void ptcl_parser_throw_out_of_memory(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_out_of_memory_type, true, "Out of memory", false, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_not_allowed_token(
	ptcl_parser *parser,
	char *token,
	ptcl_location location)
{
	char *message = ptcl_string("Token '", token, "' not allowed", NULL);
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_not_allowed_token_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_except_token(
	ptcl_parser *parser,
	char *value,
	ptcl_location location)
{
	char *message = ptcl_string("Except token '", value, "'", NULL);
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_except_token_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_except_type_specifier(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_except_type_specifier_type, true, "Expect type specifier", false, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_incorrect_type(
	ptcl_parser *parser,
	char *excepted,
	char *received,
	ptcl_location location)
{
	char *message = ptcl_string("Except '", excepted, "' type, but received '", received, "'", NULL);
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_incorrect_type_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_fast_incorrect_type(
	ptcl_parser *parser,
	const ptcl_type *const excepted,
	const ptcl_type *const received,
	ptcl_location location)
{
	PTCL_VALIDATE_COND(excepted != NULL && received != NULL);
	char *excepted_message = ptcl_type_to_present_string_copy(excepted);
	char *received_message = ptcl_type_to_present_string_copy(received);
	ptcl_parser_throw_incorrect_type(parser, excepted_message, received_message, location);

	free(excepted_message);
	free(received_message);
}

void ptcl_parser_throw_variable_redefinition(
	ptcl_parser *parser,
	char *name,
	ptcl_location location)
{
	char *message = ptcl_string("Redefination variable with '", name, "'", NULL);
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_variable_redefinition_type, false, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_none_type(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_none_type_type, true, "Type must be specified to use 'none'", false, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_must_be_variable(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_must_be_variable_type, false, "Must be variable", false, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_must_be_static(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_must_be_static_type, false, "Must be static", false, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_unknown_member(
	ptcl_parser *parser,
	const char *name,
	ptcl_location location)
{
	char *message = ptcl_string("Unknown member with '", name, "' name", NULL);
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_unknown_member_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_unknown_statement(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_unknown_statement_type, true, "Unknown statement", false, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_unknown_expression(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_unknown_expression_type, true, "Unknown expression", false, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_unknown_variable(
	ptcl_parser *parser,
	const char *name,
	ptcl_location location)
{
	char *message = ptcl_string("Unknown variable with '", name, "' name", NULL);
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_unknown_variable_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_unknown_syntax(
	ptcl_parser *parser,
	const ptcl_parser_syntax *const syntax,
	ptcl_location location)
{
	PTCL_VALIDATE_COND(syntax != NULL);
	char *message = ptcl_string("Unknown syntax: '", NULL);
	for (size_t i = 0; i < syntax->count; i++)
	{
		ptcl_parser_syntax_node *node = &syntax->nodes[i];
		if (node->type == ptcl_parser_syntax_node_word_type)
		{
			message = ptcl_string_append(message, ptcl_name_value(&node->word.identifier), NULL);
		}
		else if (node->type == ptcl_parser_syntax_node_value_type)
		{
			char *type = ptcl_type_to_present_string_copy(&node->value.value->return_type);
			message = ptcl_string_append(message, "[", type, "]", NULL);
			free(type);
		}

		if (i != syntax->count - 1)
		{
			message = ptcl_string_append(message, " ", NULL);
		}
	}

	message = ptcl_string_append(message, "'", NULL);

	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_unknown_syntax_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_wrong_arguments(
	ptcl_parser *parser,
	char *name,
	ptcl_expression **values,
	size_t count,
	ptcl_argument *arguments,
	size_t arguments_count,
	ptcl_location location)
{
	char *message = ptcl_string("Wrong arguments '", name, "(", NULL);
	if (message == NULL)
	{
		return;
	}

	for (size_t i = 0; i < count; i++)
	{
		char *type = ptcl_type_to_present_string_copy(&values[i]->return_type);
		if (type == NULL)
		{
			free(message);
			return;
		}

		char *new_message = ptcl_string_append(message, type, NULL);
		free(type);

		if (new_message == NULL)
		{
			free(message);
			return;
		}

		message = new_message;
		if (i != count - 1)
		{
			new_message = ptcl_string_append(message, ", ", NULL);
			if (!new_message)
			{
				free(message);
				return;
			}

			message = new_message;
		}
	}

	char *new_message = ptcl_string_append(message, ")', expected (", NULL);
	if (new_message == NULL)
	{
		free(message);
		return;
	}

	message = new_message;
	for (size_t i = 0; i < arguments_count; i++)
	{
		ptcl_argument *argument = &arguments[i];
		char *type;
		if (argument->is_variadic)
		{
			type = ptcl_string_duplicate("...");
			if (type == NULL)
			{
				free(message);
				return;
			}
		}
		else
		{
			type = ptcl_type_to_present_string_copy(&argument->type);
			if (type == NULL)
			{
				free(message);
				return;
			}
		}

		char *new_message = ptcl_string_append(message, type, NULL);
		free(type);

		if (new_message == NULL)
		{
			free(message);
			return;
		}

		message = new_message;
		if (i != arguments_count - 1)
		{
			new_message = ptcl_string_append(message, ", ", NULL);
			if (!new_message)
			{
				free(message);
				return;
			}

			message = new_message;
		}
	}

	new_message = ptcl_string_append(message, ")", NULL);
	if (new_message == NULL)
	{
		free(message);
		return;
	}

	message = new_message;
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_wrong_arguments_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_max_depth(
	ptcl_parser *parser,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_max_depth_type, true, "Max syntaxes depth", false, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_unknown_function(
	ptcl_parser *parser,
	const char *name,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_unknown_function_type, true, ptcl_string("Unknown function '", name, "'", NULL), true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_unknown_variable_or_type(
	ptcl_parser *parser,
	char *name,
	ptcl_location location)
{
	char *message = ptcl_string("Unknown variable with '", name, "' name or expected type", NULL);
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_unknown_variable_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_unknown_type(
	ptcl_parser *parser,
	char *value,
	ptcl_location location)
{
	char *message = ptcl_string("Unknown type '", value, "'", NULL);
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_unknown_type_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_redefination(
	ptcl_parser *parser,
	char *name,
	ptcl_location location)
{
	char *message = ptcl_string("Redefination '", name, "'", NULL);
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_redefinition_type, false, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_throw_user(
	ptcl_parser *parser,
	char *message,
	ptcl_location location)
{
	ptcl_parser_error error = ptcl_parser_error_create(
		ptcl_parser_error_max_depth_type, true, message, true, location);
	ptcl_parser_add_error(parser, &error);
}

void ptcl_parser_add_error(
	ptcl_parser *parser,
	ptcl_parser_error *const error)
{
	if (!ptcl_parser_add_errors(parser))
	{
		ptcl_parser_error_destroy(error);
		if (!ptcl_parser_critical(parser))
		{
			ptcl_parser_set_state(parser, PTCL_PARSER_CRITICAL_FLAG, error->is_critical);
		}

		return;
	}

	ptcl_parser_error *buffer = realloc(parser->errors.items, (parser->errors.count + 1) * sizeof(ptcl_parser_error));
	if (buffer == NULL)
	{
		ptcl_parser_enable_state(parser, PTCL_PARSER_CRITICAL_FLAG);
		ptcl_parser_error_destroy(error);
	}
	else
	{
		parser->errors.items = buffer;
		parser->errors.items[parser->errors.count++] = *error;
		ptcl_parser_set_state(parser, PTCL_PARSER_CRITICAL_FLAG, error->is_critical);
	}
}

void ptcl_parser_result_destroy(
	ptcl_parser_result *const result)
{
	PTCL_VALIDATE_COND(result != NULL);
	for (size_t i = 0; i < result->errors_count; i++)
	{
		ptcl_parser_error_destroy(&result->errors[i]);
	}

	free(result->errors);
	for (size_t i = 0; i < result->syntaxes_count; i++)
	{
		ptcl_parser_syntax_destroy(&result->syntaxes[i]);
	}

	free(result->syntaxes);
	for (size_t i = 0; i < result->comp_types_count; i++)
	{
		ptcl_parser_comp_type_destroy(&result->comp_types[i]);
	}

	free(result->comp_types);
	for (size_t i = 0; i < result->typedatas_count; i++)
	{
		ptcl_parser_typedata_destroy(&result->typedatas[i]);
	}

	free(result->typedatas);
	free(result->functions);
	for (size_t i = 0; i < result->variables_count; i++)
	{
		ptcl_parser_variable_destroy(&result->variables[i]);
	}

	free(result->variables);

	for (size_t i = 0; i < result->lated_states_count; i++)
	{
		free(result->lated_states[i].tokens);
	}

	free(result->this_pairs);
	for (size_t i = 0; i < result->providers_count; i++)
	{
		ptcl_parser_provider_destroy(&result->providers[i]);
	}

	free(result->providers);
	for (size_t i = 0; i < result->unique_pointers_count; i++)
	{
		free(result->unique_pointers[i]);
	}

	free(result->unique_pointers);
	free(result->lated_states);
	if (!result->is_critical)
	{
		ptcl_func_body_destroy(&result->body);
	}
}

void ptcl_parser_destroy(ptcl_parser *parser)
{
	ptcl_interpreter_destroy(parser->interpreter);
	free(parser);
}