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
public:
    RBXRenderingSystem();
    ~RBXRenderingSystem();
    void enable();
    void disable();
    void render();
    void set_viewport(Viewport* new_viewport);
};

class RBXBasePartRender {
    virtual void resize(RBXVector3 new_size);
    virtual void set_reflectance(float refl);
};

}; // namespace godot

#endif