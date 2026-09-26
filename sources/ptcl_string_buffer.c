#include <stdlib.h>
#include <string.h>
#include <ptcl_string_buffer.h>
#include <ptcl_string.h>

typedef struct ptcl_string_buffer
{
    char* buffer;
    size_t capacity;
    size_t length;
    size_t position;
} ptcl_string_buffer;

ptcl_string_buffer* ptcl_string_buffer_create()
{
    ptcl_string_buffer* string_buffer = malloc(sizeof(ptcl_string_buffer));
    if (string_buffer == NULL)
    {
        return NULL;
    }

    size_t initial_capacity = 16;
    char* buffer = malloc(initial_capacity * sizeof(char));
    if (buffer == NULL)
    {
        free(string_buffer);
        return NULL;
    }

    string_buffer->buffer = buffer;
    string_buffer->capacity = initial_capacity;
    string_buffer->length = 0;
    string_buffer->position = 0;
    string_buffer->buffer[0] = '\0';
    return string_buffer;
}

bool ptcl_string_buffer_append_str(ptcl_string_buffer* string_buffer, char* value, size_t count)
{
    if (count == 0)
    {
        return true;
    }

    size_t needed = string_buffer->length + count + 1;
    if (needed > string_buffer->capacity)
    {
        size_t new_capacity = string_buffer->capacity * 2;
        if (new_capacity < needed)
        {
            new_capacity = needed;
        }

        char* buffer = realloc(string_buffer->buffer, new_capacity * sizeof(char));
        if (buffer == NULL)
        {
            return false;
        }

        string_buffer->buffer = buffer;
        string_buffer->capacity = new_capacity;
    }

    memcpy(string_buffer->buffer + string_buffer->length, value, count);
    string_buffer->length += count;
    string_buffer->buffer[string_buffer->length] = '\0';
    return true;
}

bool ptcl_string_buffer_append(ptcl_string_buffer* string_buffer, char value)
{
    size_t needed = string_buffer->length + 2;
    if (needed > string_buffer->capacity)
    {
        size_t new_capacity = string_buffer->capacity * 2;
        if (new_capacity < needed)
        {
            new_capacity = needed;
        }

        char* buffer = realloc(string_buffer->buffer, new_capacity * sizeof(char));
        if (buffer == NULL)
        {
            return false;
        }

        string_buffer->buffer = buffer;
        string_buffer->capacity = new_capacity;
    }

    string_buffer->buffer[string_buffer->length++] = value;
    string_buffer->buffer[string_buffer->length] = '\0';
    return true;
}

bool ptcl_string_buffer_insert(ptcl_string_buffer* string_buffer, char value)
{
    if (string_buffer->position > string_buffer->length)
    {
        return false;
    }

    if (string_buffer->position == string_buffer->length)
    {
        const bool result = ptcl_string_buffer_append(string_buffer, value);
        string_buffer->position++;
        return result;
    }

    size_t needed = string_buffer->length + 2;
    if (needed > string_buffer->capacity)
    {
        size_t new_capacity = string_buffer->capacity * 2;
        if (new_capacity < needed)
        {
            new_capacity = needed;
        }

        char* buffer = realloc(string_buffer->buffer, new_capacity * sizeof(char));
        if (buffer == NULL)
        {
            return false;
        }

        string_buffer->buffer = buffer;
        string_buffer->capacity = new_capacity;
    }

    for (size_t i = string_buffer->length; i > string_buffer->position; i--)
    {
        string_buffer->buffer[i] = string_buffer->buffer[i - 1];
    }

    string_buffer->buffer[string_buffer->position] = value;
    string_buffer->length++;
    string_buffer->buffer[string_buffer->length] = '\0';
    string_buffer->position++;
    return true;
}

bool ptcl_string_buffer_insert_str(ptcl_string_buffer* string_buffer, char* value, size_t count)
{
    if (string_buffer->position > string_buffer->length)
    {
        return false;
    }

    if (count == 0)
    {
        return true;
    }

    size_t needed = string_buffer->length + count + 1;
    if (needed > string_buffer->capacity)
    {
        size_t new_capacity = string_buffer->capacity * 2;
        if (new_capacity < needed)
        {
            new_capacity = needed;
        }
        
        char* buffer = realloc(string_buffer->buffer, new_capacity * sizeof(char));
        if (buffer == NULL)
        {
            return false;
        }

        string_buffer->buffer = buffer;
        string_buffer->capacity = new_capacity;
    }

    if (string_buffer->position < string_buffer->length)
    {
        memmove(string_buffer->buffer + string_buffer->position + count,
            string_buffer->buffer + string_buffer->position,
            string_buffer->length - string_buffer->position);
    }

    memcpy(string_buffer->buffer + string_buffer->position, value, count);
    string_buffer->length += count;
    string_buffer->position += count;
    string_buffer->buffer[string_buffer->length] = '\0';
    return true;
}

char* ptcl_string_buffer_copy(ptcl_string_buffer* string_buffer)
{
    return ptcl_string_duplicate(string_buffer->buffer);
}

char* ptcl_string_buffer_copy_and_clear(ptcl_string_buffer* string_buffer)
{
    char* result = ptcl_string_buffer_copy(string_buffer);
    ptcl_string_buffer_clear(string_buffer);
    return result;
}

size_t ptcl_string_buffer_length(ptcl_string_buffer* string_buffer)
{
    return string_buffer->length;
}

bool ptcl_string_buffer_is_empty(ptcl_string_buffer* string_buffer)
{
    if (string_buffer->length == 0)
    {
        return true;
    }

    for (size_t i = 0; i < string_buffer->length; i++)
    {
        if (string_buffer->buffer[i] == ' ')
        {
            continue;
        }

        return false;
    }

    return true;
}

void ptcl_string_buffer_reset_position(ptcl_string_buffer* string_buffer)
{
    string_buffer->position = string_buffer->length;
}

size_t ptcl_string_buffer_get_position(ptcl_string_buffer* string_buffer)
{
    return string_buffer->position;
}

void ptcl_string_buffer_set_position(ptcl_string_buffer* string_buffer, size_t position)
{
    if (position > string_buffer->length)
    {
        string_buffer->position = string_buffer->length;
    }
    else
    {
        string_buffer->position = position;
    }
}

void ptcl_string_buffer_clear(ptcl_string_buffer* string_buffer)
{
    string_buffer->length = 0;
    string_buffer->position = 0;
    string_buffer->buffer = realloc(string_buffer->buffer, sizeof(char));
    if (string_buffer->buffer != NULL) 
    {
        string_buffer->buffer[0] = '\0';
    }

    string_buffer->capacity = 1;
}

void ptcl_string_buffer_destroy(ptcl_string_buffer* string_buffer)
{
    free(string_buffer->buffer);
    free(string_buffer);
}