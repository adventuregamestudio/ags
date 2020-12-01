#include <string>
#include "util/string.h"

#include "script/cc_options.h"
#include "script/cc_error.h"
#include "script/script_common.h"
#include "cc_parser_test_lib.h"

typedef AGS::Common::String AGSString;
std::string last_cc_error_buf;

void clear_error()
{
    last_cc_error_buf.clear();
}

const char *last_seen_cc_error()
{
    return last_cc_error_buf.c_str();
}

// IMPORTANT: the last_seen_cc_error must contain unformatted error message.
// It is being used in test and compared to hard-coded strings.
std::pair<AGSString, AGSString> cc_error_at_line(const char *error_msg)
{
    last_cc_error_buf = _strdup(error_msg);
    return std::make_pair(AGSString::FromFormat("Error (line %d): %s", currentline, error_msg), AGSString());
}

AGSString cc_error_without_line(const char *error_msg)
{
    last_cc_error_buf = _strdup(error_msg);
    return AGSString::FromFormat("Error (line unknown): %s", error_msg);
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
