#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/box_mesh.hpp>

#include "rblx_basic_types.hpp"
#include "rblx_debug.hpp"
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

RBXRenderObject::RBXRenderObject(RBXRenderObject& o) {
    RBLX_PRINT_VERBOSE("Copying render object");
    this->rblx_renderer = o.rblx_renderer;
    this->instance = o.instance;
    o.rblx_renderer = nullptr; // make the other object useless.
}
RBXRenderObject::~RBXRenderObject() {
    if (rblx_renderer != nullptr && instance.is_valid()) {
        RBLX_PRINT_VERBOSE("Deallocating render object!");
        rblx_renderer->delete_rid(instance);
    }
}
RenderingServer* RBXRenderObject::get_server() {
    assert(rblx_renderer != nullptr);
    return rblx_renderer->rendering_server;
}
void RBXRenderObject::set_position(CFrame position) {
    assert(rblx_renderer != nullptr);
    rblx_renderer->rendering_server->instance_set_transform(instance,to_godot_transform(position));
}
void RBXRenderObject::set_visible(bool visible) {
    assert(rblx_renderer != nullptr);
    rblx_renderer->rendering_server->instance_set_visible(instance, visible);
}

RBXRenderingSystem::RBXRenderingSystem() {
    rendering_server = RenderingServer::get_singleton();
    workspace_instance = rendering_server->instance_create();
    load_meshes();
    load_materials();
}
RBXRenderingSystem::~RBXRenderingSystem() {
    for (RID& rid : rids) {
        rendering_server->free_rid(rid);
    }
    rendering_server->free_rid(workspace_instance);
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
    RBLX_PRINT_VERBOSE("Enabled rblx rendering system!");
}
void RBXRenderingSystem::disable() {
    assert(enabled);
    enabled = false;
    rendering_server->scenario_set_environment(scenario,RID());
    rendering_server->instance_set_visible(workspace_instance, false);
    RBLX_PRINT_VERBOSE("Disabled rblx rendering system!");
}
void RBXRenderingSystem::set_viewport(Viewport* viewport) {
    Ref<World3D> world3d = viewport->find_world_3d();
    Ref<World2D> world2d = viewport->find_world_2d();
    canvas = world2d->get_canvas();
    scenario = world3d->get_scenario();
    if (environment.is_valid()) delete_rid(environment);
    environment = rendering_server->environment_create();
    add_rid(environment);
    rendering_server->instance_set_scenario(workspace_instance, scenario);
    /*{ //so mesh is valid, scenario is also valid, is the workspace instance valid?
        RID cube_instance = rendering_server->instance_create();
        rendering_server->instance_set_scenario(cube_instance, scenario);
        rendering_server->instance_set_visibility_parent(cube_instance, workspace_instance);
        rendering_server->instance_geometry_set_material_override(cube_instance, smooth_plastic);
        rendering_server->instance_set_base(cube_instance, cube);
        rendering_server->instance_geometry_set_shader_parameter(cube_instance, "color", Color(1,0,1));
        rendering_server->instance_set_transform(cube_instance, to_godot_transform(RBXVector3::new_(0,0,0)));
        rendering_server->instance_set_visible(cube_instance, true);
        Color color = rendering_server->instance_geometry_get_shader_parameter(cube_instance, "color");
        RBLX_PRINT_VERBOSE(color.r, ' ', color.g, ' ', color.b, ' ', color.a);
    }*/
    {
        RBXPartRender* part = new (memalloc(sizeof(RBXPartRender))) RBXPartRender(this->create<RBXPartRender>());
        part->set_part_type(RBXPartRender::TYPE_BLOCK);
        part->resize(RBXVector3::new_(1,1,1));
        part->set_color(Color3(1,0,1));
        part->set_position(CFrame(RBXVector3::new_(0,0,0)));
        part->set_visible(true);
    }
}
void RBXRenderingSystem::render() {
    if (!enabled) return;
    rendering_server->force_draw();
}
void RBXRenderingSystem::load_materials() {
    RID shader;
    shader = rendering_server->shader_create();
    add_rid(shader);
    rendering_server->shader_set_code(shader,
    "shader_type spatial;"
    "render_mode diffuse_toon, specular_toon, cull_disabled;"
    "instance uniform vec3 color : source_color;"
    "instance uniform float reflectance : hint_range(0,1);"
    "void fragment() {"
    "   ALBEDO=color;"
    "   ROUGHNESS=0.1;"
    "   METALLIC=reflectance;"
    "   SPECULAR=.5+.5*reflectance;"
    "}"
    );
    smooth_plastic = rendering_server->material_create();
    rendering_server->material_set_shader(smooth_plastic, shader);
    add_rid(smooth_plastic);
}
void RBXRenderingSystem::load_meshes() {
    cube = rendering_server->mesh_create();
    add_rid(cube);
    Array cube_array;
    Ref<BoxMesh> boxmesh;
    boxmesh.instantiate();
    boxmesh->set_size(Vector3(1,1,1));
    cube_array = boxmesh->surface_get_arrays(0);
    rendering_server->mesh_add_surface_from_arrays(
            cube,
            RenderingServer::PRIMITIVE_TRIANGLES,
            cube_array
        );
}
template<>
RBXMeshPart RBXRenderingSystem::create<RBXMeshPart>() {
    RBXMeshPart meshpart;
    meshpart.rblx_renderer = this;
    meshpart.instance = rendering_server->instance_create();
    rendering_server->instance_set_scenario(meshpart.instance,scenario);
    rendering_server->instance_set_visibility_parent(meshpart.instance,workspace_instance);
    return meshpart;
}
template<>
RBXPartRender RBXRenderingSystem::create<RBXPartRender>() {
    RBXPartRender part;
    part.rblx_renderer = this;
    part.instance = rendering_server->instance_create();
    rendering_server->instance_set_scenario(part.instance,scenario);
    rendering_server->instance_set_visibility_parent(part.instance,workspace_instance);
    rendering_server->instance_geometry_set_material_override(part.instance, smooth_plastic);
    return part;
}

RBXRenderBasePart::RBXRenderBasePart(RBXRenderBasePart& o) : RBXRenderObject((RBXRenderObject&)o) {
    this->size = o.size;
}

RBXMeshPart::RBXMeshPart(RBXMeshPart& o) : RBXRenderBasePart((RBXRenderBasePart&)o) {
    this->mesh = o.mesh;
}
void RBXMeshPart::set_mesh(rblx_internal_rendering_system::RefCountedRID rid) {
    rblx_renderer->rendering_server->instance_set_base(instance,rid);
}
void RBXMeshPart::resize(RBXVector3 new_size) {
    size = new_size;
}
void RBXMeshPart::set_reflectance(float refl) {
    get_server()->instance_geometry_set_shader_parameter(instance, "reflectance", refl);
}
void RBXMeshPart::set_transparency(float transparency) {
    get_server()->instance_geometry_set_transparency(instance, transparency);
}
void RBXMeshPart::set_color(Color3 color) {
    Vector3 v3 = Vector3(color.R,color.G,color.B);
    get_server()->instance_geometry_set_shader_parameter(instance, "color", v3);
}

RBXPartRender::RBXPartRender(RBXPartRender& o) : RBXRenderBasePart((RBXRenderBasePart&)o) {}
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
void RBXPartRender::resize(RBXVector3 new_size) {
    size = new_size;
}
void RBXPartRender::set_reflectance(float refl) {
    get_server()->instance_geometry_set_shader_parameter(instance, "reflectance", refl);
}
void RBXPartRender::set_transparency(float transparency) {
    get_server()->instance_geometry_set_transparency(instance, transparency);
}
void RBXPartRender::set_color(Color3 color) {
    Vector3 v3 = Vector3(color.R,color.G,color.B);
    get_server()->instance_geometry_set_shader_parameter(instance, "color", v3);
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