#ifndef RBLX_RENDERING_SYSTEM
#define RBLX_RENDERING_SYSTEM
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

class RBXRenderObject {
protected:
    RID instance;
    RBXRenderingSystem* rblx_renderer;
    RBXRenderObject()=default;
public:
    RBXRenderObject(RBXRenderObject&&);
    RBXRenderObject& operator=(RBXRenderObject&&);
    ~RBXRenderObject();
    virtual void set_position(CFrame position);
    void set_visible(bool visible);
};
class RBXRenderBasePart : public RBXRenderObject {
protected:
    RBXVector3 size = RBXVector3::ONE;
public:
    RBXRenderBasePart(RBXRenderBasePart&&);
    RBXRenderBasePart& operator=(RBXRenderBasePart&&);
    virtual void resize(RBXVector3 new_size) {
        size = new_size;
    }
    virtual void set_reflectance(float refl) {
        rblx_renderer->rendering_server->instance_geometry_set_shader_parameter(instance, "reflectance", refl);
    };
    virtual void set_transparency(float transparency) {
        rblx_renderer->rendering_server->instance_geometry_set_transparency(instance, transparency);
    };
    virtual void set_color(Color3 color) {
        Vector3 v3 = Vector3(color.R,color.G,color.B);
        rblx_renderer->rendering_server->instance_geometry_set_shader_parameter(instance, "color", color);
    }
    //virtual void set_material(RBXMaterial material) = 0;
};
class RBXMeshPart : public RBXRenderBasePart {
    RefCountedRID mesh;
public:
    RBXMeshPart(RBXMeshPart&&);
    RBXMeshPart& operator=(RBXMeshPart&&);
    void set_mesh(RBXRenderingSystem::RefCountedRID rid);
};
class RBXPartRender : public RBXRenderBasePart {
public:
    RBXPartRender(RBXPartRender&&);
    RBXPartRender& operator=(RBXPartRender&&);
    enum PartType {
        TYPE_BLOCK,
        TYPE_WEDGE,
        TYPE_CYLINDER,
        TYPE_SPHERE,
        TYPE_CORNERWEDGE
    };
    void set_part_type(PartType type);
};
class RBXLight : public RBXRenderObject {
protected:
    RID light_instance;
public:
    RBXPartRender(RBXPartRender&&);
    RBXPartRender& operator=(RBXPartRender&&);
    virtual void create_light_instance() = 0;
    
    void set_brightness(float brightness);
    void set_color(float brightness);
    void set_enabled(bool v);
    void set_enable_shadows(bool enable_shadows);
};
class RBXPointLight : public RBXLight {
public:
    virtual void create_light_instance() override;
    void set_range(float range);
};
class RBXSpotLight : public RBXLight {
public:
    virtual void create_light_instance() override;
    void set_range(float range);
    //void set_face(NormalId normal);
    void set_angle(float angle);
};
class RBXSurfaceLight : public RBXLight {
public:
    virtual void create_light_instance() override;
    void set_range(float range);
    //void set_face(NormalId normal);
    void set_angle(float angle);
};
class RBXLowLevelLighting {
protected:
    RBXRenderingSystem* rblx_renderer;
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
    bool enabled = false;
    Vector<RID> rids;
    Vector<RID> materials;

    class _RefCountedRID {
        mutable size_t refcnt = 1;
    public:
        RBXRenderingSystem* const system;
        _RefCountedRID(const RID rid, RBXRenderingSystem* system) : rid(rid) {}
        const RID rid;
    };

    void load_materials();
protected:
    void add_rid(RID rid);
    void delete_rid(RID rid); // destroys too

    class RefCountedRID {
        _RefCountedRID* const rid_refcnted;
    public:
        RefCountedRID() : rid_refcnted(nullptr) {}
        RefCountedRID(RID rid, RBXRenderingSystem* system)
            : rid_refcnted(new _RefCountedRID(rid, system))
        {}
        RefCountedRID(const RefCountedRID o)
            : rid_refcnted(o->ref_refcnted) 
        {
            rid_refcnted->ref_cnt++;
        }
        RefCountedRID(const RefCountedRID& o)
            : rid_refcnted(o->ref_refcnted) 
        {
            rid_refcnted->ref_cnt++;
        }
        ~RefCountedRID() {
            if (ref_refcnted==nullptr) return;
            if (ref_refcnted->ref_cnt-- == 1) {
                ref_refcnted->system->delete_rid(ref_refcnted->rid);
                delete ref_refcnted;
            }
        }
        operator RID() const {
            return rid_refcnted->rid;
        }
        RID asRID() const {
            return rid_refcnted->rid;
        }
    };

    RenderingServer* rendering_server;
    RID scenario;//3D
    RID canvas;//2D
    RID environment;
    RID workspace_instance;
protected:
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
    void create() {}

    template <>
    RBXMeshPart&& create<RBXMeshPart>();
    template <>
    RBXPartRender&& create<RBXPartRender>();
};


}; // namespace godot

#endif