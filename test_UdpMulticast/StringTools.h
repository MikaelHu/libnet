#pragma once
#include <string.h>
#include <sstream>


class StringTools {
	/// Check if a byte is an HTTP character.
	static bool IsChar(int c) {
		return c >= 0 && c <= 127;
	}
	/// Check if a byte is an HTTP control character.
	static bool IsCtl(int c) {
		return (c >= 0 && c <= 31) || (c == 127);
	}
	/// Check if a byte is defined as an HTTP tspecial character.
	static bool IsTspecial(int c) {
		switch (c)
		{
		case '(': case ')': case '<': case '>': case '@':
		case ',': case ';': case ':': case '\\': case '"':
		case '/': case '[': case ']': case '?': case '=':
		case '{': case '}': case ' ': case '\t':
			return true;
		default:
			return false;
		}
	}
	/// Check if a byte is a digit.
	static bool IsDigit(int c) {
		return c >= '0' && c <= '9';
	}

	static int FindFirstOf(const char* input, int len, char ch) {
		if (!input || len <= 0)
			return -2;
		int pos{ 0 };
		while (*(input + pos) && pos < len) {
			if (*(input + pos) == ch)
				return pos;
			pos++;
		}
		return -1;
	}
	static int FindLastOf(const char* input, int len, char ch) {
		if (!input || len <= 0)
			return -2;
		int pos{ len - 1 };
		while (*(input + pos) && pos >= 0) {
			if (*(input + pos) == ch)
				return pos;
			pos--;
		}
		return -1;
	}
	static int FindFirstOf(const char* input, int len0, const char* find, int len1) {
		if (!input || len0 <= 0 || !find || len1 <= 0 || len0 < len1)
			return -2;
		int pos{ 0 };
		while (*(input + pos) && pos < (len0 - len1 + 1)) {
			if (*(input + pos) == *find) {
				int p = 0;
				while (*(input + pos + p) == *(find + p) && p < len1) p++;
				if (p == len1)
					return pos;
			}
			pos++;
		}
		return -1;
	}
	static int FindLastOf(const char* input, int len0, const char* find, int len1) {
		if (!input || len0 <= 0 || !find || len1 <= 0 || len0 < len1)
			return -2;
		int pos{ len0 - len1 };
		while (*(input + pos) && pos >= 0) {
			if (*(input + pos) == *find) {
				int p = 0;
				while (*(input + pos + p) == *(find + p) && p < len1) p++;
				if (p == len1)
					return pos;
			}
			pos--;
		}
		return -1;
	}
	static int SubStr(const char* input, int offset, int len, char* output) {
		if (!input || offset <= 0 || len <= 0 || !output)
			return -2;
		int i = 0;
		while (i < len) {
			*(output + i) = *(input + offset + i);
			i++;
		}
		return 0;
	}
	static char* Trim(const char* input) {
		char* p = const_cast<char*>(input);
		char* p1;
		if (p)
		{
			p1 = p + strlen(input) - 1;
			while (*p && isspace(*p))
				p++;
			while (p1 > p && isspace(*p1))
				*p1-- = 0;
		}
		return p;
	}
	static char* Trim(const char* input, char ch) {
		char* p = const_cast<char*>(input);
		char* p1;
		if (p)
		{
			p1 = p + strlen(input) - 1;
			while (*p && *p == ch)
				p++;
			while (p1 > p && *p == ch)
				*p1-- = 0;
		}
		return p;
	}
};