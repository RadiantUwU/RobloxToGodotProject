#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "rblx_basic_types.hpp"
#include "rblx_rendering_system.hpp"
#include "rblx_main.hpp"

namespace godot {

CFrame from_godot_transform(Transform3D transform) {
   return {
        .X=transform.origin.x,
        .Y=transform.origin.y,
        .Z=transform.origin.z,
        .R00=transform.basis.x.x,
        .R01=transform.basis.x.y,
        .R02=transform.basis.x.z,
        .R10=transform.basis.y.x,
        .R11=transform.basis.y.y,
        .R12=transform.basis.y.z,
        .R20=transform.basis.z.x,
        .R21=transform.basis.z.y,
        .R22=transform.basis.z.z,
    };
}
Transform3D to_godot_transform(CFrame cf) {
    Transform3D transform;
    transform.origin.x = cf.X ;
    transform.origin.y = cf.Y ;
    transform.origin.z = cf.Z ;
    transform.basis.x.x=cf.R00;
    transform.basis.x.y=cf.R01;
    transform.basis.x.z=cf.R02;
    transform.basis.y.x=cf.R10;
    transform.basis.y.y=cf.R11;
    transform.basis.y.z=cf.R12;
    transform.basis.z.x=cf.R20;
    transform.basis.z.y=cf.R21;
    transform.basis.z.z=cf.R22;
    return transform;
}
template <typename... T>
Transform3D _pivot_transform(CFrame lowest, T... others) {
    return to_godot_transform(lowest)*_pivot_transform(others...);
}
Transform3D _pivot_transform(CFrame lowest) {
    return to_godot_transform(lowest);
}
template <typename... T>
CFrame pivot_cframe(CFrame lowest, T... others) {
    return from_godot_transform(to_godot_transform(lowest)*_pivot_transform(others...));
}
CFrame pivot_cframe(CFrame lowest) {
    return lowest;
}

RBXRenderObject::RBXRenderObject(RBXRenderObject&& o) {
    this->rblx_renderer = o.rblx_renderer;
    this->instance = o.instance;
    o.rblx_renderer = nullptr; // make the other object useless.
}
RBXRenderObject& RBXRenderObject::operator=(RBXRenderObject&& o) {
    this->rblx_renderer = o.rblx_renderer;
    this->instance = o.instance;
    o.rblx_renderer = nullptr; // make the other object useless.
    return *this;
}
RBXRenderObject::~RBXRenderObject() {
    if (rblx_renderer != nullptr) rblx_renderer->delete_rid(instance);
}
void RBXRenderObject::set_position(CFrame position) {
    rblx_renderer->rendering_server->instance_set_transform(instance,to_godot_transform(position));
}
void RBXRenderObject::set_visible(bool visible) {
    rblx_renderer->rendering_server->instance_set_visible(instance, visible);
}

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
}
void RBXRenderingSystem::disable() {
#ifndef NDEBUG
    assert(enabled);
#endif
    enabled = false;
    rendering_server->scenario_set_environment(scenario,RID());
}
void RBXRenderingSystem::set_viewport(Viewport* viewport) {
    Ref<World3D> world3d = viewport->get_world_3d();
    Ref<World2D> world2d = viewport->get_world_2d();
    canvas = world2d->get_canvas();
    scenario = world3d->get_scenario();
    environment = rendering_server->environment_create();
    rendering_server->instance_set_scenario(workspace_instance);
}

RBXRenderBasePart::RBXRenderBasePart(RBXRenderBasePart&& o) : RBXRenderObject((RBXRenderObject&&)o) {
    this->size = o.size;
}
RBXRenderBasePart& RBXRenderBasePart::operator=(RBXRenderBasePart&& o) {
    ((RBXRenderObject&)*this) = (RBXRenderObject&&)o; // call super method
    this->size = o.size;
    return *this;
}

RBXMeshPart::RBXMeshPart(RBXMeshPart&& o) : RBXRenderBasePart((RBXRenderBasePart&&)o) {
    this->mesh = o.mesh;
}
RBXMeshPart& RBXMeshPart::operator=(RBXMeshPart&& o) {
    ((RBXRenderBasePart&)*this) = (RBXRenderBasePart&&)o; // call super method
    this->mesh = o.mesh;
    return *this;
}
void RBXMeshPart::set_mesh(RBXRenderingSystem::RefCountedRID rid) {
    rblx_renderer->rendering_server->instance_set_base(instance,rid);
}

RBXPartRender::RBXPartRender(RBXPartRender&& o) : RBXRenderBasePart((RBXRenderBasePart&&)o) {}
RBXPartRender& RBXPartRender::operator=(RBXPartRender&& o) {
    ((RBXRenderBasePart&)*this) = (RBXRenderBasePart&&)o; // call super method
    return *this;
}
void RBXPartRender::set_part_type(RBXPartRender::PartType part_type) {
    switch (part_type) {
    case RBXPartRender::TYPE_BLOCK:
        this->rblx_renderer->rendering_server->instance_set_base(instance,this->rblx_renderer->cube);
        break;
    case RBXPartRender::TYPE_CYLINDER:
        this->rblx_renderer->rendering_server->instance_set_base(instance,this->rblx_renderer->cylinder);
        break;
    case RBXPartRender::TYPE_SPHERE:
        this->rblx_renderer->rendering_server->instance_set_base(instance,this->rblx_renderer->sphere);
        break;
    case RBXPartRender::TYPE_WEDGE:
        this->rblx_renderer->rendering_server->instance_set_base(instance,this->rblx_renderer->wedge);
        break;
    case RBXPartRender::TYPE_CORNERWEDGE:
        this->rblx_renderer->rendering_server->instance_set_base(instance,this->rblx_renderer->corner_wedge);
        break;
    }
}


}; // namespace godot