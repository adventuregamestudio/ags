#include <string>
#include "util/string.h"

#include "script/cc_common.h"
#include "cc_parser_test_lib.h"

typedef AGS::Common::String AGSString;
std::string last_cc_error_buf;

void clear_error()
{
    last_cc_error_buf.clear();
}

char const *last_seen_cc_error()
{
    return last_cc_error_buf.c_str();
}

// Reimplementation of project-dependent functions from Common
// IMPORTANT: the last_seen_cc_error must contain unformatted error message.
// It is being used in test and compared to hard-coded strings.
AGSString cc_format_error(const AGSString &message)
{
    last_cc_error_buf = message.GetCStr();
    return message;
}

AGSString cc_get_callstack(int max_lines)
{
    return "";
}

char g_Input_String[] = "\
\"__NEWSCRIPTSTART_StringDefn\"                             \n\
internalstring autoptr builtin managed struct String        \n\
{                                                           \n\
	import static String Format(const string format, ...);  \n\
	import static bool IsNullOrEmpty(String stringToCheck); \n\
	import String  Append(const string appendText);         \n\
	import String  AppendChar(char extraChar);              \n\
	import int     CompareTo(const string otherString,      \n\
					bool caseSensitive = false);            \n\
	import int     Contains(const string needle);           \n\
	import String  Copy();                                  \n\
	import bool    EndsWith(const string endsWithText,      \n\
					bool caseSensitive = false);            \n\
	import int     IndexOf(const string needle);            \n\
	import String  LowerCase();                             \n\
	import String  Replace(const string lookForText,        \n\
					const string replaceWithText,           \n\
					bool caseSensitive = false);            \n\
	import String  ReplaceCharAt(int index, char newChar);  \n\
	import bool    StartsWith(const string startsWithText,  \n\
					bool caseSensitive = false);            \n\
	import String  Substring(int index, int length);        \n\
	import String  Truncate(int length);                    \n\
	import String  UpperCase();                             \n\
	readonly import attribute float AsFloat;                \n\
	readonly import attribute int AsInt;                    \n\
	readonly import attribute char Chars[];                 \n\
	readonly import attribute int Length;                   \n\
};                                                          \n\
\"__NEWSCRIPTSTART_TestCode\"                               \n\
";

char g_Input_Bool[] = "\
\"__NEWSCRIPTSTART_BoolDefn\"             \n\
    enum bool { false = 0, true = 1 };    \n\
\"__NEWSCRIPTSTART_TestCode\"             \n\
";
