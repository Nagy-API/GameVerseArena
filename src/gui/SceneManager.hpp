#pragma once

#include "Scene.hpp"

#include <array>
#include <cstddef>
#include <memory>

enum class SceneId : std::size_t {
    MainMenu,
    GameLibrary,
    Settings,
    About,
    Count
};

class SceneManager {
public:
    void add(SceneId id, std::unique_ptr<Scene> scene);
    void switchTo(SceneId id);

    Scene& active();
    const Scene& active() const;
    SceneId activeId() const noexcept { return activeId_; }

private:
    static constexpr std::size_t sceneCount = static_cast<std::size_t>(SceneId::Count);
    std::array<std::unique_ptr<Scene>, sceneCount> scenes_{};
    SceneId activeId_{SceneId::MainMenu};
};
