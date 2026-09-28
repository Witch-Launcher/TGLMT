#include "tglmt/ObjectRegistry.h"
namespace tglmt {
void ObjectRegistry::Gen(ObjectKind k, GLsizei n, GLuint* ids) {
    std::lock_guard<std::mutex> l(mu_);
    for (GLsizei i = 0; i < n; ++i) { ids[i] = next_++; live_[k].insert(ids[i]); }
}
void ObjectRegistry::Create(ObjectKind k, GLsizei n, GLuint* ids) { Gen(k, n, ids); }
void ObjectRegistry::Delete(ObjectKind k, GLsizei n, const GLuint* ids) {
    std::lock_guard<std::mutex> l(mu_);
    for (GLsizei i = 0; i < n; ++i) live_[k].erase(ids[i]);
}
bool ObjectRegistry::Is(ObjectKind k, GLuint id) const {
    std::lock_guard<std::mutex> l(mu_);
    auto it = live_.find(k);
    return it != live_.end() && it->second.count(id) > 0;
}
GLuint ObjectRegistry::NextId() {
    std::lock_guard<std::mutex> l(mu_);
    return next_++;
}
size_t ObjectRegistry::LiveCount(ObjectKind k) const {
    std::lock_guard<std::mutex> l(mu_);
    auto it = live_.find(k);
    return it == live_.end() ? 0 : it->second.size();
}
} // namespace tglmt
