#include <stdio.h>
#include <ptcl_transpiler.h>
#include <ptcl_parser.h>
#include <ptcl_lexer.h>
#include <windows.h>
#include <time.h>

// TODO: arrange structure members for greater speed
// TODO: delete all int's with < 0 checking. Use size_t and bool (or ptrdiff_t)
// TODO: fix "lated"
// TODO: make overloads by linked arrays
// TODO: float, integer, double - we need to make it more natural
// TODO: use pointer argument as index in 'for' is bad
// TODO: SOA for somethings
// TODO: finish pointers with loc

int main()
{
    char *source;
    FILE *target = fopen("script.ptcl", "rb");
    if (!target)
    {
        perror("Failed to open file");
        return 1;
    }

    fseek(target, 0, SEEK_END);
    long file_size = ftell(target);
    if (file_size < 0)
    {
        perror("Failed to get file size");
        fclose(target);
        return 1;
    }

    if (file_size == 0)
    {
        fclose(target);
        source = strdup("");
    }
    else
    {
        fseek(target, 0, SEEK_SET);
        source = malloc(file_size + 1);

        if (!source)
        {
            fclose(target);
            perror("Memory allocation failed");
            return 1;
        }

        size_t bytes_read = fread(source, 1, file_size, target);
        if (bytes_read != (size_t)file_size)
        {
            source[bytes_read] = '\0';
        }
        else
        {
            source[file_size] = '\0';
        }
    }

    fclose(target);

    clock_t start = clock();
    ptcl_lexer_configuration configuration = ptcl_lexer_configuration_default();
    ptcl_lexer *lexer = ptcl_lexer_create("console", source, &configuration);
    ptcl_tokens_list tokens_list = ptcl_lexer_tokenize(lexer);

    ptcl_parser *parser = ptcl_parser_create(&tokens_list, &configuration);
    ptcl_parser_result result = ptcl_parser_parse(parser);

    double seconds = 0;
    if (result.errors_count == 0)
    {
        ptcl_transpiler *transpiler = ptcl_transpiler_create(result);
        char *transpiled = ptcl_transpiler_transpile(transpiler);
        ptcl_parser_result_destroy(&result);
        ptcl_parser_destroy(parser);

        ptcl_tokens_list_destroy(tokens_list);
        ptcl_lexer_destroy(lexer);

        clock_t end = clock();
        seconds = (double)(end - start) / CLOCKS_PER_SEC;
        puts(transpiled);

        ptcl_transpiler_destroy(transpiler);
        free(transpiled);
    }
    else
    {
        for (size_t i = 0; i < result.errors_count; i++)
        {
            int error_pos = result.errors[i].location.position;
            int line_start = 0;
            int line_end = 0;
            int line_number = 1;
            for (int j = 0; j < error_pos; j++)
            {
                if (source[j] == '\n')
                {
                    line_number++;
                    line_start = j + 1;
                }
            }

            line_end = error_pos;
            while (source[line_end] != '\0' && source[line_end] != '\n')
            {
                line_end++;
            }

            printf("Error at line %d, position %d:\n", line_number, error_pos - line_start);
            printf("  ");
            for (int j = line_start; j < line_end; j++)
            {
                putchar(source[j]);
            }

            printf("\n  ");
            for (int j = line_start; j < error_pos; j++)
            {
                putchar(' ');
            }
            printf("^\n");

            printf("Message: %s\n\n", result.errors[i].message);
        }

        ptcl_parser_result_destroy(&result);
        ptcl_parser_destroy(parser);

        ptcl_tokens_list_destroy(tokens_list);
        ptcl_lexer_destroy(lexer);
    }

    printf("Comp time: %f sec\n", seconds);
    free(source);
    return 0;
}