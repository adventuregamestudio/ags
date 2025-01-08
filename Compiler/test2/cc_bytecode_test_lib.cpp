//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <string>
#include <fstream>

#include "gtest/gtest.h"
#include "script2/cc_compiledscript.h"

#include "cc_bytecode_test_lib.h"

std::string Esc(const char ch)
{
    if (ch >= ' ' && ch <= 126)
    {
        return std::string(1, ch);
    }

    switch (ch)
    {
    default:
    {
        static const char *tohex = "0123456789abcdef";
        std::string ret = "\\x";
        ret.push_back(tohex[ch / 16]);
        ret.push_back(tohex[ch % 16]);
        return ret;
    }
    case '\a': return "\\a";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\v': return "\\v";
    case '\'': return "\\\'";
    case '\"': return "\\\"";
    case '\\': return "\\\\";
    }
}

std::string EscapeString(const char *in)
{
    if (in == nullptr)
        return "0";

    std::string ret = "";
    size_t const in_len = strlen(in);
    for (size_t idx = 0; idx < in_len; idx++)
        ret += Esc(in[idx]);
    return "\"" + ret + "\"";
}

void WriteOutputCode(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    of << "size_t const codesize = " << scrip.code.size() << ";" << std::endl;
    of << "EXPECT_EQ(codesize, scrip.code.size());" << std::endl << std::endl;

    if (scrip.code.size() == 0)
        return;

    of << "int32_t code[] = {" << std::endl;
    for (size_t idx = 0; idx < scrip.code.size(); idx++)
    {
        of.width(4);
        of << scrip.code[idx] << ", ";
        if (idx % 8 == 3) of << "        ";
        if (idx % 8 == 7) of << "   // " << idx << std::endl;
    }
    of << " -999 " << std::endl << "};" << std::endl;

    of << "CompareCode(&scrip, codesize, code);" << std::endl << std::endl;
}

void CompareCode(AGS::ccCompiledScript *scrip, size_t codesize, int32_t code[])
{
    for (size_t idx = 0; idx < codesize; idx++)
    {
         if (idx >= scrip->code.size())
             break;
         std::string prefix = "code[" + std::to_string(idx) + "] == ";
         std::string should_be_val = prefix + std::to_string(code[idx]);
         std::string is_val = prefix + std::to_string(scrip->code[idx]);
         ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
}

void WriteOutputFixups(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    of << "size_t const numfixups = " << scrip.fixups.size() << ";" << std::endl;
    of << "EXPECT_EQ(numfixups, scrip.fixups.size());" << std::endl << std::endl;

    if (scrip.fixups.size() == 0)
        return;

    of << "int32_t fixups[] = {" << std::endl;
    for (size_t idx = 0; idx < scrip.fixups.size(); idx++)
    {
        of.width(4);
        of << scrip.fixups[idx] << ", ";
        if (idx % 8 == 3) of << "      ";
        if (idx % 8 == 7) of << "   // " << idx << std::endl;
    }
    of << " -999 " << std::endl << "};" << std::endl;
    
    of << "char fixuptypes[] = {" << std::endl;
    for (size_t idx = 0; idx < scrip.fixups.size(); idx++)
    {
        of.width(3);
        of << static_cast<int>(scrip.fixuptypes[idx]) << ", ";
        if (idx % 8 == 3) of << "   ";
        if (idx % 8 == 7) of << "   // " << idx << std::endl;
    }
    of << " '\\0' " << std::endl << "};" << std::endl;

    of << "CompareFixups(&scrip, numfixups, fixups, fixuptypes);" << std::endl << std::endl;
}

void CompareFixups(AGS::ccCompiledScript *scrip, size_t numfixups, int32_t fixups[], char fixuptypes[])
{
    for (size_t idx = 0; idx < numfixups; idx++)
    {
         if (idx >= scrip->fixups.size()) break;
         std::string const prefix = "fixups[" + std::to_string(idx) + "] == ";
         std::string const should_be_val = prefix + std::to_string(fixups[idx]);
         std::string const is_val = prefix + std::to_string(scrip->fixups[idx]);
         ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }

    for (size_t idx = 0; idx < numfixups; idx++)
    {
         if (static_cast<int>(idx) >= scrip->fixups.size()) break;
         std::string const prefix = "fixuptypes[" + std::to_string(idx) + "] == ";
         std::string const should_be_val = prefix + std::to_string(fixuptypes[idx]);
         std::string const is_val = prefix + std::to_string(scrip->fixuptypes[idx]);
         ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
}

void WriteOutputImports(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    // Unfortunately, imports can contain empty strings that
    // mustn't be counted. So we can't just believe numimports,
    // and we can't check against scrip.numimports.
    size_t realNumImports = 0;
    for (size_t idx = 0; idx < scrip.imports.size(); idx++)
        if (0 != strcmp(scrip.imports[idx].c_str(), ""))
            ++realNumImports;

    of << "int const numimports = " << realNumImports << ";" << std::endl;

    of << "std::string imports[] = {" << std::endl;

    size_t linelen = 0;
    for (size_t idx = 0; idx < scrip.imports.size(); idx++)
    {
        if (!strcmp(scrip.imports[idx].c_str(), ""))
            continue;

        std::string item = EscapeString(scrip.imports[idx].c_str());
        item += ",";
        item += std::string(15 - (item.length() % 15), ' ');
        of << item;
        linelen += item.length();
        if (linelen >= 75)
        {
            linelen = 0;
            of << "// " << idx << std::endl;
        }
    }
    of << " \"[[SENTINEL]]\" " << std::endl << "};" << std::endl;

    of << "CompareImports(&scrip, numimports, imports);" << std::endl << std::endl;
}

void CompareImports(AGS::ccCompiledScript *scrip, size_t numimports, std::string imports[])
{
    int idx2 = -1;
    for (size_t idx = 0; idx < scrip->imports.size(); idx++)
    {
         if (!strcmp(scrip->imports[idx].c_str(), ""))
             continue;
         idx2++;
         ASSERT_LT(static_cast<size_t>(idx2), numimports); // idx2 is sure to be non-negative here
         std::string const prefix = "imports[" + std::to_string(idx2) + "] == ";
         std::string const should_be_val = prefix + "\"" + imports[idx2] + "\"";
         std::string const is_val = prefix + "\"" + scrip->imports[idx] + "\"";
         ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
}

void WriteOutputExports(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    of << "size_t const numexports = " << scrip.exports.size() << ";" << std::endl;
    of << "EXPECT_EQ(numexports, scrip.exports.size());" << std::endl << std::endl;

    if (scrip.exports.size() == 0)
        return;

    of << "std::string exports[] = {" << std::endl;

    size_t linelen = 0;
    for (size_t idx = 0; idx < scrip.exports.size(); idx++)
    {
        std::string item = EscapeString(scrip.exports[idx].c_str());
        item += ",";
        item += std::string(6 - (item.length() % 6), ' ');
        of << item;
        linelen += item.length();
        if (linelen >= 50)
        {
            linelen = 0;
            of << "// " << idx << std::endl;
        }
    }
    of << " \"[[SENTINEL]]\" " << std::endl << "};" << std::endl;

    of << "int32_t export_addr[] = {" << std::endl;

    for (size_t idx = 0; idx < scrip.exports.size(); idx++)
    {
        of.setf(std::ios::hex, std::ios::basefield);
        of.setf(std::ios::showbase);
        of.width(4);
        of << scrip.export_addr[idx] << ", ";
        if (idx % 4 == 1) of << "   ";
        if (idx % 8 == 3)
        {
            of.setf(std::ios::dec, std::ios::basefield);
            of.unsetf(std::ios::showbase);
            of.width(0);
            of << "// " << idx;
            of << std::endl;
        }
    }

    of.setf(std::ios::dec, std::ios::basefield);
    of.unsetf(std::ios::showbase);
    of.width(0);

    of << " 0 " << std::endl << "};" << std::endl;

    of << "CompareExports(&scrip, numexports, exports, export_addr);" << std::endl << std::endl;
}

void CompareExports(AGS::ccCompiledScript *scrip, size_t numexports, std::string exports[],  int32_t export_addr[])
{
    for (size_t idx = 0; idx < numexports; idx++)
    {
        if (idx >= scrip->exports.size())
            break;
        std::string const prefix = "exports[" + std::to_string(idx) + "] == ";
        std::string const should_be_val = prefix + "\"" + exports[idx] + "\"";
        std::string const is_val = prefix + "\"" + scrip->exports[idx] + "\"";
        ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
    for (size_t idx = 0; idx < numexports; idx++)
    {
        if (idx >= scrip->exports.size())
            break;
        std::string const prefix = "export_addr[" + std::to_string(idx) + "] == ";
        std::string const should_be_val = prefix + std::to_string(export_addr[idx]);
        std::string const is_val = prefix + std::to_string(scrip->export_addr[idx]);
        ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
}


void WriteOutputStrings(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    of << "size_t const stringssize = " << scrip.strings.size() << ";" << std::endl;
    of << "EXPECT_EQ(stringssize, scrip.strings.size());" << std::endl << std::endl;

    if (scrip.strings.size() == 0)
        return;

    of << "char strings[] = {" << std::endl;
    for (size_t idx = 0; idx < scrip.strings.size(); idx++)
    {
        std::string out = "";
        if (scrip.strings[idx] == 0)
            out = "  0";
        else
            out += '\'' + Esc(scrip.strings[idx]) + '\'';
        of << out << ",  ";
        if (idx % 8 == 3) of << "        ";
        if (idx % 8 == 7) of << "   // " << idx << std::endl;
    }
    of << "'\\0'" << std::endl << "};" << std::endl;

    of << " CompareStrings(&scrip, stringssize, strings);" << std::endl << std::endl;
   }

void CompareStrings(AGS::ccCompiledScript *scrip, size_t stringssize, char strings[])
{
    for (size_t idx = 0; idx < stringssize; idx++)
    {
         if (idx >= scrip->strings.size())
             break;
         std::string const prefix = "strings[" + std::to_string(idx) + "] == ";
         std::string const should_be_val = prefix + std::to_string(strings[idx]);
         std::string const is_val = prefix + std::to_string(scrip->strings[idx]);
         ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
}

void WriteOutput(const char *fname, AGS::ccCompiledScript const &scrip)
{
    std::string path = WRITE_PATH;
    std::ofstream of;
    of.open(path.append(fname).append(".txt"));

    WriteOutputCode(of, scrip);
    WriteOutputFixups(of, scrip);
    WriteOutputImports(of, scrip);
    WriteOutputExports(of, scrip);
    WriteOutputStrings(of, scrip);

    of.close();
}
