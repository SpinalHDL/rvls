#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <optional>

class Hart;

class ContextHartsManager {
public:
    void add(int id, Hart& hart);
    bool remove(int id);
    std::optional<std::reference_wrapper<Hart>> find(int id);

private:
    std::mutex mutex;
    std::map<int, std::reference_wrapper<Hart>> harts;
};

ContextHartsManager& getContextHartsManager();
