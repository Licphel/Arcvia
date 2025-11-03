#include "gfx/mesh.h"

#include "gfx/brush.h"
#include "gfx/buffer.h"

// clang-format off
#include <gl/glew.h>
#include <gl/gl.h>
// clang-format on

namespace arc {

mesh::mesh() {
    buffer = complex_buffer::make();
    brush_ = buffer->derive_brush();
    brush_->mesh_root_ = this;
}

mesh::~mesh() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);
}

brush* mesh::retry() {
    buffer->clear();
    if (!is_direct_) brush_->is_in_mesh_ = true;
    return brush_.get();
}

void mesh::record() {
    state = brush_->state_;
    if (!is_direct_) brush_->is_in_mesh_ = false;
}

void mesh::draw(brush* gbrush) {
    auto old_state = gbrush->state_;
    auto old_buf = gbrush->wbuf;
    auto old_msh = gbrush->mesh_root_;

    gbrush->use_state(state);
    gbrush->wbuf = buffer.get();
    gbrush->mesh_root_ = this;
    gbrush->clear_when_flush_ = false;

    gbrush->flush();

    gbrush->clear_when_flush_ = true;
    gbrush->mesh_root_ = old_msh;
    gbrush->wbuf = old_buf;
    gbrush->use_state(old_state);
}

std::shared_ptr<mesh> mesh::make() {
    std::shared_ptr<mesh> msh = std::make_shared<mesh>();
    std::shared_ptr<complex_buffer> buf = msh->buffer;

    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    msh->vao_ = vao;
    msh->vbo_ = vbo;
    msh->ebo_ = ebo;

    return msh;
}

}  // namespace arc
