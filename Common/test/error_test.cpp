#include "gtest/gtest.h"
#include "util/error.h"

using namespace AGS::Common;

enum MyErrorCode
{
    kMyErr_None = 0,
    kMyErr_FileNotFound = 1,
    kMyErr_CorruptData = 2
};

static String GetMyErrorText(MyErrorCode code)
{
    switch (code)
    {
    case kMyErr_FileNotFound: return "File not found.";
    case kMyErr_CorruptData: return "Data is corrupt.";
    default: return "";
    }
}

typedef TypedCodeError<MyErrorCode, GetMyErrorText> MyError;

TEST(Error, DefaultConstructor)
{
    Error err;
    EXPECT_EQ(0, err.Code());
    EXPECT_TRUE(err.Message().IsEmpty());
    EXPECT_TRUE(err.FullMessage().IsEmpty());
    EXPECT_EQ(nullptr, err.PreviousError().get());
}

TEST(Error, StringConstructor)
{
    Error err(String("An error occurred."));
    EXPECT_EQ(0, err.Code());
    EXPECT_STREQ("An error occurred.", err.Message().GetCStr());
    EXPECT_STREQ("An error occurred.", err.FullMessage().GetCStr());
    EXPECT_EQ(nullptr, err.PreviousError().get());
}

TEST(Error, VariadicConstructor)
{
    Error err("Error at %s line %d: %s", "file.txt", 42, "syntax error");
    EXPECT_EQ(0, err.Code());
    EXPECT_STREQ("Error at file.txt line 42: syntax error", err.Message().GetCStr());
    EXPECT_STREQ("Error at file.txt line 42: syntax error", err.FullMessage().GetCStr());
}

TEST(Error, CodeAndStringConstructor)
{
    Error err(404, String("Not Found"));
    EXPECT_EQ(404, err.Code());
    EXPECT_STREQ("Not Found", err.Message().GetCStr());
    EXPECT_STREQ("Not Found", err.FullMessage().GetCStr());
}

TEST(Error, CodeAndVariadicConstructor)
{
    Error err(500, "Server error code %d: %s", 500, "internal");
    EXPECT_EQ(500, err.Code());
    EXPECT_STREQ("Server error code 500: internal", err.Message().GetCStr());
    EXPECT_STREQ("Server error code 500: internal", err.FullMessage().GetCStr());
}

TEST(Error, PreviousErrorStringConstructor)
{
    PError inner = std::make_shared<Error>("Disk full.");
    Error err(inner, String("Failed to save."));
    EXPECT_EQ(0, err.Code());
    EXPECT_STREQ("Failed to save.", err.Message().GetCStr());
    EXPECT_STREQ("Failed to save.\nDisk full.", err.FullMessage().GetCStr());
    EXPECT_EQ(inner.get(), err.PreviousError().get());
}

TEST(Error, PreviousErrorVariadicConstructor)
{
    PError inner = std::make_shared<Error>("Disk %s is full.", "C:");
    Error err(inner, "Failed to save slot %d.", 3);
    EXPECT_EQ(0, err.Code());
    EXPECT_STREQ("Failed to save slot 3.", err.Message().GetCStr());
    EXPECT_STREQ("Failed to save slot 3.\nDisk C: is full.", err.FullMessage().GetCStr());
    EXPECT_EQ(inner.get(), err.PreviousError().get());
}

TEST(Error, PreviousErrorCodeAndStringConstructor)
{
    PError inner = std::make_shared<Error>("Permission denied.");
    Error err(inner, 10, String("Failed to open file."));
    EXPECT_EQ(10, err.Code());
    EXPECT_STREQ("Failed to open file.", err.Message().GetCStr());
    EXPECT_STREQ("Failed to open file.\nPermission denied.", err.FullMessage().GetCStr());
}

TEST(Error, PreviousErrorCodeAndVariadicConstructor)
{
    PError inner = std::make_shared<Error>("Permission denied.");
    Error err(inner, 10, "Failed to open file: %s", "game.dat");
    EXPECT_EQ(10, err.Code());
    EXPECT_STREQ("Failed to open file: game.dat", err.Message().GetCStr());
    EXPECT_STREQ("Failed to open file: game.dat\nPermission denied.", err.FullMessage().GetCStr());
}

TEST(Error, ChainedErrors)
{
    PError err1 = std::make_shared<Error>("Layer 1 error");
    PError err2 = std::make_shared<Error>(err1, "Layer 2 error");
    Error err3(err2, "Layer 3 error");
    EXPECT_STREQ("Layer 3 error\nLayer 2 error\nLayer 1 error", err3.FullMessage().GetCStr());
}

TEST(TypedCodeError, BasicCodeOnly)
{
    MyError err(kMyErr_FileNotFound);
    EXPECT_EQ(kMyErr_FileNotFound, err.Code());
    EXPECT_STREQ("File not found.", err.Message().GetCStr());
    EXPECT_STREQ("File not found.", err.FullMessage().GetCStr());
}

TEST(TypedCodeError, CodeWithVariadicComment)
{
    MyError err(kMyErr_FileNotFound, "Looked in %s", "/path/to/file");
    EXPECT_EQ(kMyErr_FileNotFound, err.Code());
    EXPECT_STREQ("File not found.\nLooked in /path/to/file", err.Message().GetCStr());
    EXPECT_STREQ("File not found.\nLooked in /path/to/file", err.FullMessage().GetCStr());
}

TEST(TypedCodeError, PreviousErrorWithCode)
{
    PError inner = std::make_shared<Error>("OS error 2");
    MyError err(inner, kMyErr_FileNotFound);
    EXPECT_EQ(kMyErr_FileNotFound, err.Code());
    EXPECT_STREQ("File not found.", err.Message().GetCStr());
    EXPECT_STREQ("File not found.\nOS error 2", err.FullMessage().GetCStr());
}

TEST(TypedCodeError, PreviousErrorWithCodeAndVariadicComment)
{
    PError inner = std::make_shared<Error>("OS error 2");
    MyError err(inner, kMyErr_FileNotFound, "File: %s", "test.txt");
    EXPECT_EQ(kMyErr_FileNotFound, err.Code());
    EXPECT_STREQ("File not found.\nFile: test.txt", err.Message().GetCStr());
    EXPECT_STREQ("File not found.\nFile: test.txt\nOS error 2", err.FullMessage().GetCStr());
}
