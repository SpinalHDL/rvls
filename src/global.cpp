#include "context.hpp"
#include "hart.hpp"

std::string appendFailureContext(Context& context, int hartId, std::string message) {
    try {
        auto hart = context.harts.find(hartId);
        if(hart != context.harts.end()) {
            message += "\n\n";
            message += hart->second.formatFailureContext();
        }
    } catch (...) {
    }
    return message;
}
