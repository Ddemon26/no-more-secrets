/*
 * Copyright (c) 2017 Brian Barto
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GPL License. See LICENSE for more details.
 */

#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "input.h"
#include "error.h"

int input_get(unsigned char **dest, char *prompt)
{
	size_t capacity = 0;
	size_t length = 0;
	unsigned char *buffer = NULL;
	int ch;
	int is_terminal;

	if (dest == NULL)
	{
		error_log("Invalid destination buffer.");
		return -1;
	}

	*dest = NULL;
	is_terminal = isatty(STDIN_FILENO);

	if (is_terminal && prompt != NULL)
	{
		printf("%s", prompt);
		fflush(stdout);
	}

	while ((ch = fgetc(stdin)) != EOF)
	{
		if (capacity - length < 1)
		{
			size_t new_capacity = (capacity == 0) ? 256 : capacity * 2;
			unsigned char *tmp = realloc(buffer, new_capacity);
			if (tmp == NULL)
			{
				free(buffer);
				error_log("Memory allocation error.");
				return -1;
			}
			buffer = tmp;
			capacity = new_capacity;
		}

		buffer[length++] = (unsigned char)ch;

		if (is_terminal && ch == '\n')
		{
			break;
		}
	}

	if (ferror(stdin))
	{
		free(buffer);
		error_log("Input read error. Errno: %i", errno);
		return -1;
	}

	if (length == 0)
	{
		free(buffer);
		return 0;
	}

	*dest = buffer;
	return (int)length;
}

int input_get_str(char** dest, char *prompt)
{
	int r, i, input_len;
	unsigned char *input;

	r = input_get(&input, prompt);
	if (r < 0)
	{
		error_log("Could not get input.");
		return -1;
	}

	if (r > 0)
	{
		if (input[r - 1] == '\n')
		{
			--r;
			if (r > 0 && input[r - 1] == '\r')
			{
				--r;
			}
		}
	}

	if (r == 0)
	{
		error_log("No input provided.");
		return -1;
	}

	input_len = r;

	*dest = malloc(input_len + 1);
	if (*dest == NULL)
	{
		error_log("Memory allocation error.");
		return -1;
	}

	memset(*dest, 0, input_len + 1);

	for (i = 0; i < input_len; ++i)
	{
		if (isascii(input[i]))
		{
			(*dest)[i] = input[i];
		}
		else
		{
			error_log("Input contains non-ascii characters.");
			free(input);
			return -1;
		}
	}

	free(input);

	return input_len;
}

int input_get_from_pipe(unsigned char** dest)
{
	int r;

	if (isatty(STDIN_FILENO))
	{
		error_log("Input data from a piped or redirected source is required.");
		return -1;
	}

	r = input_get(dest, NULL);
	if (r < 0)
	{
		error_log("Could not get input.");
		return -1;
	}
	if (r == 0)
	{
		error_log("No input provided.");
		return -1;
	}

	return r;
}
