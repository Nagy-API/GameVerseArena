#include "SceneManager.hpp"

#include <stdexcept>

namespace {
std::size_t indexOf(SceneId id)
{
    return static_cast<std::size_t>(id);
}
} // namespace

void SceneManager::add(SceneId id, std::unique_ptr<Scene> scene)
{
    if (!scene || id == SceneId::Count) {
        throw std::invalid_argument("SceneManager requires a valid scene and identifier");
    }
    scenes_[indexOf(id)] = std::move(scene);
}

void SceneManager::switchTo(SceneId id)
{
    if (id == SceneId::Count || !scenes_[indexOf(id)]) {
        throw std::logic_error("Requested scene has not been registered");
    }
    activeId_ = id;
}

Scene& SceneManager::active()
{
    auto& scene = scenes_[indexOf(activeId_)];
    if (!scene) {
        throw std::logic_error("Active scene has not been registered");
    }
    return *scene;
}

const Scene& SceneManager::active() const
{
    const auto& scene = scenes_[indexOf(activeId_)];
    if (!scene) {
        throw std::logic_error("Active scene has not been registered");
    }
    return *scene;
}
