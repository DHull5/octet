////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012, 2013
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Scene Node heirachy
//

namespace octet {
  class scene : public scene_node {
    ///////////////////////////////////////////
    //
    // rendering information
    //

    // each of these is a set of (scene_node, mesh, material)
    dynarray<ref<mesh_instance>> mesh_instances;

    // animations playing at the moment
    dynarray<ref<animation_instance>> animation_instances;

    // cameras available
    dynarray<ref<camera_instance>> camera_instances;

    // lights available
    dynarray<ref<light_instance>> light_instances;

    enum { max_lights = 4, light_size = 4 };
    int num_light_uniforms;
    int num_lights;
    vec4 light_uniforms[1 + max_lights * light_size ];

    int frame_number;

    void calc_lighting(const mat4t &worldToCamera) {
      vec4 &ambient = light_uniforms[0];
      ambient = vec4(0, 0, 0, 1);
      num_lights = 0;
      int num_ambient = 0;
      for (unsigned i = 0; i != light_instances.size() && num_lights != max_lights; ++i) {
        light_instance *li = light_instances[i];
        scene_node *node = li->get_scene_node();
        atom_t kind = li->get_kind();
        if (kind == atom_ambient) {
          ambient += li->get_color();
          num_ambient++;
        } else {
          li->get_fragment_uniforms(&light_uniforms[1+num_lights*light_size], worldToCamera);
          num_lights++;
        }
      }
      if (num_ambient == 0) {
        ambient = vec4(0.5f, 0.5f, 0.5f, 1);
      }
      num_light_uniforms = 1 + num_lights * light_size;
    }

    void render_impl(bump_shader &object_shader, bump_shader &skin_shader, camera_instance &cam, float aspect_ratio) {
      mat4t cameraToWorld = cam.get_node()->calcModelToWorld();

      mat4t worldToCamera;
      cameraToWorld.invertQuick(worldToCamera);

      calc_lighting(worldToCamera);

      cam.set_cameraToWorld(cameraToWorld, aspect_ratio);
      mat4t cameraToProjection = cam.get_cameraToProjection();

      for (unsigned mesh_index = 0; mesh_index != mesh_instances.size(); ++mesh_index) {
        mesh_instance *mi = mesh_instances[mesh_index];
        mesh *msh = mi->get_mesh();
        skin *skn = msh->get_skin();
        skeleton *skel = mi->get_skeleton();
        material *mat = mi->get_material();

        mat4t modelToWorld = mi->get_node()->calcModelToWorld();
        mat4t modelToCamera;
        mat4t modelToProjection;
        cam.get_matrices(modelToProjection, modelToCamera, modelToWorld);

        if (!skel || !skn) {
          // normal rendering for single matrix objects
          // build a projection matrix: model -> world -> camera_instance -> projection
          // the projection space is the cube -1 <= x/w, y/w, z/w <= 1
          mat->render(object_shader, modelToProjection, modelToCamera, light_uniforms, num_light_uniforms, num_lights);
        } else {
          // multi-matrix rendering
          mat4t *transforms = skel->calc_transforms(modelToCamera, skn);
          int num_bones = skel->get_num_bones();
          assert(num_bones < 64);
          mat->render_skinned(skin_shader, cameraToProjection, transforms, num_bones, light_uniforms, num_light_uniforms, num_lights);
        }
        msh->render();
      }
      frame_number++;
    }
  public:
    RESOURCE_META(scene)

    // create an empty scene
    scene() {
      frame_number = 0;
    }

    void visit(visitor &v) {
      scene_node::visit(v);
      v.visit(mesh_instances, "mesh_instances");
      v.visit(animation_instances, "animation_instances");
      v.visit(camera_instances, "camera_instances");
      v.visit(light_instances, "light_instances");
    }

    void create_default_camera_and_lights() {
      // default camera_instance
      if (camera_instances.size() == 0) {
        scene_node *node = add_scene_node();
        camera_instance *cam = new camera_instance();
        node->access_nodeToParent().translate(0, 0, 100);
        float n = 0.1f, f = 5000.0f;
        cam->set_node(node);
        cam->set_perspective(1, 1, 1, n, f);
        camera_instances.push_back(cam);
      }

      // default light instance
      if (light_instances.size() == 0) {
        scene_node *node = add_scene_node();
        light_instance *li = new light_instance();
        node->access_nodeToParent().translate(100, 100, 100);
        node->access_nodeToParent().rotateX(45);
        node->access_nodeToParent().rotateY(45);
        float n = 0.1f, f = 5000.0f;
        li->set_kind(atom_directional);
        li->set_node(node);
        light_instances.push_back(li);
      }
    }

    void play_all_anims(resources &dict) {
      dynarray<resource*> anims;
      dict.find_all(anims, atom_animation);

      for (unsigned i = 0; i != anims.size(); ++i) {
        animation *anim = anims[i]->get_animation();
        if (anim) {
          play(anim, true);
        }
      }
    }

    scene_node *add_scene_node() {
      scene_node *new_node = new scene_node();
      scene_node::add_child(new_node);
      return new_node;
    }

    void add_mesh_instance(mesh_instance *inst=0) {
      mesh_instances.push_back(inst);
    }

    void add_animation_instance(animation_instance *inst) {
      animation_instances.push_back(inst);
    }

    void add_camera_instance(camera_instance *inst) {
      camera_instances.push_back(inst);
    }

    void add_light_instance(light_instance *inst) {
      light_instances.push_back(inst);
    }

    // how many mesh instances do we have?
    int num_mesh_instances() {
      return (int)mesh_instances.size();
    }

    // how many camera_instances do we have?
    int num_camera_instances() {
      return (int)camera_instances.size();
    }

    // how many light_instances do we have?
    int num_light_instances() {
      return (int)light_instances.size();
    }

    scene_node *get_root_node() {
      return (scene_node*)this;
    }

    // access camera_instance information
    camera_instance *get_camera_instance(int index) {
      return camera_instances[index];
    }

    // advance all the animation instances
    // note that we want to update before rendering or doing physics and AI actions.
    void update(float delta_time) {
      for (int idx = 0; idx != animation_instances.size(); ++idx) {
        animation_instance *inst = animation_instances[idx];
        inst->update(delta_time);
      }

      for (int idx = 0; idx != mesh_instances.size(); ++idx) {
        mesh_instance *inst = mesh_instances[idx];
        inst->update(delta_time);
      }
    }

    // call OpenGL to draw all the mesh instances (scene_node + mesh + material)
    void render(bump_shader &object_shader, bump_shader &skin_shader, camera_instance &cam, float aspect_ratio) {
      render_impl(object_shader, skin_shader, cam, aspect_ratio);
    }

    // play an animation on another target (not the same one as in the collada file)
    void play(animation *anim, animation_target *target, bool is_looping) {
      animation_instance *inst = new animation_instance(anim, target, is_looping);
      animation_instances.push_back(inst);
    }

    // play an animation with built-in targets (as in the collada file)
    void play(animation *anim, bool is_looping) {
      animation_instance *inst = new animation_instance(anim, NULL, is_looping);
      animation_instances.push_back(inst);
    }

    // find a mesh instance for a node
    mesh_instance *get_first_mesh_instance(scene_node *node) {
      for (int i = 0; i != mesh_instances.size(); ++i) {
        if (mesh_instances[i]->get_node() == node) {
          return mesh_instances[i];
        }
      }
      return NULL;
    }

    void dump(FILE *file) {
      xml_writer xml(file);
      visit(xml);
    }
  };
}
