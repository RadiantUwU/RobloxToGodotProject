#ifndef RBLX_RENDERING_SYSTEM
#define RBLX_RENDERING_SYSTEM

#include <type_traits>

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/templates/vector.hpp>

#include "rblx_basic_types.hpp"
#include "rblx_main.hpp"

namespace godot {

class RBXRenderingSystem;

CFrame from_godot_transform(Transform3D transform);
Transform3D to_godot_transform(CFrame cframe);
template <typename... T>
CFrame pivot_cframe(CFrame lowest, T... others);
CFrame pivot_cframe(CFrame lowest);
/*
template <typename... T>
Transform3D _pivot_transform(CFrame lowest, T... others);
Transform3D _pivot_transform(CFrame lowest);
*/// Hide the functions from the header, essentially making them private.
namespace rblx_internal_rendering_system {
class RefCountedRID {
    class _RefCountedRID {
    public:
        mutable size_t refcnt = 1;
        RBXRenderingSystem* const system;
        _RefCountedRID(const RID rid, RBXRenderingSystem* system) : rid(rid), system(system) {}
        const RID rid;
    };
    _RefCountedRID* rid_refcnted;
public:
    RefCountedRID() : rid_refcnted(nullptr) {}
    RefCountedRID(RID rid, RBXRenderingSystem* system)
        : rid_refcnted(new _RefCountedRID(rid, system))
    {}
    RefCountedRID(const RefCountedRID& o)
        : rid_refcnted(o.rid_refcnted) 
    {
        rid_refcnted->refcnt++;
    }
    ~RefCountedRID();
    operator RID() const {
        return rid_refcnted->rid;
    }
    RID asRID() const {
        return rid_refcnted->rid;
    }
};
};//rblx_internal_rendering_system

class RBXRenderObject {
protected:
    RID instance;
    char* refcount = nullptr;
    RBXRenderingSystem* rblx_renderer;
    RBXRenderObject()=default;
    RenderingServer* get_server();
public:
    RBXRenderObject(RBXRenderObject& obj);
    virtual ~RBXRenderObject();
    virtual void set_position(CFrame position);
    void set_visible(bool visible);
};
//TODO: proper deallocation for all these subclasses
class RBXRenderBasePart : public RBXRenderObject {
protected:
    RBXVector3 size = {1,1,1};
    RBXRenderBasePart() : RBXRenderObject() {}
public:
    RBXRenderBasePart(RBXRenderBasePart&);
    virtual void resize(RBXVector3 new_size) = 0;
    virtual void set_reflectance(float refl) = 0;
    virtual void set_transparency(float transparency) = 0;
    virtual void set_color(Color3 color) = 0;
    void set_position(CFrame position) override;
    //virtual void set_material(RBXMaterial material) = 0;
};
class RBXMeshPart : public RBXRenderBasePart {
    rblx_internal_rendering_system::RefCountedRID mesh;
    friend class RBXRenderingSystem;
protected:
    RBXMeshPart() : RBXRenderBasePart() {}
public:
    RBXMeshPart(RBXMeshPart&);
    void set_mesh(rblx_internal_rendering_system::RefCountedRID rid);
    void resize(RBXVector3 new_size) override;
    void set_reflectance(float refl) override;
    void set_transparency(float transparency) override;
    void set_color(Color3 color) override;
};
class RBXPartRender : public RBXRenderBasePart {
protected:
    friend class RBXRenderingSystem;
    RBXPartRender() : RBXRenderBasePart() {}
public:
    RBXPartRender(RBXPartRender&);
    enum PartType {
        TYPE_BLOCK,
        TYPE_WEDGE,
        TYPE_CYLINDER,
        TYPE_SPHERE,
        TYPE_CORNERWEDGE
    };
    void set_part_type(PartType type);
    void resize(RBXVector3 new_size) override;
    void set_reflectance(float refl) override;
    void set_transparency(float transparency) override;
    void set_color(Color3 color) override;
};
class RBXLight : public RBXRenderObject {
protected:
    RID light_instance;
public:
    RBXLight(RBXLight&);
    ~RBXLight();
    virtual void create_light_instance() = 0;
    
    void set_brightness(float brightness);
    void set_color(Color3 color);
    //void set_enabled(bool v); // use set_visible
    void set_enable_shadows(bool enable_shadows);
};
class RBXPointLight : public RBXLight {
public:
    virtual void create_light_instance() override;
    void set_range(float range);
};
class RBXSpotLight : public RBXLight {
    CFrame last_position;
    NormalId face;
public:
    virtual void create_light_instance() override;
    void set_range(float range);
    void set_face(NormalId normal);
    void set_position(CFrame cframe) override;
    void set_angle(float angle);
};
class RBXSurfaceLight : public RBXLight {
public:
    void create_light_instance() override;
    void set_range(float range);
    void set_face(NormalId normal);
    void set_angle(float angle);
};
class RBXLowLevelLighting {
protected:
    RBXRenderingSystem* rblx_renderer;
    friend class RBXRenderingSystem;
    RBXLowLevelLighting()=default;
    RBXLowLevelLighting(RBXLowLevelLighting&)=delete;
    RBXLowLevelLighting& operator=(const RBXLowLevelLighting&)=delete;

    Color3 ambient_light;
    float brightness = 1;
public:
    void set_ambient_light(Color3 color);
    void set_brightness(float brightness);
    void set_clock_time(float time);
    void set_colorshift_bottom(Color3 color);
    void set_colorshift_top(Color3 color);
    void set_environment_diffuse_scale(float scale);
    void set_environment_specular_scale(float scale);
    void set_exposure(float exp);
    void set_fog_color(Color3 color);
    void set_fog_end(float depth);
    void set_fog_start(float depth);
    void set_global_shadows(bool shadows);
    void set_outdoor_ambient(Color3 ambient);
    void set_shadow_softness(float soft);
    void set_technology(Technology lighting_system);
};
class RBXRenderingSystem {
    friend class RBXRenderObject;
    friend class RBXRenderBasePart;
    friend class RBXMeshPart;
    friend class RBXPartRender;
    friend class RBXLowLevelLighting;
    friend class rblx_internal_rendering_system::RefCountedRID;
    bool enabled = false;
    Vector<RID> rids;
    Vector<RID> materials;

    void load_materials();
    void load_meshes();
protected:
    void add_rid(RID rid);
    void delete_rid(RID rid); // destroys too

    RBXLowLevelLighting* lighting;

    RenderingServer* rendering_server;
    RID scenario;//3D
    RID canvas;//2D
    RID environment;
    RID workspace_instance;

protected:
    RID smooth_plastic;

    RID cube;
    RID cylinder;
    RID sphere;
    RID wedge;
    RID corner_wedge;
public:
    RBXRenderingSystem();
    ~RBXRenderingSystem();
    void enable();
    void disable();
    void render();
    void set_viewport(Viewport* new_viewport);

    template<class T>
    T create() {
        static_assert(::std::is_base_of_v<RBXRenderObject,T>, "The class is not creatable by the rendering system.");
        return T();
    }

    template <>
    RBXMeshPart create<RBXMeshPart>();
    template <>
    RBXPartRender create<RBXPartRender>();
    template <>
    RBXPointLight create<RBXPointLight>();
    template <>
    RBXSpotLight create<RBXSpotLight>();
    template <>
    RBXSurfaceLight create<RBXSurfaceLight>();

    RBXLowLevelLighting& get_lighting();
};


}; // namespace godot

#endif