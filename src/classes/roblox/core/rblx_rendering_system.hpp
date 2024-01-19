#ifndef RBLX_RENDERING_SYSTEM
#define RBLX_RENDERING_SYSTEM
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/templates/vector.hpp>

#include "rblx_basic_types.hpp"
#include "rblx_main.hpp"

namespace godot {

class RBXRenderingSystem {
    bool enabled = false;
    Vector<RID> rids;
    HashMap<RID,bool> is_supposed_to_be_visible;
protected:
    void add_rid(RID rid);
    void delete_rid(RID rid); // destroys too

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

}; // namespace godot

#endif