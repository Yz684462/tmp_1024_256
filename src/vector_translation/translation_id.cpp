#include "vector_translation.h"
#include <unistd.h>

namespace BinaryTranslation {
namespace TranslationId {

// Initialize static counter
int TranslationIdManager::translation_id_counter_ = 0;

TranslationIdManager& TranslationIdManager::getInstance() {
    static TranslationIdManager instance;
    return instance;
}

int TranslationIdManager::get_current_translation_id() {
    pid_t current_pid = getpid();
    
    // First try to find without lock
    auto it = pid_tid_map_.find(current_pid);
    if (it != pid_tid_map_.end()) {
        return it->second;
    }
    
    // If not found, acquire lock and try again
    std::lock_guard<std::mutex> lock(map_mutex_);
    
    // If still not found, assign new translation ID
    int new_id = translation_id_counter_++;
    pid_tid_map_[current_pid] = new_id;

    // release lock
    lock.unlock();
    
    return new_id;
}

} // namespace TranslationId
} // namespace BinaryTranslation