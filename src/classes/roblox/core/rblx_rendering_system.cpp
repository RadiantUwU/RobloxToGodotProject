#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include "rblx_basic_types.hpp"
#include "rblx_rendering_system.hpp"
#include "rblx_main.hpp"

namespace godot {

CFrame from_godot_transform(Transform3D transform) {
    return CFrame(
        transform.basis.rows[0].x,
        transform.basis.rows[1].x,
        transform.basis.rows[2].x,
        transform.basis.rows[0].y,
        transform.basis.rows[1].y,
        transform.basis.rows[2].y,
        transform.basis.rows[0].z,
        transform.basis.rows[1].z,
        transform.basis.rows[2].z,
        transform.origin.x,
        transform.origin.y,
        transform.origin.z
   );
}
Transform3D to_godot_transform(CFrame cf) {
    Transform3D transform;
    transform.origin.x = cf.X ;
    transform.origin.y = cf.Y ;
    transform.origin.z = cf.Z ;
    transform.basis.rows[0].x=cf.R00;
    transform.basis.rows[0].y=cf.R01;
    transform.basis.rows[0].z=cf.R02;
    transform.basis.rows[1].x=cf.R10;
    transform.basis.rows[1].y=cf.R11;
    transform.basis.rows[1].z=cf.R12;
    transform.basis.rows[2].x=cf.R20;
    transform.basis.rows[2].y=cf.R21;
    transform.basis.rows[2].z=cf.R22;
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
CFrame face_to_cframe(NormalId face) {
    switch (face) {
    case NormalId_Top:
        return from_godot_transform(Transform3D(Basis::looking_at(Vector3( 0, 1, 0)),Vector3(0,0,0)));
    case NormalId_Bottom:
        return from_godot_transform(Transform3D(Basis::looking_at(Vector3( 0,-1, 0)),Vector3(0,0,0)));
    case NormalId_Front:
        return from_godot_transform(Transform3D(Basis::looking_at(Vector3( 0, 0,-1)),Vector3(0,0,0)));
    case NormalId_Back:
        return from_godot_transform(Transform3D(Basis::looking_at(Vector3( 0, 0, 1)),Vector3(0,0,0)));
    case NormalId_Left:
        return from_godot_transform(Transform3D(Basis::looking_at(Vector3( 1, 0, 0)),Vector3(0,0,0)));
    case NormalId_Right:
        return from_godot_transform(Transform3D(Basis::looking_at(Vector3(-1, 0, 0)),Vector3(0,0,0)));
    }
}
rblx_internal_rendering_system::RefCountedRID::~RefCountedRID() {
    if (rid_refcnted==nullptr) return;
    if (rid_refcnted->refcnt-- == 1) {
        rid_refcnted->system->delete_rid(rid_refcnted->rid);
        delete rid_refcnted;
    }
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
RenderingServer* RBXRenderObject::get_server() {
    return rblx_renderer->rendering_server;
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
    assert(!rids.has(rid));
    rids.append(rid);
}
void RBXRenderingSystem::delete_rid(RID rid) {
    assert(rids.has(rid));
    rids.erase(rid);
    rendering_server->free_rid(rid);
}
void RBXRenderingSystem::enable() {
    assert(!enabled);
    enabled = true;
    rendering_server->scenario_set_environment(scenario, environment);
    rendering_server->instance_set_visible(workspace_instance, true);
}
void RBXRenderingSystem::disable() {
    assert(enabled);
    enabled = false;
    rendering_server->scenario_set_environment(scenario,RID());
    rendering_server->instance_set_visible(workspace_instance, false);
}
void RBXRenderingSystem::set_viewport(Viewport* viewport) {
    Ref<World3D> world3d = viewport->get_world_3d();
    Ref<World2D> world2d = viewport->get_world_2d();
    canvas = world2d->get_canvas();
    scenario = world3d->get_scenario();
    if (environment != nullptr) delete_rid(environment);
    environment = rendering_server->environment_create();
    add_rid(environment);
    rendering_server->instance_set_scenario(workspace_instance, scenario);
}
void RBXRenderingSystem::render() {
    assert(enabled);
    rendering_server->force_draw();
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
void RBXMeshPart::set_mesh(rblx_internal_rendering_system::RefCountedRID rid) {
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

void RBXLight::set_brightness(float brightness) {
    get_server()->light_set_param(light_instance, RenderingServer::LIGHT_PARAM_ENERGY, brightness);
}
void RBXLight::set_color(Color3 color) {
    get_server()->light_set_color(light_instance, Color(
            color.R,
            color.G,
            color.B
        ));
}
void RBXLight::set_enable_shadows(bool enable_shadows) {
    get_server()->light_set_shadow(light_instance, enable_shadows);
}

void RBXPointLight::create_light_instance() {
    light_instance = get_server()->omni_light_create();
}
void RBXPointLight::set_range(float range) {
    get_server()->light_set_param(light_instance, RenderingServer::LIGHT_PARAM_RANGE, range);
}

void RBXSpotLight::create_light_instance() {
    light_instance = get_server()->spot_light_create();
}
void RBXSpotLight::set_face(NormalId normal) {
    face = normal;
    set_position(last_position);
}
void RBXSpotLight::set_angle(float angle) {
    get_server()->light_set_param(light_instance, RenderingServer::LIGHT_PARAM_SPOT_ANGLE, angle);
}
void RBXSpotLight::set_range(float range) {
    get_server()->light_set_param(light_instance, RenderingServer::LIGHT_PARAM_RANGE, range);
}
void RBXSpotLight::set_position(CFrame position) {
    last_position = position;
    get_server()->instance_set_transform(instance, to_godot_transform(pivot_cframe(
            last_position,
            face_to_cframe(face)
        )));
}


}; // namespace godot