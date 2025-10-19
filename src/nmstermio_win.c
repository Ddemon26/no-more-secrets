/*
 * Windows-specific terminal IO implementation.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nmstermio.h"

#define FG_MASK (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define BG_MASK (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY)

static HANDLE hIn = INVALID_HANDLE_VALUE;
static HANDLE hOut = INVALID_HANDLE_VALUE;
static DWORD originalInMode = 0;
static DWORD originalOutMode = 0;
static int modesSaved = 0;
static int clearScr = 0;
static int foregroundColor = 4; /* COLOR_BLUE */
static WORD defaultAttributes = FG_MASK;
static CONSOLE_CURSOR_INFO savedCursorInfo;
static int cursorHidden = 0;

static void ensure_handles(void);
static void clear_console(void);
static void write_utf8(const char *s);
static WORD map_color(int color);

static void ensure_handles(void)
{
	if (hIn == INVALID_HANDLE_VALUE)
	{
		hIn = GetStdHandle(STD_INPUT_HANDLE);
	}
	if (hOut == INVALID_HANDLE_VALUE)
	{
		hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	}
}

static void clear_console(void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	DWORD written;
	COORD origin = {0, 0};

	if (!GetConsoleScreenBufferInfo(hOut, &info))
	{
		return;
	}

	FillConsoleOutputCharacterW(hOut, L' ', info.dwSize.X * info.dwSize.Y, origin, &written);
	FillConsoleOutputAttribute(hOut, defaultAttributes, info.dwSize.X * info.dwSize.Y, origin, &written);
	SetConsoleCursorPosition(hOut, origin);
}

static void write_utf8(const char *s)
{
	if (s == NULL)
	{
		return;
	}

	size_t len = strlen(s);
	if (len == 0)
	{
		return;
	}

	int wlen = MultiByteToWideChar(CP_UTF8, 0, s, (int)len, NULL, 0);
	if (wlen <= 0)
	{
		DWORD written = 0;
		WriteConsoleA(hOut, s, (DWORD)len, &written, NULL);
		return;
	}

	wchar_t *wbuf = (wchar_t *)malloc((size_t)(wlen + 1) * sizeof(wchar_t));
	if (wbuf == NULL)
	{
		DWORD written = 0;
		WriteConsoleA(hOut, s, (DWORD)len, &written, NULL);
		return;
	}

	MultiByteToWideChar(CP_UTF8, 0, s, (int)len, wbuf, wlen);
	DWORD written = 0;
	WriteConsoleW(hOut, wbuf, (DWORD)wlen, &written, NULL);
	free(wbuf);
}

static WORD map_color(int color)
{
	switch (color)
	{
		case 0: /* COLOR_BLACK */
			return 0;
		case 1: /* COLOR_RED */
			return FOREGROUND_RED | FOREGROUND_INTENSITY;
		case 2: /* COLOR_GREEN */
			return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		case 3: /* COLOR_YELLOW */
			return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		case 4: /* COLOR_BLUE */
			return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		case 5: /* COLOR_MAGENTA */
			return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		case 6: /* COLOR_CYAN */
			return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		case 7: /* COLOR_WHITE */
			return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		default:
			return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
	}
}

void nmstermio_init_terminal(void)
{
	ensure_handles();
	if (hIn == INVALID_HANDLE_VALUE || hOut == INVALID_HANDLE_VALUE)
	{
		return;
	}

	if (!modesSaved)
	{
		GetConsoleMode(hIn, &originalInMode);
		GetConsoleMode(hOut, &originalOutMode);

		CONSOLE_SCREEN_BUFFER_INFO info;
		if (GetConsoleScreenBufferInfo(hOut, &info))
		{
			defaultAttributes = info.wAttributes;
		}
		else
		{
			defaultAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		}

		GetConsoleCursorInfo(hOut, &savedCursorInfo);
		modesSaved = 1;
	}

	DWORD inputMode = originalInMode;
	inputMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	inputMode |= ENABLE_EXTENDED_FLAGS;
	SetConsoleMode(hIn, inputMode);

	SetConsoleMode(hOut, originalOutMode | ENABLE_PROCESSED_OUTPUT);

	if (clearScr)
	{
		clear_console();
		CONSOLE_CURSOR_INFO cursor = savedCursorInfo;
		cursor.bVisible = FALSE;
		SetConsoleCursorInfo(hOut, &cursor);
		cursorHidden = 1;
	}
	else
	{
		cursorHidden = 0;
	}
}

void nmstermio_restore_terminal(void)
{
	ensure_handles();

	if (modesSaved)
	{
		SetConsoleMode(hIn, originalInMode);
		SetConsoleMode(hOut, originalOutMode);
	}

	if (cursorHidden)
	{
		SetConsoleCursorInfo(hOut, &savedCursorInfo);
		cursorHidden = 0;
	}
}

int nmstermio_get_rows(void)
{
	ensure_handles();
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (!GetConsoleScreenBufferInfo(hOut, &info))
	{
		return 25;
	}

	return info.srWindow.Bottom - info.srWindow.Top + 1;
}

int nmstermio_get_cols(void)
{
	ensure_handles();
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (!GetConsoleScreenBufferInfo(hOut, &info))
	{
		return 80;
	}

	return info.srWindow.Right - info.srWindow.Left + 1;
}

int nmstermio_get_cursor_row(void)
{
	ensure_handles();
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (!GetConsoleScreenBufferInfo(hOut, &info))
	{
		return 0;
	}

	return (info.dwCursorPosition.Y - info.srWindow.Top) + 1;
}

void nmstermio_move_cursor(int y, int x)
{
	ensure_handles();
	CONSOLE_SCREEN_BUFFER_INFO info;
	COORD pos;

	if (!GetConsoleScreenBufferInfo(hOut, &info))
	{
		return;
	}

	pos.X = (SHORT)x;
	if (y <= 0)
	{
		pos.Y = info.srWindow.Top;
	}
	else
	{
		pos.Y = (SHORT)(info.srWindow.Top + y - 1);
	}

	SetConsoleCursorPosition(hOut, pos);
}

void nmstermio_print_string(char *s)
{
	ensure_handles();
	write_utf8(s);
}

void nmstermio_refresh(void)
{
	fflush(stdout);
}

void nmstermio_clear_input(void)
{
	ensure_handles();

	if (hIn != INVALID_HANDLE_VALUE)
	{
		FlushConsoleInputBuffer(hIn);
	}

	while (_kbhit())
	{
		_getch();
	}
}

char nmstermio_get_char(void)
{
	int c = _getch();
	return (char)c;
}

void nmstermio_print_reveal_string(char *s, int colorOn)
{
	ensure_handles();

	WORD target = defaultAttributes;
	if (colorOn)
	{
		target = (defaultAttributes & BG_MASK) | map_color(foregroundColor);
	}

	SetConsoleTextAttribute(hOut, target);
	write_utf8(s);
	SetConsoleTextAttribute(hOut, defaultAttributes);
}

void nmstermio_show_cursor(void)
{
	ensure_handles();
	CONSOLE_CURSOR_INFO cursor = savedCursorInfo;
	cursor.bVisible = TRUE;
	SetConsoleCursorInfo(hOut, &cursor);
	cursorHidden = 0;
}

void nmstermio_beep(void)
{
	Beep(750, 100);
}

int nmstermio_get_clearscr(void)
{
	return clearScr;
}

void nmstermio_set_clearscr(int s)
{
	if (s)
		clearScr = 1;
	else
		clearScr = 0;
}

void nmstermio_set_foregroundcolor(char *c)
{
	if (c == NULL)
	{
		foregroundColor = 4;
		return;
	}

	if (strcmp("white", c) == 0)
		foregroundColor = 7;
	else if (strcmp("yellow", c) == 0)
		foregroundColor = 3;
	else if (strcmp("black", c) == 0)
		foregroundColor = 0;
	else if (strcmp("magenta", c) == 0)
		foregroundColor = 5;
	else if (strcmp("blue", c) == 0)
		foregroundColor = 4;
	else if (strcmp("green", c) == 0)
		foregroundColor = 2;
	else if (strcmp("red", c) == 0)
		foregroundColor = 1;
	else if (strcmp("cyan", c) == 0)
		foregroundColor = 6;
	else
		foregroundColor = 4;
}

#endif /* _WIN32 */
