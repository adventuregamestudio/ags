#include "gtest/gtest.h"
#include "script2/cc_symboltable.h"

TEST(SymbolTable, GetNameNonExistent)
{
    AGS::SymbolTable symt;

    EXPECT_STREQ("(invalid symbol)", symt.GetName(100).c_str());
    EXPECT_STREQ("(invalid symbol)", symt.GetName(200).c_str());

    // check edge conditions. index immediately after 'c' should be null
    symt.Add("a");
    symt.Add("b");
    int const c_sym = symt.Add("c");
    EXPECT_STREQ("(invalid symbol)", symt.GetName(c_sym + 1).c_str());
}

TEST(SymbolTable, GetNameNormal)
{
    AGS::SymbolTable symt;

    // Read out the name that was put in

    int const foo_sym = symt.Add("foo");

    EXPECT_STREQ("foo", symt.GetName(foo_sym).c_str());
}

TEST(SymbolTable, GetNamePredefined)
{
    AGS::SymbolTable symt;

    EXPECT_STREQ("return", symt.GetName(AGS::kKW_Return).c_str());
}

TEST(SymbolTable, GetVartypeName)
{
    AGS::SymbolTable symt;

    AGS::Vartype const foo_vartype = symt.Add("foo");
    symt.MakeEntryVartype(foo_vartype);
    symt[foo_vartype].VartypeD->Type = AGS::VTT::kAtomic;
    EXPECT_STREQ(
        "foo[]",
        symt.GetName(symt.VartypeWith(AGS::VTT::kDynarray, foo_vartype)).c_str());

    EXPECT_STREQ(
        "foo *",
        symt.GetName(symt.VartypeWith(AGS::VTT::kDynpointer, foo_vartype)).c_str());

    std::vector<size_t> const dims = { 3, 5, 7 };
    EXPECT_STREQ(
        "foo[3, 5, 7]",
        symt.GetName(symt.VartypeWithArray(dims, foo_vartype)).c_str());
}

TEST(SymbolTable, VartypeWithWithout)
{
    AGS::SymbolTable symt;

    AGS::Vartype const foo_vartype = symt.Add("foo");
    symt.MakeEntryVartype(foo_vartype);
    symt[foo_vartype].VartypeD->Type = AGS::VTT::kAtomic;

    AGS::Vartype foo_converted = symt.VartypeWith(AGS::VTT::kDynarray, foo_vartype);
    EXPECT_EQ(foo_vartype, symt.VartypeWithout(AGS::VTT::kDynarray, foo_converted));

    foo_converted = symt.VartypeWith(AGS::VTT::kConst, foo_vartype);
    EXPECT_EQ(foo_vartype, symt.VartypeWithout(AGS::VTT::kConst, foo_converted));
    EXPECT_EQ(foo_converted, symt.VartypeWithout(AGS::VTT::kDynarray, foo_converted));
}

TEST(SymbolTable, AddSymbolAlreadyExists)
{
    AGS::SymbolTable symt;

    int const a_sym = symt.Add("a");
    ASSERT_EQ(AGS::kKW_NoSymbol, symt.Add("a"));
}

TEST(SymbolTable, AddSymbolUnique)
{
    AGS::SymbolTable symt;

    int const a_sym = symt.Add("a");
    int const b_sym = symt.Add("b");
    ASSERT_NE(a_sym, b_sym);
}

TEST(SymbolTable, FindOrAdd)
{
    AGS::SymbolTable symt;

    int const a_sym = symt.Add("a");
    int const b_sym = symt.Add("b");
    int const a2_sym = symt.FindOrAdd("a");
    EXPECT_EQ(a_sym, a2_sym);
}

TEST(SymbolTable, FindResetCaches)
{
    AGS::SymbolTable symt;

    // Even after caches are reset, Find() must work

    EXPECT_EQ(AGS::kKW_NoSymbol, symt.Find("Llanfair­pwllgwyngyll­gogery­chwyrn­drobwll­llan­tysilio­gogo­goch"));
    ASSERT_EQ(AGS::kKW_Minus, symt.Find("-"));
    symt.ResetCaches();
    EXPECT_EQ(AGS::kKW_Minus, symt.Find("-"));
    EXPECT_EQ(AGS::kKW_NoSymbol, symt.Find("Taumatawhakatangihangakoauauotamateapokaiwhenuakitanatahu"));
    // BTW those names really exist
}

TEST(SymbolTable, EntriesEnsureModifiable)
{
    AGS::SymbolTable symt;

    // ensure reading and writing to entries actually works

    int const a_sym = symt.Add("x");
    symt.entries.at(a_sym).Declared = 10;
    EXPECT_EQ(10, symt.entries.at(a_sym).Declared);
    symt[a_sym].Declared = 11;
    EXPECT_EQ(11, symt.entries.at(a_sym).Declared);
    EXPECT_EQ(11, symt[a_sym].Declared);
}

TEST(SymbolTable, StringStruct)
{
    AGS::SymbolTable symt;

    // set/get a stringstruct symbol

    AGS::Symbol const str_set = symt.Add("String");
    symt.SetStringStructSym(str_set);

    AGS::Symbol const str_get = symt.GetStringStructSym();
    EXPECT_EQ(str_set, str_get);

    AGS::Symbol const strptr_get = symt.GetStringStructPtrSym();
    EXPECT_EQ(str_set, symt.VartypeWithout(AGS::VTT::kDynpointer, strptr_get));
    EXPECT_TRUE(symt.IsDynpointerVartype(strptr_get));
}

TEST(SymbolTable, IsInBounds)
{
    AGS::SymbolTable symt;

    EXPECT_FALSE(symt.IsInBounds(-1));
    EXPECT_FALSE(symt.IsInBounds(0));
    EXPECT_FALSE(symt.IsInBounds(symt.GetLastAllocated() + 1));
    EXPECT_TRUE(symt.IsInBounds(AGS::kKW_New));
}

TEST(SymbolTable, CanBePartOfAnExpression)
{
    AGS::SymbolTable symt;

    EXPECT_FALSE(symt.CanBePartOfAnExpression(AGS::kKW_Float));     // typical vartype
    EXPECT_FALSE(symt.CanBePartOfAnExpression(AGS::kKW_Return));    // typical keyword
    EXPECT_TRUE(symt.CanBePartOfAnExpression(AGS::kKW_And));        // typical bool operator
    EXPECT_TRUE(symt.CanBePartOfAnExpression(AGS::kKW_Divide));     // typical binary operator
    EXPECT_FALSE(symt.CanBePartOfAnExpression(AGS::kKW_CloseBrace));        // delimiter 
    EXPECT_TRUE(symt.CanBePartOfAnExpression(AGS::kKW_OpenBracket));       // delimiter 
    EXPECT_TRUE(symt.CanBePartOfAnExpression(AGS::kKW_OpenParenthesis));    // delimiter

    // Literal
    AGS::Symbol const lit_sym = symt.Add("4711");
    symt.MakeEntryLiteral(lit_sym);
    EXPECT_TRUE(symt.CanBePartOfAnExpression(lit_sym));

    // Constant
    AGS::Symbol const const_sym = symt.Add("kBoom");
    symt.MakeEntryConstant(const_sym);
    EXPECT_TRUE(symt.CanBePartOfAnExpression(const_sym));

    // Function
    AGS::Symbol const func_sym = symt.Add("Foo");
    symt.MakeEntryFunction(func_sym);
    EXPECT_TRUE(symt.CanBePartOfAnExpression(func_sym));

    // Variable
    AGS::Symbol const var_sym = symt.Add("Damage");
    symt.MakeEntryVariable(var_sym);
    EXPECT_TRUE(symt.CanBePartOfAnExpression(var_sym));
}

TEST(SymbolTable, IsPrimitiveAtomic)
{
    AGS::SymbolTable symt;

    EXPECT_TRUE(symt.IsPrimitiveVartype(AGS::kKW_Long));
    EXPECT_TRUE(symt.IsAtomicVartype(AGS::kKW_Long));
    EXPECT_FALSE(symt.IsPrimitiveVartype(AGS::kKW_Return));
    EXPECT_FALSE(symt.IsAtomicVartype(AGS::kKW_Return));

    AGS::Symbol const structy_sym = symt.Add("Structy");
    symt.MakeEntryVartype(structy_sym);
    EXPECT_FALSE(symt.IsPrimitiveVartype(structy_sym));
    EXPECT_TRUE(symt.IsAtomicVartype(structy_sym));

    AGS::Symbol const structy_ptr_sym = symt.VartypeWith(AGS::VTT::kDynpointer, structy_sym);
    EXPECT_FALSE(symt.IsPrimitiveVartype(structy_ptr_sym));
    EXPECT_FALSE(symt.IsAtomicVartype(structy_ptr_sym));
}

TEST(SymbolTable, ArrayClassic)
{
    AGS::SymbolTable symt;

    AGS::Symbol const array_sym = symt.Add("HArry");
    symt.MakeEntryVartype(array_sym);
    symt[array_sym].VartypeD->BaseVartype = AGS::kKW_Int;
    symt[array_sym].VartypeD->Dims = { 2, };
    symt[array_sym].VartypeD->Type = AGS::VTT::kArray;

    EXPECT_TRUE(symt.IsArrayVartype(array_sym));
    EXPECT_TRUE(symt.IsAnyArrayVartype(array_sym));
    EXPECT_FALSE(symt.IsDynarrayVartype(array_sym));
    EXPECT_EQ(2, symt.NumArrayElements(array_sym));

    symt[array_sym].VartypeD->Dims = { 2, 3, 5, 7, };
    EXPECT_EQ(2 * 3 * 5 * 7, symt.NumArrayElements(array_sym));
}

TEST(SymbolTable, OperatorPrio)
{
    AGS::SymbolTable symt;

    EXPECT_EQ(symt.kNoPrio, symt.BinaryOpPrio(AGS::kKW_BitNeg));
    EXPECT_NE(symt.kNoPrio, symt.UnaryOpPrio(AGS::kKW_BitNeg));

    EXPECT_NE(symt.kNoPrio, symt.BinaryOpPrio(AGS::kKW_Divide));
    EXPECT_EQ(symt.kNoPrio, symt.UnaryOpPrio(AGS::kKW_Divide));

    EXPECT_NE(symt.kNoPrio, symt.BinaryOpPrio(AGS::kKW_Minus));
    EXPECT_NE(symt.kNoPrio, symt.UnaryOpPrio(AGS::kKW_Minus));
}

TEST(SymbolTable, IsAnyIntegerVartype)
{
    AGS::SymbolTable symt;

    EXPECT_TRUE(symt.IsAnyIntegerVartype(AGS::kKW_Long));
    EXPECT_FALSE(symt.IsAnyIntegerVartype(AGS::kKW_Float));
}
