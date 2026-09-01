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
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>
#include "util/error.h"
#include "util/string.h"
#include "util/string_types.h"
#include "util/textstreamwriter.h"

namespace AGSBuild
{

using String = AGS::Common::String;
using HError = AGS::Common::HError;

enum class TaskState
{
    Pending,
    Running,
    Success,
    Failure,
    Cancelled
};

class Task;

class ITaskManager
{
public:
    virtual ~ITaskManager() {}

    // Adds new task to the list. Is safe to execute while the TaskManager is running tasks.
    virtual void AddTask(std::unique_ptr<Task> &&task) = 0;
};

struct TaskInput
{
    // Name of the input providing task, basically a prerequisite
    String OutputTask;
    // Provider task's output index (where to get data from), UINT32_MAX means NONE
    uint32_t OutputIndex = UINT32_MAX;
    // This task's input index (where to put data to), UINT32_MAX means NONE
    uint32_t InputIndex = UINT32_MAX;

    TaskInput(const String &out_task, uint32_t out_index = UINT32_MAX, uint32_t in_index = UINT32_MAX)
        : OutputTask(out_task), OutputIndex(out_index), InputIndex(in_index) {}

    TaskInput Clone() const
    {
        return TaskInput(OutputTask.Clone(), OutputIndex, InputIndex);
    }
};

// Makes a deep copy of a vector of strings (copy the vector, and explicitly copy each string)
std::vector<String> Clone(const std::vector<String> &v);
// Makes a deep copy of a vector of string pairs (copy the vector, and explicitly copy each string)
std::vector<std::pair<String, String>> Clone(const std::vector<std::pair<String, String>> &v);
// Makes a deep copy of a vector of TaskInput (copy the vector, and deep copy TaskInput, copying each of its fields)
std::vector<TaskInput> Clone(const std::vector<TaskInput> &v);

class Task
{
public:
    Task(const String &name, ITaskManager *mgr);
    Task(const String &name, ITaskManager *mgr, const std::vector<TaskInput> &input_tasks);
    virtual ~Task() {}

    const String &GetName() const { return _name; }
    TaskState GetState() const;
    const std::vector<TaskInput> &GetInputs() const { return _inputs; }
    HError GetResult() const;
    std::vector<String> GetOutputData() const;
    String GetLogBuffer() const;

    void Cancel();
    void SetInputData(uint32_t in_index, const String &data);

    HError Run();

protected:
    virtual HError RunImpl() { return HError::None(); }

    const String _name;
    ITaskManager *const _mgr = nullptr;
    TaskState _state = TaskState::Pending;
    HError _result = HError::None();
    const std::vector<TaskInput> _inputs;
    std::vector<String> _inputData;
    std::vector<String> _outputData;
    std::unique_ptr<AGS::Common::TextStreamWriter> _logWriter;
    mutable std::vector<uint8_t> _logBuffer;
    mutable std::mutex _mutex;
};

class TaskStub : public Task
{
public:
    TaskStub(const String &name, ITaskManager *mgr)
        : Task(name, mgr) {}
    TaskStub(const String &name, ITaskManager *mgr, const std::vector<TaskInput> &input_tasks)
        : Task(name, mgr, input_tasks) {}
};

enum class FileMoveOp
{
    Move,
    Copy,
    Hardlink
};

class TaskMoveFiles : public Task
{
public:
    TaskMoveFiles(const String &name, ITaskManager *mgr, const std::vector<std::pair<String, String>> &files,
        FileMoveOp file_op, bool skip_if_no_src)
        : Task(name, mgr)
        , _files(Clone(files))
        , _fileOp(file_op)
        , _skipIfNoSrc(skip_if_no_src)
    {}
    TaskMoveFiles(const String &name, ITaskManager *mgr, const std::vector<std::pair<String, String>> &files,
        FileMoveOp file_op, bool skip_if_no_src, const std::vector<TaskInput> &input_tasks)
        : Task(name, mgr, input_tasks)
        , _files(Clone(files))
        , _fileOp(file_op)
        , _skipIfNoSrc(skip_if_no_src)
    {}

private:
    HError RunImpl() override;

    std::vector<std::pair<String, String>> _files;
    FileMoveOp _fileOp = FileMoveOp::Move;
    bool _skipIfNoSrc = false;
};

class TaskDeleteFiles : public Task
{
public:
    TaskDeleteFiles(const String &name, ITaskManager *mgr, const std::vector<String> &files)
        : Task(name, mgr), _files(files) {}
    TaskDeleteFiles(const String &name, ITaskManager *mgr, const std::vector<String> &files, const std::vector<TaskInput> &input_tasks)
        : Task(name, mgr, input_tasks), _files(files) {}
private:
    HError RunImpl() override;

    std::vector<String> _files;
};

class TaskWriteStringList : public Task
{
public:
    TaskWriteStringList(const String &name, ITaskManager *mgr, const String &filepath, std::vector<String> &list)
        : Task(name, mgr)
        , _filepath(filepath.Clone())
        , _list(Clone(list))
    {}
    TaskWriteStringList(const String &name, ITaskManager *mgr, const String &filepath, std::vector<String> &list,
        const std::vector<TaskInput> &input_tasks)
        : Task(name, mgr, input_tasks)
        , _filepath(filepath.Clone())
        , _list(Clone(list))
    {}

private:
    HError RunImpl() override;

    String _filepath;
    std::vector<String> _list;
};

struct PipedProcessParams
{
    String ExePath;
    String Params;
    bool OutputToStdOut = false;

    PipedProcessParams() = default;
    PipedProcessParams(const String &exe_path, const String &params, bool out_to_std = false)
        : ExePath(exe_path), Params(params), OutputToStdOut(out_to_std) {}
};

class TaskPipedProcess : public Task
{
public:
    TaskPipedProcess(const String &name, ITaskManager *mgr, const PipedProcessParams &params);
    TaskPipedProcess(const String &name, ITaskManager *mgr, const PipedProcessParams &params,
        const std::vector<TaskInput> &input_tasks);

private:
    HError RunImpl() override;

    const PipedProcessParams _params;
};

class TaskManager : public ITaskManager
{
public:
    TaskManager(uint32_t max_threads, bool verbose = false);

    void AddTask(std::unique_ptr<Task> &&task) override;
    HError RunAll();

private:
    void RunReadyTasks();
    bool UpdateTaskState(bool &has_any_task_succeed);
    void CancelTask(Task *task, const String &parent_task, std::vector<String> &cancel_tasks);
    void CancelTaskDependents(const String &task_name, std::vector<String> &cancel_tasks);
    void MoveTasks(std::vector<String> &tasks, std::unordered_map<String, Task*> &from, std::unordered_map<String, Task*> &to);

    struct ThreadData
    {
        uint32_t Slot = 0u;
        AGSBuild::Task *Task = nullptr;
        std::thread Thread;

        ThreadData(uint32_t slot) : Slot(slot) {}
    };

    // Main tasks collection, used for lifetime control and general lookup
    std::unordered_map<String, std::unique_ptr<Task>> _allTasks;
    // Lookup collection of tasks awaiting to be run
    std::unordered_map<String, Task*> _pendingTasks;
    // Lookup collection of the currently running tasks
    std::unordered_map<String, Task*> _runTasks;
    // Lookup collection of the finished tasks (successful, failed or cancelled)
    std::unordered_map<String, Task*> _doneTasks;
    std::vector<ThreadData> _threads;
    uint32_t _freeThreads = 0u;
    uint32_t _unnamedTaskCounter = 0u;
    bool _verbose = false;
    std::mutex _mutex;
};

void ReadStringListFromStdOut(const String &input_s, std::vector<String> &list);
HError ReadStringListFromTextFile(const String &filename, std::vector<String> &list);
HError WriteStringListToTextFile(const String &filename, const std::vector<String> &list);

} // namespace AGSBuild
