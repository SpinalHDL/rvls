#include "snapshot.hpp"
#include "hart.hpp"

void ContextHartsManager::add(int id, Hart& hart) {
    std::lock_guard<std::mutex> lock(mutex);
    harts.insert_or_assign(id, std::ref(hart));
}

bool ContextHartsManager::remove(int id) {
    std::lock_guard<std::mutex> lock(mutex);
    return harts.erase(id) != 0;
}

std::optional<std::reference_wrapper<Hart>> ContextHartsManager::find(int id) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = harts.find(id);
    if(it == harts.end()) return std::nullopt;
    return it->second;
}

ContextHartsManager& getContextHartsManager() {
    static ContextHartsManager manager;
    return manager;
}

std::string appendFailureContext(int hartId, std::string message) {
    try {
        auto hart = getContextHartsManager().find(hartId);
        if(hart) {
            message += "\n\n";
            message += hart->get().formatFailureContext();
        }
    } catch (...) {
    }
    return message;
}
