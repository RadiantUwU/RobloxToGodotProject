#ifndef RBLX_RENDERING_SYSTEM
#define RBLX_RENDERING_SYSTEM
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/templates/vector.hpp>

#include "rblx_main.hpp"

namespace godot {

class RBXRenderingSystem {
    bool enabled = false;
    Vector<RID> rids;
protected:
    RenderingServer* rendering_server;
    void add_rid(RID rid);
    void delete_rid(RID rid);
    Viewport* viewport;
    RID old_space;
    RID old_canvas;
    RID new_space;
    RID new_canvas;
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