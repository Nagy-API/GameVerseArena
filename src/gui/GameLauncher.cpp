#include "GameLauncher.hpp"

#include <utility>

GameLauncher::GameLauncher(SceneManager& scenes) : scenes_(scenes) {}

void GameLauncher::bindDedicatedScene(const std::string& key, SceneId setupScene)
{
    dedicated_[key] = setupScene;
}

void GameLauncher::setBoardGameLauncher(BoardGameLauncher launcher)
{
    boardGameLauncher_ = std::move(launcher);
}

bool GameLauncher::canLaunch(const catalogue::GameDescriptor& game) const
{
    switch (game.launch) {
    case catalogue::LaunchKind::DedicatedScene: return dedicated_.count(game.key) != 0;
    case catalogue::LaunchKind::BoardGame: return static_cast<bool>(boardGameLauncher_) && static_cast<bool>(game.createGame);
    case catalogue::LaunchKind::ConsoleOnly: break;
    }
    return false;
}

bool GameLauncher::launch(const catalogue::GameDescriptor& game)
{
    if (!canLaunch(game)) return false;
    if (game.launch == catalogue::LaunchKind::DedicatedScene) {
        scenes_.switchTo(dedicated_.at(game.key));
    } else {
        boardGameLauncher_(game);
    }
    return true;
}
