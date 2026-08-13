#pragma once

#include "MatchRepository.hpp"
#include "ProfileService.hpp"
#include "Scene.hpp"
#include "SceneManager.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/Text.hpp>

#include <array>
#include <cstdint>
#include <vector>

class MatchHistoryScene final : public Scene {
public:
    MatchHistoryScene(const sf::Font& regularFont,const sf::Font& semiboldFont,SceneManager& scenes,
        persistence::ProfileService& profiles,persistence::MatchRepository& matches,std::int64_t& selectedProfileId);
    void handleEvent(const sf::Event&,sf::RenderWindow&)override; void update(sf::Time)override;
    void render(sf::RenderWindow&)const override; void onResize(sf::Vector2u)override{} void onActivate()override;
private:
    static constexpr std::size_t pageSize=8;
    void reload(); void refresh(); void activate(std::size_t); void select(std::size_t);
    SceneManager& scenes_; persistence::ProfileService& profiles_; persistence::MatchRepository& matches_; std::int64_t& profileId_;
    const sf::Font& regular_; sf::Text title_; sf::Text subtitle_; sf::Text status_; sf::Text empty_;
    std::vector<sf::Text> rows_; std::array<UiButton,5> buttons_; persistence::MatchFilter filter_; std::size_t gameFilter_{};std::size_t resultFilter_{};std::size_t page_{};std::size_t selected_{};std::int64_t total_{};
};
