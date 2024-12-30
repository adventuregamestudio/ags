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
#include <sstream>
#include <algorithm>

#include "gtest/gtest.h"
#include "script2/cc_compiledscript.h"

#include "cc_bytecode_test_lib.h"

static std::string Esc(char const ch)
{
    if (ch >= ' ' && ch <= 126)
    {
        return std::string(1, ch);
    }

    switch (ch)
    {
    default:
    {
        static char const *tohex = "0123456789abcdef";
        unsigned char uch = static_cast<unsigned char>(ch);
        std::string ret = "\\x";
        ret.push_back(tohex[uch / 16u]);
        ret.push_back(tohex[uch % 16u]);
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

static std::string EscapeString(char const *in)
{
    if (in == nullptr)
        return "0";

    std::string ret = "";
    size_t const in_len = strlen(in);
    for (size_t idx = 0; idx < in_len; idx++)
        ret += Esc(in[idx]);
    return "\"" + ret + "\"";
}

static void WriteOutputCode(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    of << "size_t const code_size = " << scrip.code.size() << ";" << std::endl;
    of << "EXPECT_EQ(code_size, scrip.code.size());" << std::endl << std::endl;

    if (scrip.code.empty())
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

    of << "CompareCode(&scrip, code_size, code);" << std::endl << std::endl;
}

void CompareCode(AGS::ccCompiledScript *scrip, size_t code_size, int32_t code[])
{
    code_size = std::min(code_size, scrip->code.size());
    for (size_t idx = 0u; idx < code_size; idx++)
    {
         std::string prefix = "code[" + std::to_string(idx) + "] == ";
         std::string should_be_val = prefix + std::to_string(code[idx]);
         std::string is_val = prefix + std::to_string(scrip->code[idx]);
         ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
}

static void WriteOutputFixups(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    size_t const fixups_size = scrip.fixups.size();
    of << "size_t const fixups_size = " << fixups_size << ";" << std::endl;
    of << "ASSERT_EQ(scrip.fixups.size(), scrip.fixuptypes.size());" << std::endl;
    of << "EXPECT_EQ(fixups_size, scrip.fixups.size());" << std::endl << std::endl;

    if (scrip.fixups.empty())
        return;

    of << "int32_t fixups[] = {" << std::endl;
    for (size_t idx = 0u; idx < fixups_size; idx++)
    {
        of.width(4);
        of << scrip.fixups[idx] << ", ";
        if (idx % 8u == 3u) of << "      ";
        if (idx % 8u == 7u) of << "   // " << idx << std::endl;
    }
    of << "};" << std::endl;
    
    of << "char fixuptypes[] = {" << std::endl;
    for (size_t idx = 0u; idx < fixups_size; idx++)
    {
        of.width(4);
        of << static_cast<int>(scrip.fixuptypes[idx]) << ", ";
        if (idx % 8u == 3u) of << "      ";
        if (idx % 8u == 7u) of << "   // " << idx << std::endl;
    }
    of << "};" << std::endl;

    of << "CompareFixups(&scrip, fixups_size, fixups, fixuptypes);" << std::endl << std::endl;
}

void CompareFixups(AGS::ccCompiledScript *scrip, size_t fixups_size, int32_t fixups[], char fixuptypes[])
{
    fixups_size = std::min(fixups_size, scrip->fixups.size());

    for (size_t idx = 0u; idx < fixups_size; idx++)
    {
         std::string const prefix = "fixups[" + std::to_string(idx) + "] == ";
         std::string const should_be_val = prefix + std::to_string(fixups[idx]);
         std::string const is_val = prefix + std::to_string(scrip->fixups[idx]);
         ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }

    for (size_t idx = 0u; idx < fixups_size; idx++)
    {
         std::string const prefix = "fixuptypes[" + std::to_string(idx) + "] == ";
         std::string const should_be_val = prefix + std::to_string(fixuptypes[idx]);
         std::string const is_val = prefix + std::to_string(scrip->fixuptypes[idx]);
         ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
}

static void WriteOutputImports(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    // Unfortunately, imports can contain empty strings that mustn't be counted. 
    // So we can't believe 'imports.size()' and we may not check against it.
    std::vector<std::string> imports;
    for (size_t idx = 0u; idx < scrip.imports.size(); idx++)
        if (!scrip.imports[idx].empty())
            imports.push_back(scrip.imports[idx]);

    of << "int const non_empty_imports_count = " << imports.size() << ";" << std::endl;

    if (imports.size())
    {
        of << "std::string imports[] = {" << std::endl;

        size_t line_length = 0u;
        for (size_t idx = 0u; idx < imports.size(); idx++)
        {
            std::string item = EscapeString(imports[idx].c_str());
            item += ",";
            item += std::string(15 - (item.length() % 15), ' ');
            of << item;
            line_length += item.length();
            if (line_length >= 75u)
            {
                line_length = 0u;
                of << "// " << idx << std::endl;
            }
        }
        of << "};" << std::endl;
        of << "CompareImports(&scrip, non_empty_imports_count, imports);" << std::endl << std::endl;
    }
    else
    {
        of << "CompareImports(&scrip, non_empty_imports_count, nullptr);" << std::endl << std::endl;
    }
}

void CompareImports(AGS::ccCompiledScript *scrip, size_t non_empty_imports_count, std::string imports[])
{
    size_t actual_non_empty_imports_count = 0u;
    for (size_t scrip_idx = 0u; scrip_idx < scrip->imports.size(); scrip_idx++)
    {
        if (!scrip->imports[scrip_idx].empty())
            actual_non_empty_imports_count++;
    }
    size_t const should_be = non_empty_imports_count;
    ASSERT_EQ(should_be, actual_non_empty_imports_count);

    if (!imports)
        return;

    size_t array_idx = 0u; // index into 'imports[]'
    for (size_t scrip_idx = 0u; scrip_idx < scrip->imports.size(); scrip_idx++)
    {
        if (scrip->imports[scrip_idx].empty())
            continue;

        std::string const prefix = "scrip.imports[" + std::to_string(scrip_idx) + "] == ";
        std::string const should_be_val = prefix + "\"" + imports[array_idx] + "\"";
        std::string const is_val = prefix + "\"" + scrip->imports[scrip_idx] + "\"";
        ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
        array_idx++;
    }
}

static void WriteOutputExports(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    size_t const exports_size = scrip.exports.size();
    of << "size_t const exports_size = " << scrip.exports.size() << ";" << std::endl;
    of << "ASSERT_EQ(scrip.exports.size(), scrip.export_addr.size());" << std::endl;
    of << "EXPECT_EQ(exports_size, scrip.exports.size());" << std::endl << std::endl;

    if (scrip.exports.empty())
        return;

    of << "std::string exports[] = {" << std::endl;

    size_t line_length = 0u;
    for (size_t idx = 0u; idx < exports_size; idx++)
    {
        std::string item = EscapeString(scrip.exports[idx].c_str());
        item += ",";
        item += std::string(6u - (item.length() % 6u), ' ');
        of << item;
        line_length += item.length();
        if (line_length >= 50u)
        {
            line_length = 0u;
            of << "// " << idx << std::endl;
        }
    }
    of << "};" << std::endl;

    of << "int32_t export_addr[] = {" << std::endl;

    for (size_t idx = 0u; idx < exports_size; idx++)
    {
        of.setf(std::ios::hex, std::ios::basefield);
        of.setf(std::ios::showbase);
        of.width(4);
        of << scrip.export_addr[idx] << ", ";
        if (idx % 4u == 1u) of << "   ";
        if (idx % 8u == 3u)
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

    of << "};" << std::endl;

    of << "CompareExports(&scrip, exports_size, exports, export_addr);" << std::endl << std::endl;
}

void CompareExports(AGS::ccCompiledScript *scrip, size_t exports_size, std::string exports[], int32_t export_addr[])
{
    exports_size = std::min(
        std::min(exports_size, scrip->exports.size()),
        exports_size);
    exports_size = std::min(
        std::min(exports_size, scrip->export_addr.size()),
        exports_size);

    for (size_t idx = 0u; idx < exports_size; idx++)
    {
        std::string const prefix = "exports[" + std::to_string(idx) + "] == ";
        std::string const should_be_val = prefix + "\"" + exports[idx] + "\"";
        std::string const is_val = prefix + "\"" + scrip->exports[idx] + "\"";
        ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
    for (size_t idx = 0u; idx < exports_size; idx++)
    {
        std::string const prefix = "export_addr[" + std::to_string(idx) + "] == ";
        std::string const should_be_val = prefix + std::to_string(export_addr[idx]);
        std::string const is_val = prefix + std::to_string(scrip->export_addr[idx]);
        ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
}


static void WriteOutputStrings(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    of << "size_t const strings_size = " << scrip.strings.size() << ";" << std::endl;
    of << "EXPECT_EQ(strings_size, scrip.strings.size());" << std::endl << std::endl;

    if (scrip.strings.empty())
        return;

    of << "char strings[] = {" << std::endl;
    for (size_t idx = 0u; idx < scrip.strings.size(); idx++)
    {
        std::string out = "";
        if (scrip.strings[idx] == 0)
            out = "  0";
        else
            out += '\'' + Esc(scrip.strings[idx]) + '\'';
        of << out << ",  ";
        if (idx % 8u == 3u) of << "        ";
        if (idx % 8u == 7u) of << "   // " << idx << std::endl;
    }
    of << "};" << std::endl;

    of << " CompareStrings(&scrip, strings_size, strings);" << std::endl << std::endl;
   }

void CompareStrings(AGS::ccCompiledScript *scrip, size_t strings_size, char strings[])
{
    strings_size = std::min(strings_size, scrip->strings.size());
    for (size_t idx = 0u; idx < strings_size; idx++)
    {
         std::string const prefix = "strings[" + std::to_string(idx) + "] == ";
         std::string const should_be_val = prefix + std::to_string(strings[idx]);
         std::string const is_val = prefix + std::to_string(scrip->strings[idx]);
         ASSERT_STREQ(should_be_val.c_str(), is_val.c_str());
    }
}

static void WriteOutputGlobals(std::ofstream &of, AGS::ccCompiledScript const &scrip)
{
    size_t const globaldata_size = scrip.globaldata.size();
    of << "size_t const globaldata_size = " << globaldata_size << ";" << std::endl;
    of << "EXPECT_EQ(globaldata_size, scrip.globaldata.size());" << std::endl << std::endl;

    if (scrip.globaldata.empty())
        return;

    std::vector<CharRun> global_runs = {};

    for (size_t idx = 0u; idx < globaldata_size;)
    {
        char const run_char = scrip.globaldata[idx];
        size_t run_count = 0u;
        while (idx < globaldata_size && run_char == scrip.globaldata[idx])
        {
            run_count++;
            idx++;
        }
        global_runs.emplace_back(run_count, run_char);
    }

    if (global_runs.size() * 2u < scrip.globaldata.size())
    {
        // Write a run-length encoding of the globals to save space
        // and for better comprehension
        // A run '{ 7, 0xAA }' means:
        // Expect 7 consecutive bytes that each have the value '0xAA'.
        size_t cumulative = 0u;
        of << "CharRun global_runs[] = {" << std::endl;
        for (size_t idx = 0u; idx < global_runs.size(); idx++)
        {
            cumulative += global_runs[idx].count;
            of << "{ " <<
                std::dec << std::setfill(' ') << std::setw(3) <<
                    global_runs[idx].count << ", 0x" <<
                std::hex << std::setfill('0') << std::setw(2) <<
                (0xFF & global_runs[idx].value) <<
                "}, ";
            if (idx % 4u == 3u)
                of << "   // " << std::dec << std::setw(0) << cumulative << std::endl;
        }
        // Add a sentinel that has a count of '0' 
        of << "{0, 0x00} };" << std::endl;
        of << "CompareGlobalRuns(&scrip, global_runs);" <<
            std::endl << std::endl;
        return;
    }

    // Write the globals byte-by-byte
    of << "unsigned char globaldata[] = {" << std::endl;
    for (size_t idx = 0u; idx < scrip.globaldata.size(); idx++)
    {
        of << "0x" <<
            std::hex << std::setfill('0') << std::setw(2) <<
            (0xFF & scrip.globaldata[idx]) << ", ";
        if (idx % 8u == 3u) of << "        ";
        if (idx % 8u == 7u) of << "   // " << std::dec << std::setw(0) << idx << std::endl;
    }
    of << "};" << std::endl;
    of << "CompareGlobalData(&scrip, globaldata_size, globaldata);" <<
        std::endl << std::endl;
}

void CompareGlobalRuns(AGS::ccCompiledScript *scrip, CharRun global_runs[])
{
    size_t runs_idx = 0u;
    // A run '{ 7, 0xAA }' means:
    // Expect 7 consecutive bytes that each have the value '0xAA'.
    size_t counter = global_runs[runs_idx].count;
    unsigned char value = global_runs[runs_idx].value;
    size_t data_idx = 0u;
    do {
        if (data_idx >= scrip->globaldata.size())
            break;
        if ((unsigned char) scrip->globaldata[data_idx] != value)
        {
            std::string const prefix = "globaldata[" + std::to_string(data_idx) + "] == 0x";
            std::ostringstream should_be_val;
            should_be_val << prefix << std::hex << std::setfill('0') <<
                std::setw(2) << (0xFF & value);
            std::ostringstream is_val;
            is_val << prefix << std::hex << std::setfill('0') <<
                std::setw(2) << (0xFF & scrip->globaldata[data_idx]);
            ASSERT_STREQ(should_be_val.str().c_str(), is_val.str().c_str());
        }
        ++data_idx;
        --counter;
        if (!counter)
        {
            // Get next run
            runs_idx++;
            counter = global_runs[runs_idx].count;
            value = global_runs[runs_idx].value;
        }
    } while (counter);
}

void CompareGlobalData(AGS::ccCompiledScript *scrip, size_t size, unsigned char gdata[])
{
    size = std::min(size, scrip->globaldata.size());
    for (size_t idx = 0u; idx < size; idx++)
    {
        if ((unsigned char) scrip->globaldata[idx] == gdata[idx])
            continue;

        std::string const prefix = "globaldata[" + std::to_string(idx) + "] == 0x";
        std::ostringstream should_be_val;
        should_be_val << prefix << std::hex << std::setfill('0') <<
            std::setw(2) << (0xFF & gdata[idx]);
        std::ostringstream is_val;
        is_val << prefix << std::hex << std::setfill('0') <<
            std::setw(2) << (0xFF & scrip->globaldata[idx]);
        ASSERT_STREQ(should_be_val.str().c_str(), is_val.str().c_str());
    }
}

void WriteOutput(char const *fname, AGS::ccCompiledScript const &scrip)
{
    std::ofstream output { std::string(WRITE_PATH) + fname + ".txt" };
    output.exceptions(std::ofstream::failbit);

    WriteOutputCode(output, scrip);
    WriteOutputFixups(output, scrip);
    WriteOutputImports(output, scrip);
    WriteOutputExports(output, scrip);
    WriteOutputStrings(output, scrip);
    WriteOutputGlobals(output, scrip);
}
