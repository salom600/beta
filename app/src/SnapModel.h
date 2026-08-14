#pragma once

#include <map>
#include <vector>
#include <cstdint>

namespace beta {

/// A snap-point store with refcounting. Ported from Kdenlive's
/// `SnapModel` — see:
///   https://github.com/KDE/kdenlive/blob/master/src/timeline2/model/snapmodel.hpp
///
/// The keys are frame positions; the values are reference counts.
/// Multiple sources (clip end + guide + marker at the same frame)
/// each increment the counter, and the point only disappears when
/// all sources have removed it. `std::map<int,int>` is ordered
/// (unlike QMap which is NOT ordered by iteration), so
/// `getClosestPoint` is O(log n) via `lower_bound`.
class SnapModel {
public:
    /// Add a snap point (increments refcount if already present).
    void addPoint(int position)
    {
        m_snaps[position]++;
    }

    /// Remove a snap point (decrements refcount; erases when it hits 0).
    void removePoint(int position)
    {
        auto it = m_snaps.find(position);
        if (it == m_snaps.end()) return;
        if (--(it->second) == 0) {
            m_snaps.erase(it);
        }
    }

    /// Returns the closest snap point to `position`, or -1 if empty.
    int getClosestPoint(int position) const
    {
        if (m_snaps.empty()) return -1;
        auto it = m_snaps.lower_bound(position);
        if (it == m_snaps.end()) {
            // position is past the last snap — return the last one
            return std::prev(m_snaps.end())->first;
        }
        if (it->first == position) return position;
        if (it == m_snaps.begin()) return it->first;
        auto prev = std::prev(it);
        // Pick the closer of prev and it
        if (position - prev->first <= it->first - position) {
            return prev->first;
        }
        return it->first;
    }

    /// Returns the next snap point strictly after `position`, or
    /// `position` itself if there is none.
    int getNextPoint(int position) const
    {
        auto it = m_snaps.upper_bound(position);
        if (it == m_snaps.end()) return position;
        return it->first;
    }

    /// Returns the previous snap point strictly before `position`, or
    /// 0 if there is none.
    int getPreviousPoint(int position) const
    {
        if (m_snaps.empty()) return 0;
        auto it = m_snaps.lower_bound(position);
        if (it == m_snaps.begin()) return 0;
        return std::prev(it)->first;
    }

    /// Temporarily ignore a set of points (used during drag so a clip
    /// doesn't snap to its own edges). Restored by `unIgnore`.
    void ignore(const std::vector<int>& pts)
    {
        for (int p : pts) {
            auto it = m_snaps.find(p);
            if (it != m_snaps.end()) {
                m_ignored.push_back({p, it->second});
                m_snaps.erase(it);
            }
        }
    }

    void unIgnore()
    {
        for (const auto& [pos, count] : m_ignored) {
            m_snaps[pos] = count;
        }
        m_ignored.clear();
    }

    bool isEmpty() const { return m_snaps.empty(); }
    size_t size() const { return m_snaps.size(); }
    void clear() { m_snaps.clear(); m_ignored.clear(); }

private:
    std::map<int, int>                 m_snaps;
    std::vector<std::pair<int, int>>   m_ignored;
};

} // namespace beta
