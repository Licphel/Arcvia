#pragma once
#include "gfx/buffer.h"
#include "gfx/state.h"

namespace arc {

struct brush;

struct mesh {
    graph_state state;
    std::shared_ptr<complex_buffer> buffer;
    std::unique_ptr<brush> brush_;

    /* unstable */ unsigned int vao_, vbo_, ebo_;
    /* unstable */ bool is_direct_;

    mesh();
    ~mesh();
    // clear the mesh and attempt to redraw it.
    brush* retry();
    // record the mesh state and buffer content, and end the drawing.
    void record();
    // draw the mesh with the brush. the brush should be direct-to-screen.
    void draw(brush* gbrush);

    static std::shared_ptr<mesh> make();
};

}  // namespace arc