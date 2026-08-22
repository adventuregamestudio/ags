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
#include "taskmanager.h"
#include "pipedprocess.h"
#include "util/file.h"
#include "util/memorystream.h"
#include "util/path.h"
#include "util/textstreamreader.h"
#include "util/textstreamwriter.h"

using namespace AGS::Common;

namespace AGSBuild
{

std::vector<String> Clone(const std::vector<String> &v)
{
    std::vector<String> copy;
    for (const auto &s : v)
        copy.push_back(s.Clone());
    return copy;
}

std::vector<std::pair<String, String>> Clone(const std::vector<std::pair<String, String>> &v)
{
    std::vector<std::pair<String, String>> copy;
    for (const auto &s : v)
        copy.push_back(std::make_pair(s.first, s.second));
    return copy;
}

std::vector<TaskInput> Clone(const std::vector<TaskInput> &v)
{
    std::vector<TaskInput> copy;
    for (const auto &s : v)
        copy.push_back(s.Clone());
    return copy;
}

void ReadStringListFromStdOut(const String &input_s, std::vector<String> &input_list)
{
    String uni_input_s = input_s;
    uni_input_s.Replace("\r\n", "\n");
    auto list = uni_input_s.Split("\n");
    list.erase(
        std::remove_if(list.begin(), list.end(), [](const String &s){return s.IsEmpty();}),
        list.end());
    input_list.insert(input_list.end(), list.begin(), list.end());
}

HError ReadStringListFromTextFile(const String &filename, std::vector<String> &list)
{
    auto in = File::OpenFileRead(filename);
    if (!in)
        return new Error(String::FromFormat("Failed to open file for reading: %s", filename.GetCStr()));
    auto text_reader = TextStreamReader(std::move(in));
    for (String line = text_reader.ReadLine(); !line.IsEmpty(); line = text_reader.ReadLine())
        list.push_back(line);
    return HError::None();
}

HError WriteStringListToTextFile(const String &filename, const std::vector<String> &list)
{
    auto out = File::CreateFile(filename);
    if (!out)
        return new Error(String::FromFormat("Failed to open file for writing: %s", filename.GetCStr()));
    auto text_writer = TextStreamWriter(std::move(out));
    for (const auto &line : list)
        text_writer.WriteLine(line);
    return HError::None();
}

//-----------------------------------------------------------------------------
// Task
//-----------------------------------------------------------------------------

Task::Task(const String &name, ITaskManager *mgr)
    : _name(name.Clone())
    , _mgr(mgr)
{
    _logWriter.reset(new TextStreamWriter(std::make_unique<Stream>(std::make_unique<VectorStream>(_logBuffer, kStream_Write))));
}

Task::Task(const String &name, ITaskManager *mgr, const std::vector<TaskInput> &input_tasks)
    : _name(name.Clone())
    , _mgr(mgr)
    , _inputs(Clone(input_tasks))
{
    _inputData.resize(_inputs.size());
    _logWriter.reset(new TextStreamWriter(std::make_unique<Stream>(std::make_unique<VectorStream>(_logBuffer, kStream_Write))));
}

TaskState Task::GetState() const
{
    std::lock_guard<std::mutex> lk(_mutex);
    return _state;
}

HError Task::GetResult() const
{
    std::lock_guard<std::mutex> lk(_mutex);
    return _result;
}

std::vector<String> Task::GetOutputData() const
{
    std::lock_guard<std::mutex> lk(_mutex);
    return Clone(_outputData);
}

String Task::GetLogBuffer() const
{
    std::lock_guard<std::mutex> lk(_mutex);
    _logBuffer.push_back(0); // for safety
    return String(reinterpret_cast<const char*>(_logBuffer.data()));
}

void Task::Cancel()
{
    std::lock_guard<std::mutex> lk(_mutex);
    _state = TaskState::Cancelled;
}

void Task::SetInputData(uint32_t in_index, const String &data)
{
    std::lock_guard<std::mutex> lk(_mutex);
    if (in_index < _inputs.size())
    {
        // Clone string in order to avoid problems with threads (String has a ref-counted data) 
        _inputData[in_index] = data.Clone();
    }
}

HError Task::Run()
{
    {
        std::lock_guard<std::mutex> lk(_mutex);
        _state = TaskState::Running;
    }
    _logWriter->WriteFormat("Task: %s\n", _name.GetCStr());
    HError err = RunImpl();
    {
        std::lock_guard<std::mutex> lk(_mutex);
        _state = err ? TaskState::Success : TaskState::Failure;
        if (_state == TaskState::Success)
            _logWriter->WriteFormat("Success");
        else
            _logWriter->WriteFormat("Failure: %s\n", err->FullMessage().GetCStr());
    }
    return err;
}

//-----------------------------------------------------------------------------
// TaskMoveFiles
//-----------------------------------------------------------------------------

HError TaskMoveFiles::RunImpl()
{
    for (const auto item : _files)
    {
        const String src_filepath = item.first;
        const String dst_filepath = item.second;

        if (File::IsFile(src_filepath))
        {
            bool success = false;
            if (_doCopy)
                success = File::CopyFile(src_filepath, dst_filepath, true);
            else
                success = File::RenameFile(src_filepath, dst_filepath);

            if (success)
            {
                _logWriter->WriteFormat("+ %s: %s -> %s\n", _doCopy ? "Copy" : "Move", src_filepath.GetCStr(), dst_filepath.GetCStr());
            }
            else
            {
                return new Error(String::FromFormat("Failed to %s file %s to %s", _doCopy ? "copy" : "move", src_filepath.GetCStr(), dst_filepath.GetCStr()));
            }
        }
        else if (_skipIfNoSrc)
        {
            _logWriter->WriteFormat("Skip: no source file %s\n", src_filepath.GetCStr());
        }
        else
        {
            return new Error(String::FromFormat("Source file %s does not exist", src_filepath.GetCStr()));
        }
    }

    return HError::None();
}

//-----------------------------------------------------------------------------
// TaskWriteStringList
//-----------------------------------------------------------------------------

HError TaskWriteStringList::RunImpl()
{
    return WriteStringListToTextFile(_filepath, _list);
}

//-----------------------------------------------------------------------------
// TaskPipedProcess
//-----------------------------------------------------------------------------

TaskPipedProcess::TaskPipedProcess(const String &name, ITaskManager *mgr, const PipedProcessParams &params)
    : Task(name, mgr)
    , _params(params)
{
}

TaskPipedProcess::TaskPipedProcess(const String &name, ITaskManager *mgr, const PipedProcessParams &params,
    const std::vector<TaskInput> &input_tasks)
    : Task(name, mgr, input_tasks)
    , _params(params)
{
}

HError TaskPipedProcess::RunImpl()
{
    _logWriter->WriteFormat("Running: %s %s\n", _params.ExePath.GetCStr(), _params.Params.GetCStr());

    std::vector<uint8_t> data;
    PipedProcess pp(_params.ExePath, _params.Params, _logWriter.get());
    HError err = pp.RunSync(_params.OutputToStdOut ? &data : nullptr);
    if (!err)
        _logWriter->WriteFormat("Process failed with error:\n%s", err->FullMessage().GetCStr());

    {
        std::lock_guard<std::mutex> lk(_mutex);

        if (_params.OutputToStdOut)
        {
            data.push_back(0); // safety fix
            _outputData.push_back(String(reinterpret_cast<const char*>(data.data())));
            /* TODO: ?
            // We assume that the output is a list of strings, separated by linebreaks.
            // Then we assume that there's a second kind of separator: an empty line;
            // and this separator divides the list in groups, each group goes into its
            // own output.
            String data_s = reinterpret_cast<const char*>(data.data());
            data_s.Replace("\r\n", "\n");
            _outputData = data_s.Split("\n\n"); // split by two consecutive linebreaks
            // Edge case: first group is empty
            if (data_s.GetLength() > 0 && data_s[0] == '\n')
                _outputData.insert(_outputData.begin(), String());
            */
        }
    }

    if (err && pp.GetExitCode() != 0)
        err = new Error(String::FromFormat("Process exited with error code %d", pp.GetExitCode()));
    return err;
}

//-----------------------------------------------------------------------------
// TaskManager
//-----------------------------------------------------------------------------

TaskManager::TaskManager(uint32_t max_threads)
{
    assert(max_threads > 0);
    _threads.resize(max_threads);
    _freeThreads = max_threads;
}

void TaskManager::AddTask(std::unique_ptr<Task> &&task)
{
    assert(task && !task->GetName().IsEmpty());
    if (task)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        String name = task->GetName();
        if (name.IsEmpty())
            name = String::FromFormat("__%u", _unnamedTaskCounter++);

        if (_allTasks.count(name) == 0)
        {
            _pendingTasks[name] = task.get();
            _allTasks[name] = std::move(task);
        }
    }
}

HError TaskManager::RunAll()
{
    // Begin with running as much tasks as possible
    RunReadyTasks();

    bool want_stop = false;
    while (!want_stop)
    {
        bool has_any_task_finished = false;
        want_stop = !UpdateTaskState(has_any_task_finished);
        if (has_any_task_finished)
            RunReadyTasks();
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    };

    uint32_t failed_tasks = 0u;
    for (const auto &task : _allTasks)
        if (task.second->GetState() != TaskState::Success)
            failed_tasks++;
    return failed_tasks == 0 ? HError::None() : new Error(String::FromFormat("%d task(s) failed to complete", failed_tasks));
}

static void RunTaskOnThread(Task *task)
{
    assert(task);
    if (task)
        task->Run();
}

void TaskManager::RunReadyTasks()
{
    std::lock_guard<std::mutex> lk(_mutex);
    if (_freeThreads == 0u)
        return;

    std::vector<String> start_tasks;
    std::vector<String> cancel_tasks;
    for (auto &task : _pendingTasks)
    {
        // TODO: test if any prerequisites do not exist at all, and cancel dependent tasks automatically

        const auto &inputs = task.second->GetInputs();
        bool is_ready_to_run = true;
        bool auto_cancel = false;
        for (const auto &input : inputs)
        {
            if (input.OutputTask.IsEmpty())
                continue; // not expected, just ignore

            // If prerequisite is not available, then cancel this task
            if (_allTasks.count(input.OutputTask) == 0)
            {
                is_ready_to_run = false;
                auto_cancel = true;
            }
            else
            {
                const auto it_found = _doneTasks.find(input.OutputTask);
                if (it_found != _doneTasks.end())
                {
                    is_ready_to_run &= (it_found->second->GetState() == TaskState::Success);
                    auto_cancel |= (it_found->second->GetState() != TaskState::Success);
                }
                else
                {
                    is_ready_to_run = false; // prerequisite task is not complete yet
                }
            }

            if (!is_ready_to_run)
                break;
        }

        if (auto_cancel)
        {
            task.second->Cancel();
            cancel_tasks.push_back(task.first);
        }
        else if (is_ready_to_run)
        {
            // Find free thread slot
            for (auto &slot : _threads)
            {
                if (slot.Task == nullptr)
                {
                    _freeThreads--;
                    slot.Task = task.second;
                    slot.Thread = std::thread(RunTaskOnThread, slot.Task);
                    start_tasks.push_back(task.first);
                    break;
                }
            }
        }

        // No more free thread slots
        if (_freeThreads == 0u)
            break;
    }

    for (const auto &name : start_tasks)
    {
        _runTasks[name] = std::move(_pendingTasks[name]);
        _pendingTasks.erase(name);
    }

    for (const auto &name : cancel_tasks)
    {
        _runTasks[name] = std::move(_pendingTasks[name]);
        _pendingTasks.erase(name);
    }
}

bool TaskManager::UpdateTaskState(bool &has_any_task_finished)
{
    std::lock_guard<std::mutex> lk(_mutex);
    for (auto &slot : _threads)
    {
        if (slot.Task != nullptr)
        {
            Task *task = slot.Task;
            const String &name = task->GetName();
            const TaskState state = task->GetState();

            // Print the accumulated log from the finished task
            if (state == TaskState::Success || state == TaskState::Failure)
            {
                const auto &log = task->GetLogBuffer();
                printf("-----------------------------------------------------------------------\n");
                printf("%s\n", log.GetCStr());
                printf("-----------------------------------------------------------------------\n");
            }

            if (state == TaskState::Success)
            {
                // If task has succeeded, then find all the dependent tasks,
                // and pass the received output data into their inputs.
                const auto &out_data = task->GetOutputData();
                if (out_data.size() != 0)
                {
                    for (auto &entry : _pendingTasks)
                    {
                        Task *pending_task = entry.second;
                        for (const auto &input : pending_task->GetInputs())
                        {
                            if ((input.OutputTask == name) && (input.OutputIndex < out_data.size()))
                                pending_task->SetInputData(input.InputIndex, out_data[input.OutputIndex]);
                        }
                    }
                }

                // Move this task to "done"
                _doneTasks[name] = std::move(_runTasks[name]);
                _runTasks.erase(name);
            }
            else if (state == TaskState::Failure || state == TaskState::Cancelled)
            {
                // If task failed or was cancelled, then find all the dependent tasks
                // and cancel them too.
                std::vector<String> cancel_tasks;
                for (auto &entry : _pendingTasks)
                {
                    Task *pending_task = entry.second;
                    const auto &inputs = pending_task->GetInputs();
                    if (std::find_if(inputs.begin(), inputs.end(), [name](const TaskInput &ti){ return ti.OutputTask == name;}) != inputs.end())
                    {
                        pending_task->Cancel();
                        cancel_tasks.push_back(pending_task->GetName());
                    }
                }

                // Move this task to "done"
                _doneTasks[name] = std::move(_runTasks[name]);
                _runTasks.erase(name);

                // Cancel dependent tasks and move them to "done"
                for (const auto &cancel_task : cancel_tasks)
                {
                    _doneTasks[name] = std::move(_pendingTasks[name]);
                    _pendingTasks.erase(name);
                }
            }

            // If the task is no longer running (for any reason), then stop the thread and free this slot
            if (state != TaskState::Running && state != TaskState::Pending)
            {
                if (slot.Thread.joinable())
                    slot.Thread.join();
                slot = {};
                _freeThreads++;
                has_any_task_finished = true;
            }
        }
    }

    return (_pendingTasks.size() + _runTasks.size()) > 0u;
}

} // namespace AGSBuild
