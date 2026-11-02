//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Machine-readable output of agsbuild (docs/refactor/PLAN.md, 7.1h).
//
// With --json, agsbuild writes one JSON object per line to stdout, and its
// usual log goes to stderr. The objects, told apart by "event":
//   {"event":"start","tasks":N}
//   {"event":"task","task":"agscc:foo.asc","state":"running|success|failed|cancelled"}
//   {"event":"diagnostic","severity":"error|warning|info","tool":"agscc","task":"agscc:foo.asc",
//    "file":"foo.asc","line":12,"message":"..."}      ("file" and "line" only when the tool gave them)
//   {"event":"progress","done":K,"total":N}
//   {"event":"end","result":"success|failed|cancelled","errors":E,"warnings":W}
//
// The tools print plain text; diagnostics are read from their output, which has this form:
//   Error: <message>
//   Warning: <message>
//   Error: <what> at <file>, line <N> : <message>        (agscc, also "Warning: issue at ...")
// A line starting with "error:", "warning:" or "info:" in any letter case counts.
//
// A build is cancelled by SIGINT / SIGTERM (Ctrl+C on Windows), or in --json mode by the line
// "cancel" on stdin. Running tools are killed, waiting tasks do not start.
//
//=============================================================================
#ifndef __AGSBUILD_DIAGNOSTICS_H
#define __AGSBUILD_DIAGNOSTICS_H

#include <string>
#include <vector>

namespace AGSBuild
{
namespace Diag
{

enum class Severity
{
    Info,
    Warning,
    Error
};

struct Diagnostic
{
    Severity Level = Severity::Error;
    std::string Tool;
    std::string Task;
    std::string File; // empty if not known
    int Line = 0;     // 0 if not known
    std::string Message;
};

// Reads a diagnostic from one line of tool output; returns false if the line is not one.
// Tool and Task are left for the caller to fill.
bool ParseDiagnosticLine(const std::string &line, Diagnostic &diag);
// All the diagnostics in a block of output
std::vector<Diagnostic> ExtractDiagnostics(const std::string &text);

// JSON text of a string (with quotes). Bytes that are not valid UTF-8 are replaced with '?'.
std::string JsonString(const std::string &s);
std::string ToJson(const Diagnostic &diag);

// Selects the output mode (see above). Off by default.
void SetJsonOutput(bool on);
bool IsJsonOutput();
// The usual log: stdout, or stderr in JSON mode
void Log(const char *fmt, ...);
// Writes one JSON line to stdout, if the JSON mode is on
void EmitEvent(const std::string &json_line);
// Event helpers; all of them do nothing in the plain mode (but diagnostics are still counted)
void EmitStart(size_t tasks);
void EmitTask(const std::string &task, const char *state);
void EmitProgress(size_t done, size_t total);
void EmitDiagnostic(const Diagnostic &diag);
void EmitEnd(const char *result);

// Number of errors and warnings emitted so far
int ErrorCount();
int WarningCount();

// Cancelling: the flag is set from a signal handler or another thread, and polled by the build
void RequestCancel();
bool IsCancelRequested();
// Clears the flag; for tests
void ResetCancel();
// Installs the signal handlers; in JSON mode also starts reading "cancel" from stdin
void InstallCancelHandlers();

} // namespace Diag
} // namespace AGSBuild

#endif // __AGSBUILD_DIAGNOSTICS_H
