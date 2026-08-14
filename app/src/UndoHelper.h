#pragma once

#include <QUndoCommand>
#include <QString>
#include <functional>

namespace beta {

/// A `Fun` is a callable that performs (or reverses) an atomic model
/// mutation and returns true on success. Ported from Kdenlive's
/// `undohelper.hpp` — see:
///   https://github.com/KDE/kdenlive/blob/master/src/undohelper.hpp
///
/// The Kdenlive architecture builds undo/redo lambdas AS the model
/// mutates state, then pushes a single `FunctionalUndoCommand`
/// carrying both lambdas. This makes it trivial to compose complex
/// operations (cut = clone + resize + move) into one undo step.
using Fun = std::function<bool(void)>;

/// Compose `operation` AFTER `lambda` (operation runs last on redo,
/// first on undo). Used to accumulate steps into a single chain.
///   lambda = lambda && operation
#define PUSH_LAMBDA(operation, lambda) \
    lambda = [lambda, operation]() { \
        bool v = lambda(); \
        return v && operation(); \
    };

/// Compose `operation` BEFORE `lambda` (operation runs first on redo,
/// last on undo).
#define PUSH_FRONT_LAMBDA(operation, lambda) \
    lambda = [lambda, operation]() { \
        bool v = operation(); \
        return v && lambda(); \
    };

/// A generic `QUndoCommand` that wraps two `Fun` closures.
///
/// The `m_undone` flag is the key trick: Qt's `QUndoStack::push()`
/// immediately calls `redo()` on the command. But in the Kdenlive
/// pattern, the model has *already performed* the change by the time
/// the lambdas are pushed (it executes eagerly so it can detect
/// failure and roll back). So the first automatic `redo()` from
/// `push()` must be a no-op. Subsequent `redo()` calls (after an
/// `undo()`) genuinely re-execute the lambda.
class FunctionalUndoCommand : public QUndoCommand {
public:
    FunctionalUndoCommand(Fun undo, Fun redo, const QString& text,
                          QUndoCommand* parent = nullptr)
        : QUndoCommand(parent)
        , m_undo(std::move(undo))
        , m_redo(std::move(redo))
        , m_undone(false)
    {
        setText(text);
    }

    void undo() override
    {
        m_undone = true;
        bool res = m_undo();
        Q_ASSERT(res);
        Q_UNUSED(res);
        QUndoCommand::undo();
    }

    void redo() override
    {
        if (m_undone) {
            // Only re-run m_redo if we've previously been undone.
            bool res = m_redo();
            Q_ASSERT(res);
            Q_UNUSED(res);
        }
        QUndoCommand::redo();
    }

private:
    Fun  m_undo;
    Fun  m_redo;
    bool m_undone;
};

} // namespace beta
