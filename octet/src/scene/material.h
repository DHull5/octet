////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012, 2013
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Material 
//
//
// Materials are represented as GL textures with solid colours as single pixel textures.
// This simplifies shader design.
//

namespace octet {
  class material : public resource {
    // material
    GLuint diffuse;
    GLuint ambient;
    GLuint emission;
    GLuint specular;
    GLuint bump;

    float shininess;

    void bind_textures() const {
      // set textures 0, 1, 2, 3 to their respective values
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, diffuse);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, ambient);
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, emission);
      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_2D, specular);
      glActiveTexture(GL_TEXTURE4);
      glBindTexture(GL_TEXTURE_2D, bump);
      glActiveTexture(GL_TEXTURE0);
    }

  public:
    RESOURCE_META(material)

    // default constructor makes a blank material.
    material() {
      diffuse = 0;
      ambient = 0;
      emission = 0;
      specular = 0;
      bump = 0;
      shininess = 0;
    }

    material(const char *texture) {
      specular = diffuse = ambient = resources::get_texture_handle(GL_RGBA, "#00000000");
      emission = resources::get_texture_handle(GL_RGBA, texture);
      bump = resources::get_texture_handle(GL_RGBA, "#8080ff00");
      shininess = 30.0f;
    }

    void visit(visitor &v) {
    }

    void init(GLuint diffuse, GLuint ambient, GLuint emission, GLuint specular, GLuint bump, float shininess) {
      this->diffuse = diffuse;
      this->ambient = ambient;
      this->emission = emission;
      this->specular = specular;
      this->bump = bump;
      this->shininess = shininess;
    }

    // make a solid color with a specular highlight
    void make_color(const vec4 &color, bool bumpy, bool shiny) {
      char name[16];
      sprintf(name, "#%02x%02x%02x%02x", (int)(color[0]*255.0f+0.5f), (int)(color[1]*255.0f+0.5f), (int)(color[2]*255.0f+0.5f), (int)(color[3]*255.0f+0.5f));
      diffuse = ambient = resources::get_texture_handle(GL_RGBA, name);
      emission = resources::get_texture_handle(GL_RGBA, "#00000000");
      specular = resources::get_texture_handle(GL_RGBA, shiny ? "#80808000" : "#00000000");
      bump = resources::get_texture_handle(GL_RGBA, bumpy ? "!bump" : "#8080ff00");
      shininess = 30.0f;
    }

    void render(bump_shader &shader, const mat4t &modelToProjection, const mat4t &modelToCamera, vec4 *light_uniforms, int num_light_uniforms, int num_lights) const {
      shader.render(modelToProjection, modelToCamera, light_uniforms, num_light_uniforms, num_lights);
      bind_textures();
    }

    void render_skinned(bump_shader &shader, const mat4t &cameraToProjection, const mat4t *modelToCamera, int num_nodes, vec4 *light_uniforms, int num_light_uniforms, int num_lights) const {
      shader.render_skinned(cameraToProjection, modelToCamera, num_nodes, light_uniforms, num_light_uniforms, num_lights);
      bind_textures();
    }

    //void render(bump_shader &shader, const mat4t &modelToProjection, const mat4t &modelToCamera, const vec4 &light_direction, vec4 &light_ambient, vec4 &light_diffuse, vec4 &light_specular);
  };
}
