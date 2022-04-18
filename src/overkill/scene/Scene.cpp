#include "Scene.hpp"
#include <overkill/scene/Components/SimpleMeshRendered.hpp>
#include <overkill/ShaderSystem.hpp>
#include <overkill/TextureSystem.hpp>
#include <overkill/graphics_internal/FrameBuffer.hpp>

#include <glm/gtc/quaternion.hpp>

#include <iostream>

namespace OK
{
    Scene::Scene(const char* name)
        : name {name},
        objects{},
        components{},
        viewport{}
    {
        scene_graph = new Transform();

        const std::type_index renderer_TypeName = std::type_index(typeid(GraphicsComponent));
        components[renderer_TypeName] = new ComponentRegistry<GraphicsComponent>();
        renderers = (ComponentRegistry<GraphicsComponent>*)components[renderer_TypeName];

        const std::type_index lightsource_TypeName = std::type_index(typeid(LightSource));
        components[lightsource_TypeName] = new ComponentRegistry<LightSource>();
        light_sources = (ComponentRegistry<LightSource>*)components[lightsource_TypeName];

        // TODO: this is quite ugly -> Ideally order of construction should not matter!
        shared_matrices_buffer = ShaderSystem::get_uniform_buffer("OK_Commons");
        light_buffer = ShaderSystem::get_uniform_buffer("OK_Lights");

        camera = add_game_object("main_camera")->add_component<Camera>();
    }
    inline constexpr const std::vector<GameObject*>& Scene::get_objects() const
    {
        return std::as_const(objects);
    }

    Transform* const & Scene::root() const
    {
        return scene_graph;
    }

    Camera* const& Scene::active_camera() const
    {
        return camera;
    }

    GameObject*& Scene::add_game_object(const char* name)
    {
        GameObject* go = new GameObject(name, this);
        go->transform = go->add_component<Transform>();
        objects.emplace_back(go);
        return go;
    }

    void Scene::propagate_scene_graph()
    {
        propagate_trs(scene_graph, glm::mat4(1.0f));
    }

    void Scene::start_scene()
    {
        //renderers = (ComponentRegistry<GraphicsComponent>*)components.find(std::type_index(typeid(GraphicsComponent)))->second;
        //Gather data for running the rendering

    }

    void Scene::render()
    {
        if (main_framebuffer)
        {
            main_framebuffer->bind(); 
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        // TODO: Update range of lights in one update call by batching together the data beforehand
        //void* raw_light_data = malloc(light_buffer->size());
        //std::fill_n(raw_light_data, light_buffer->size(), 0);
        for (const auto& light_component : light_sources->registered_components) 
        {
            auto packed = OK::pack_data(
                glm::vec4{ light_component->light.intensities, 1 },
                glm::vec4{ glm::eulerAngles(light_component->transform()->rotation), 1 }
            );
            switch (light_component->light_type)
            {
            case LightSource::Type::DIRECTIONAL:;
                
                light_buffer->update(sizeof(packed), &packed);
                break;
            default:
                std::cout << "Light source type not implemented!" << std::endl;
                break;
            }
        }
        //free(raw_light_data);

        for (const auto& r : renderers->registered_components) {
            r->render();
        }
        if (main_framebuffer)
        {
            main_framebuffer->unbind();
            viewport.render();
        }
        
    }

    void Scene::update(float dt)
    {
        for (auto& go : objects)
        {
            for (auto& comp : go->components) 
            {
                comp->update(dt);
            }
        }
    }
    void Scene::late_update(float dt)
    {
        for (auto& go : objects)
        {
            for (auto& comp : go->components)
            {
                comp->late_update(dt);
            }
        }

        auto packed_matrices = OK::pack_data(
            camera->projection_matrix,
            camera->view_matrix
        );

        shared_matrices_buffer->update(sizeof(packed_matrices), &packed_matrices);
    }
    
    void Scene::propagate_trs(Transform* transform, glm::mat4 parent_trs)
    {
        transform->trs = parent_trs * (
            glm::translate(transform->position) * 
            glm::rotate(transform->rotation.y, glm::vec3(0, 1, 0)) * 
            glm::rotate(transform->rotation.x, glm::vec3(1, 0, 0)) * 
            glm::rotate(transform->rotation.z, glm::vec3(0, 0, 1)) * 
            glm::scale(transform->scale)
        );
        for (auto& child : transform->children)
        {
            propagate_trs(child, transform->trs);
        }
    }

    Scene::~Scene()
    {
        for (auto game_object : objects) {
            delete game_object;
        }
        objects.clear();
        delete scene_graph;
    }
}
