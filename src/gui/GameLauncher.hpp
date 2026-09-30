#pragma once

#include "GameCatalogue.hpp"
#include "SceneManager.hpp"

#include <functional>
#include <map>
#include <string>

// Binds catalogue entries to the scenes that start them. Dedicated games map to their own
// setup scene; shared board games go through one board-game launcher callback.
class GameLauncher {
public:
    using BoardGameLauncher = std::function<void(const catalogue::GameDescriptor&)>;

    explicit GameLauncher(SceneManager& scenes);

    void bindDedicatedScene(const std::string& key, SceneId setupScene);
    void setBoardGameLauncher(BoardGameLauncher launcher);

    bool canLaunch(const catalogue::GameDescriptor& game) const;
    // Opens the game's setup. Returns false (and changes nothing) when the game cannot be
    // started in the GUI.
    bool launch(const catalogue::GameDescriptor& game);

private:
    SceneManager& scenes_;
    std::map<std::string, SceneId> dedicated_;
    BoardGameLauncher boardGameLauncher_;
};
