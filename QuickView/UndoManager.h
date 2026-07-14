#pragma once
#include <string>
#include <vector>
#include "LosslessTransform.h"

/// <summary>
/// Supported actions that can be undone
/// </summary>
enum class UndoType : uint8_t {
    None = 0,
    Delete,
    Rename,
    Transform
};

/// <summary>
/// Context data for a single undoable operation
/// </summary>
struct UndoAction {
    UndoType type = UndoType::None;
    std::wstring path;
    std::wstring oldPath; // For Rename: original path
    std::vector<TransformType> reverseTransforms; // For Transform: reverse transforms to apply
    bool leftSlot = false; // Used in compare mode to track which slot was deleted
};

/// <summary>
/// Lightweight, zero-allocation-hot-path manager for Undo history
/// </summary>
class UndoManager {
private:
    std::vector<UndoAction> m_undoStack;
    static constexpr size_t MAX_UNDO_DEPTH = 32;

public:
    void PushDelete(const std::wstring& path, bool leftSlot) {
        if (m_undoStack.size() >= MAX_UNDO_DEPTH) {
            m_undoStack.erase(m_undoStack.begin());
        }
        UndoAction action;
        action.type = UndoType::Delete;
        action.path = path;
        action.leftSlot = leftSlot;
        m_undoStack.push_back(action);
    }

    void PushRename(const std::wstring& oldPath, const std::wstring& newPath, bool leftSlot) {
        if (m_undoStack.size() >= MAX_UNDO_DEPTH) {
            m_undoStack.erase(m_undoStack.begin());
        }
        UndoAction action;
        action.type = UndoType::Rename;
        action.path = newPath;     // Current new path
        action.oldPath = oldPath;   // Original old path
        action.leftSlot = leftSlot;
        m_undoStack.push_back(action);
    }

    void PushTransform(const std::wstring& path, const std::vector<TransformType>& reverseTransforms, bool leftSlot) {
        if (m_undoStack.size() >= MAX_UNDO_DEPTH) {
            m_undoStack.erase(m_undoStack.begin());
        }
        UndoAction action;
        action.type = UndoType::Transform;
        action.path = path;
        action.reverseTransforms = reverseTransforms;
        action.leftSlot = leftSlot;
        m_undoStack.push_back(action);
    }

    bool CanUndo() const noexcept {
        return !m_undoStack.empty();
    }

    UndoAction Pop() noexcept {
        if (m_undoStack.empty()) return {};
        UndoAction action = m_undoStack.back();
        m_undoStack.pop_back();
        return action;
    }

    void Clear() noexcept {
        m_undoStack.clear();
    }

    UndoType GetLastActionType() const noexcept {
        if (m_undoStack.empty()) return UndoType::None;
        return m_undoStack.back().type;
    }
};

