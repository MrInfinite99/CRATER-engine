#pragma once
#include <entt/entt.hpp>

namespace CRATER::Scene
{

    class Scene; // forward declaration

    class System {
    public:
        virtual ~System() = default;
        virtual void update(float deltaTime, Scene& scene) = 0;
        virtual void init() = 0;
    };

}