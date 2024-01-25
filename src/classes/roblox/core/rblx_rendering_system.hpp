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

CFrame from_godot_transform(Transform3D transform);
Transform3D to_godot_transform(CFrame cframe);
template <typename... T>
CFrame pivot_cframe(CFrame lowest, CFrame... others);

class RBXRenderObject {
protected:
    RID instance;
    RBXRenderingSystem* rblx_renderer;
    RBXRenderObject();
public:
    RBXRenderObject(RBXRenderObject&&);
    /*RBXRenderObject(RBXRenderObject&) = delete;
    RBXRenderObject& operator=(RBXRenderObject) = delete;
    RBXRenderObject& operator=(RBXRenderObject&&);*/
    ~RBXRenderObject();
    void set_position(CFrame position);
    void set_visible(bool visible);
};

class RBXRenderingSystem {
    friend class RBXRenderObject;
    friend class RBXMeshPart;
    bool enabled = false;
    Vector<RID> rids;
    HashMap<RID,bool> is_supposed_to_be_visible;
protected:
    void add_rid(RID rid);
    void delete_rid(RID rid); // destroys too

    class _RefCountedRID {
        mutable size_t refcnt = 1;
    public:
        RBXRenderingSystem* const system;
        _RefCountedRID(const RID rid, RBXRenderingSystem* system) : rid(rid) {}
        const RID rid;
    };
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
public:
    RBXRenderingSystem();
    ~RBXRenderingSystem();
    void enable();
    void disable();
    void render();
    void set_viewport(Viewport* new_viewport);
};

class RBXRenderBasePart : public RBXRenderObject {
protected:
    RBXVector3 size = RBXVector3::ONE;
public:
    virtual void resize(RBXVector3 new_size) = 0;
    virtual void set_reflectance(float refl) = 0;
    virtual void set_transparency(float transparency) = 0;
    virtual void set_local_transparency(float local_transparency) = 0;
    virtual void set_color(Color3 color) = 0;
    //virtual void set_material(RBXMaterial material) = 0;
};
class RBXMeshPart : public RBXRenderBasePart {
    RefCountedRID mesh;
public:
    virtual void resize(RBXVector3 new_size) override;
    virtual void set_reflectance(float refl) override;
    virtual void set_transparency(float transparency) override;
    virtual void set_local_transparency(float local_transparency) override;
    virtual void set_color(Color3 color) override;
    //virtual void set_material(RBXMaterial material) override;
    void set_mesh(RefCountedRID rid);
}
class RBXPartRender : public RBXRenderBasePart {
public:
    enum PartType {
        TYPE_BLOCK,
        TYPE_WEDGE,
        TYPE_CYLINDER,
        TYPE_SPHERE,
        TYPE_CORNERWEDGE
    };
    virtual void resize(RBXVector3 new_size) override;
    virtual void set_reflectance(float refl) override;
    virtual void set_transparency(float transparency) override;
    virtual void set_local_transparency(float local_transparency) override;
    virtual void set_color(Color3 color) override;
    //virtual void set_material(RBXMaterial material) override;
    void set_part_type(PartType type);
};

}; // namespace godot

#endif