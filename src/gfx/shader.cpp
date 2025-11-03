#include "gfx/shader.h"

#include "core/log.h"
#include "core/math.h"
#include "gfx/color.h"

// clang-format off
#include <gl/glew.h>
#include <gl/gl.h>
// clang-format on

namespace arc {

shader_attrib::shader_attrib(unsigned int id) : attrib_id_(id) {}

void shader_attrib::layout(shader_vertex_data_type size, int components, int stride, int offset, bool normalize) {
    glEnableVertexAttribArray(attrib_id_);
    GLenum type;
    switch (size) {
        case shader_vertex_data_type::u8:
            type = GL_UNSIGNED_BYTE;
            break;
        case shader_vertex_data_type::i32:
            type = GL_UNSIGNED_SHORT;
            break;
        case shader_vertex_data_type::f16:
            type = GL_HALF_FLOAT;
            break;
        case shader_vertex_data_type::f32:
            type = GL_FLOAT;
            break;
        default:
            type = GL_UNSIGNED_BYTE;
            break;
    }
    glVertexAttribPointer(attrib_id_, components, type, normalize, stride, reinterpret_cast<void*>(offset));
}

shader_uniform::shader_uniform(unsigned int id) : uniform_id_(id) {}

void shader_uniform::set_texture_unit(int unit) { glUniform1i(uniform_id_, unit); }
void shader_uniform::seti(int v) { glUniform1i(uniform_id_, v); }
void shader_uniform::set(double v) { glUniform1f(uniform_id_, v); }
void shader_uniform::set(const vec2& v) { glUniform2f(uniform_id_, v.x, v.y); }
void shader_uniform::set(const vec3& v) { glUniform3f(uniform_id_, v.x, v.y, v.z); }
void shader_uniform::set(const color& v) { glUniform4f(uniform_id_, v.r, v.g, v.b, v.a); }
void shader_uniform::set(const transform& v) {
    float m[16] = {v.m00, v.m10, 0.0f, 0.0f, v.m01, v.m11, 0.0f, 0.0f,
                   0.0f,  0.0f,  1.0f, 0.0f, v.m02, v.m12, 0.0f, 1.0f};
    glUniformMatrix4fv(uniform_id_, 1, GL_FALSE, m);
}

program::~program() { glDeleteProgram(program_id_); }

shader_attrib program::get_attrib(const std::string& name) {
    return shader_attrib((unsigned int)glGetAttribLocation(program_id_, name.c_str()));
}
shader_attrib program::get_attrib(int index) { return shader_attrib((unsigned int)index); }
shader_uniform program::get_uniform(const std::string& name) {
    return shader_uniform((unsigned int)glGetUniformLocation(program_id_, name.c_str()));
}

shader_uniform program::cache_uniform(const std::string& name) {
    auto uni = get_uniform(name);
    cached_uniforms.push_back(uni);
    return uni;
}

static int build_shader_part_(std::string source, GLenum type) {
    int id = glCreateShader(type);

    if (id == 0) print_throw(log_level::fatal, "Fail to create shader.");

    const char* src = source.c_str();
    GLint len = static_cast<GLint>(source.size());

    glShaderSource(id, 1, &src, &len);
    glCompileShader(id);

    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        glDeleteShader(id);
        print_throw(log_level::fatal, "glsl compile error: {}", std::string(log));
    }

    return id;
}

std::shared_ptr<program> program::make(const std::string& vert, const std::string& frag,
                                       std::function<void(program* program)> callback_setup) {
    std::shared_ptr<program> prog = std::make_shared<program>();
    int id = prog->program_id_ = glCreateProgram();
    int vert_id = build_shader_part_(vert, GL_VERTEX_SHADER);
    int frag_id = build_shader_part_(frag, GL_FRAGMENT_SHADER);

    glAttachShader(id, vert_id);
    glAttachShader(id, frag_id);
    glBindFragDataLocation(id, 0, "fragColor");
    glLinkProgram(id);
    glDetachShader(id, vert_id);
    glDetachShader(id, frag_id);
    glValidateProgram(id);
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);

    prog->callback_setup = callback_setup;

    return prog;
}

static const std::string dvert_textured_ =
    "#version 330 core\n"
    "layout(location = 0) in vec2 i_position;\n"
    "layout(location = 1) in vec4 i_color;\n"
    "layout(location = 2) in vec2 i_texCoord;\n"
    "out vec4 o_color;\n"
    "out vec2 o_texCoord;\n"
    "uniform mat4 u_proj;\n"
    "void main() {\n"
    "    o_color = i_color;\n"
    "    o_texCoord = i_texCoord;\n"
    "    gl_Position = u_proj * vec4(i_position.x, i_position.y, 0.0, 1.0);\n"
    "}";

static const std::string dfrag_textured_ =
    "#version 330 core\n"
    "in vec4 o_color;\n"
    "in vec2 o_texCoord;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D u_tex;\n"
    "void main() {\n"
    "    fragColor = o_color * texture(u_tex, o_texCoord);\n"
    "}";

static const std::string dvert_colored_ =
    "#version 330 core\n"
    "layout(location = 0) in vec2 i_position;\n"
    "layout(location = 1) in vec4 i_color;\n"
    "out vec4 o_color;\n"
    "uniform mat4 u_proj;\n"
    "void main() {\n"
    "    o_color = i_color;\n"
    "    gl_Position = u_proj * vec4(i_position.x, i_position.y, 0.0, 1.0);\n"
    "}";

static const std::string dfrag_colored_ =
    "#version 330 core\n"
    "in vec4 o_color;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = o_color;\n"
    "}";

static std::shared_ptr<program> builtin_colored_ = nullptr, builtin_textured_ = nullptr;

std::shared_ptr<program> program::make(builtin_program_type type) {
    if (builtin_colored_ == nullptr || builtin_textured_ == nullptr) {
        builtin_colored_ = program::make(dvert_colored_, dfrag_colored_, [](program* program) {
            program->get_attrib(0).layout(shader_vertex_data_type::f32, 2, 16, 0);
            program->get_attrib(1).layout(shader_vertex_data_type::f16, 4, 16, 8);

            if (program->cached_uniforms.size() > 0) return;
            program->cache_uniform("u_proj");  // 0
        });
        builtin_textured_ = program::make(dvert_textured_, dfrag_textured_, [](program* program) {
            program->get_attrib(0).layout(shader_vertex_data_type::f32, 2, 24, 0);
            program->get_attrib(1).layout(shader_vertex_data_type::f16, 4, 24, 8);
            program->get_attrib(2).layout(shader_vertex_data_type::f32, 2, 24, 16);

            if (program->cached_uniforms.size() > 0) return;
            program->cache_uniform("u_proj");                     // 0
            program->cache_uniform("u_tex").set_texture_unit(1);  // 1
        });
    }
    switch (type) {
        case builtin_program_type::colored:
            return builtin_colored_;
        case builtin_program_type::textured:
            return builtin_textured_;
    }
    return nullptr;
}

}  // namespace arc
