#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "rblx_rendering_system.hpp"
#include "rblx_main.hpp"

namespace godot {

RBXRenderingSystem::RBXRenderingSystem() {
    rendering_server = RenderingServer::get_singleton();
    workspace_instance = rendering_server->instance_create();
}
RBXRenderingSystem::~RBXRenderingSystem() {
    for (RID& rid : rids) {
        rendering_server->free_rid(rid);
    }
    rendering_server->free_rid(workspace_instance);
    rendering_server->free_rid(environment);
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
    rendering_server->scenario_set_environment(scenario,environment);
    for (auto& key_value : is_supposed_to_be_visible)
        rendering_server->instance_set_visible(key_value.key,key_value.value);
}
void RBXRenderingSystem::disable() {
#ifndef NDEBUG
    assert(enabled);
#endif
    enabled = false;
    rendering_server->scenario_set_environment(scenario,RID());
    for (auto& key_value : is_supposed_to_be_visible)
        rendering_server->instance_set_visible(key_value.key,false);
}
void RBXRenderingSystem::set_viewport(Viewport* viewport) {
    Ref<World3D> world3d = viewport->get_world_3d();
    Ref<World2D> world2d = viewport->get_world_2d();
    canvas = world2d->get_canvas();
    scenario = world3d->get_scenario();
    environment = rendering_server->environment_create();
    rendering_server->instance_set_scenario(workspace_instance);
}


}; // namespace godot