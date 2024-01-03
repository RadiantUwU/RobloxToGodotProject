#include "rblx_rendering_system.hpp"
#include "rblx_main.hpp"

namespace godot {

RBXRenderingSystem::RBXRenderingSystem() {
    rendering_server = RenderingServer::get_singleton();
}
RBXRenderingSystem::~RBXRenderingSystem() {
    for (RID& rid : rids) {
        rendering_server->free_rid(rid);
    }
}
void RBXRenderingSystem::add_rid(RID rid) {
#ifndef NDEBUG
    assert(!rids.has(rid));
#endif
    rids.append(rid);
}
void RBXRenderingSystem::delete_rid(RID rid) {
#ifndef NDEBUG
    assert(rids.has(rid));
#endif
    rids.remove(rid);
    rendering_server->free_rid(rid);
}
void RBXRenderingSystem::enable() {
#ifndef NDEBUG
    assert(!enabled);
#endif
    enabled = true;
}
void RBXRenderingSystem::disable() {
#ifndef NDEBUG
    assert(enabled);
#endif
    enabled = false;
}

}; // namespace godot