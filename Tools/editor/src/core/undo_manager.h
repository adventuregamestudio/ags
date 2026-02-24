// AGS Editor ImGui - Undo/Redo Manager
// Provides a centralized undo system with per-context stacks.
// Each editor pane registers as a context; the active context's
// stack is used when the user presses Ctrl+Z / Ctrl+Y.
#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace AGSEditor
{

// A single undoable command
struct UndoCommand
{
    std::string description;
    std::function<void()> undo_fn;
    std::function<void()> redo_fn;
};

// An undo context represents one undo/redo stack (e.g., per-pane)
class UndoContext
{
public:
    explicit UndoContext(const std::string& name) : name_(name) {}

    void PushCommand(const std::string& desc,
                     std::function<void()> undo_fn,
                     std::function<void()> redo_fn)
    {
        // Discard redo history past current position
        if (pos_ + 1 < (int)stack_.size())
            stack_.erase(stack_.begin() + pos_ + 1, stack_.end());

        stack_.push_back({desc, std::move(undo_fn), std::move(redo_fn)});
        pos_ = (int)stack_.size() - 1;

        // Trim to max
        if ((int)stack_.size() > kMaxUndo)
        {
            stack_.erase(stack_.begin());
            pos_--;
        }
    }

    bool CanUndo() const { return pos_ >= 0; }
    bool CanRedo() const { return pos_ + 1 < (int)stack_.size(); }

    std::string GetUndoDescription() const
    {
        return CanUndo() ? stack_[pos_].description : "";
    }

    std::string GetRedoDescription() const
    {
        return CanRedo() ? stack_[pos_ + 1].description : "";
    }

    void Undo()
    {
        if (!CanUndo()) return;
        stack_[pos_].undo_fn();
        pos_--;
    }

    void Redo()
    {
        if (!CanRedo()) return;
        pos_++;
        stack_[pos_].redo_fn();
    }

    void Clear()
    {
        stack_.clear();
        pos_ = -1;
    }

    const std::string& GetName() const { return name_; }
    int GetStackSize() const { return (int)stack_.size(); }
    int GetPosition() const { return pos_; }

private:
    static constexpr int kMaxUndo = 200;
    std::string name_;
    std::vector<UndoCommand> stack_;
    int pos_ = -1;
};

// Central undo manager — holds all contexts, tracks the active one
class UndoManager
{
public:
    UndoManager() = default;

    // Create a new undo context (e.g., for a pane)
    UndoContext* CreateContext(const std::string& name)
    {
        contexts_.push_back(std::make_unique<UndoContext>(name));
        return contexts_.back().get();
    }

    // Remove a context (e.g., when a pane closes)
    void RemoveContext(UndoContext* ctx)
    {
        if (active_context_ == ctx)
            active_context_ = nullptr;
        contexts_.erase(
            std::remove_if(contexts_.begin(), contexts_.end(),
                [ctx](const std::unique_ptr<UndoContext>& c) { return c.get() == ctx; }),
            contexts_.end());
    }

    // Set which context is currently active (the focused pane)
    void SetActiveContext(UndoContext* ctx) { active_context_ = ctx; }
    UndoContext* GetActiveContext() const { return active_context_; }

    // Convenience — undo/redo on the active context
    bool CanUndo() const { return active_context_ && active_context_->CanUndo(); }
    bool CanRedo() const { return active_context_ && active_context_->CanRedo(); }

    std::string GetUndoDescription() const
    {
        return active_context_ ? active_context_->GetUndoDescription() : "";
    }

    std::string GetRedoDescription() const
    {
        return active_context_ ? active_context_->GetRedoDescription() : "";
    }

    void Undo()
    {
        if (active_context_) active_context_->Undo();
    }

    void Redo()
    {
        if (active_context_) active_context_->Redo();
    }

private:
    std::vector<std::unique_ptr<UndoContext>> contexts_;
    UndoContext* active_context_ = nullptr;
};

} // namespace AGSEditor
