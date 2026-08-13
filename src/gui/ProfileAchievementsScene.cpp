#include "ProfileAchievementsScene.hpp"

#include "Theme.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace {
std::string unlockedDate(std::int64_t milliseconds)
{
    const std::time_t raw = static_cast<std::time_t>(milliseconds / 1000);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &raw);
#else
    gmtime_r(&raw, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%d %H:%M UTC");
    return out.str();
}
} // namespace

ProfileAchievementsScene::ProfileAchievementsScene(
    const sf::Font& regularFont, const sf::Font& semiboldFont, SceneManager& scenes,
    persistence::ProfileService& profiles, persistence::AchievementService& achievements,
    std::int64_t& selectedProfileId)
    : scenes_(scenes), profiles_(profiles), achievements_(achievements),
      selectedProfileId_(selectedProfileId), regularFont_(regularFont), semiboldFont_(semiboldFont),
      kicker_(semiboldFont, "PLAYER PROFILE", Theme::labelSize),
      title_(semiboldFont, "Achievements", Theme::pageTitleSize),
      subtitle_(regularFont, "", Theme::bodySize), summary_(semiboldFont, "", 20),
      scrollStatus_(regularFont, "", 14),
      buttons_{UiButton(semiboldFont, "All", {130.f, 44.f}),
               UiButton(semiboldFont, "General", {160.f, 44.f}),
               UiButton(semiboldFont, "Tic-Tac-Toe", {210.f, 44.f}),
               UiButton(semiboldFont, "Ping Pong", {170.f, 44.f}),
               UiButton(semiboldFont, "Back", {150.f, 48.f})}
{
    kicker_.setPosition({72.f, 32.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({72.f, 57.f});
    title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({72.f, 110.f});
    subtitle_.setFillColor(Theme::textSecondary);
    summary_.setPosition({940.f, 76.f});
    summary_.setFillColor(Theme::warning);
    scrollStatus_.setPosition({72.f, 664.f});
    scrollStatus_.setFillColor(Theme::textMuted);
    buttons_[0].setPosition({72.f, 150.f});
    buttons_[1].setPosition({218.f, 150.f});
    buttons_[2].setPosition({394.f, 150.f});
    buttons_[3].setPosition({620.f, 150.f});
    buttons_[4].setPosition({1058.f, 646.f});
    select(0);
}

void ProfileAchievementsScene::onActivate()
{
    const auto profiles = profiles_.listProfiles();
    auto found = std::find_if(profiles.begin(), profiles.end(),
        [this](const auto& profile) { return profile.id == selectedProfileId_; });
    if (found == profiles.end()) {
        const auto active = profiles_.activeProfile();
        if (!active) {
            scenes_.switchTo(SceneId::Profiles);
            return;
        }
        selectedProfileId_ = active->id;
        found = std::find_if(profiles.begin(), profiles.end(),
            [this](const auto& profile) { return profile.id == selectedProfileId_; });
    }
    subtitle_.setString(found->displayName);
    statuses_ = achievements_.statuses(selectedProfileId_);
    std::stable_sort(statuses_.begin(), statuses_.end(), [](const auto& left, const auto& right) {
        if (left.unlockedAt.has_value() != right.unlockedAt.has_value()) return left.unlockedAt.has_value();
        if (left.unlockedAt && right.unlockedAt) return *left.unlockedAt > *right.unlockedAt;
        return false;
    });
    const auto unlocked = std::count_if(statuses_.begin(), statuses_.end(),
        [](const auto& status) { return status.unlockedAt.has_value(); });
    summary_.setString(std::to_string(unlocked) + " / 12 unlocked");
    category_ = 0;
    selected_ = 0;
    scroll_ = 0;
    rebuildFilter();
}

void ProfileAchievementsScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) activate(4);
        else if (key->code == sf::Keyboard::Key::Tab || key->code == sf::Keyboard::Key::Right)
            select((selected_ + 1) % buttons_.size());
        else if (key->code == sf::Keyboard::Key::Left)
            select((selected_ + buttons_.size() - 1) % buttons_.size());
        else if (key->code == sf::Keyboard::Key::Up) moveScroll(-1);
        else if (key->code == sf::Keyboard::Key::Down) moveScroll(1);
        else if (key->code == sf::Keyboard::Key::PageUp) {
            for (std::size_t count = 0; count < visibleRows; ++count) moveScroll(-1);
        } else if (key->code == sf::Keyboard::Key::PageDown) {
            for (std::size_t count = 0; count < visibleRows; ++count) moveScroll(1);
        } else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
            activate(selected_);
        }
    }
    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        moveScroll(wheel->delta > 0.f ? -1 : 1);
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        for (std::size_t index = 0; index < buttons_.size(); ++index) {
            buttons_[index].setHovered(buttons_[index].contains(point));
            if (buttons_[index].contains(point)) select(index);
        }
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        for (std::size_t index = 0; index < buttons_.size(); ++index) {
            if (buttons_[index].contains(point)) activate(index);
        }
    }
}

void ProfileAchievementsScene::update(sf::Time deltaTime)
{
    for (auto& button : buttons_) button.update(deltaTime);
}

void ProfileAchievementsScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_);
    window.draw(title_);
    window.draw(subtitle_);
    window.draw(summary_);
    window.draw(scrollStatus_);
    for (const auto& button : buttons_) button.draw(window);

    const auto end = std::min(filtered_.size(), scroll_ + visibleRows);
    for (std::size_t index = scroll_; index < end; ++index) {
        const auto& status = filtered_[index];
        const float y = 210.f + static_cast<float>(index - scroll_) * 102.f;
        sf::RectangleShape card({1136.f, 88.f});
        card.setPosition({72.f, y});
        card.setFillColor(Theme::backgroundRaised);
        card.setOutlineThickness(1.f);
        card.setOutlineColor(status.unlockedAt ? Theme::secondary : Theme::border);
        window.draw(card);

        sf::Text cardTitle(semiboldFont_, status.definition.title, 20);
        cardTitle.setPosition({94.f, y + 11.f});
        cardTitle.setFillColor(Theme::textPrimary);
        window.draw(cardTitle);
        sf::Text description(regularFont_, status.definition.description, 15);
        description.setPosition({94.f, y + 44.f});
        description.setFillColor(Theme::textSecondary);
        window.draw(description);

        std::string state;
        if (status.unlockedAt) state = "Unlocked  |  " + unlockedDate(*status.unlockedAt);
        else if (status.progress) state = "Locked  |  Progress " + std::to_string(status.progress->current) +
                                          " / " + std::to_string(status.progress->target);
        else state = "Locked  |  Complete the listed condition";
        sf::Text stateText(regularFont_, state, 14);
        stateText.setPosition({815.f, y + 35.f});
        stateText.setFillColor(status.unlockedAt ? Theme::secondary : Theme::textMuted);
        window.draw(stateText);
    }
}

void ProfileAchievementsScene::activate(std::size_t index)
{
    if (index < 4) {
        category_ = index;
        scroll_ = 0;
        rebuildFilter();
    } else {
        scenes_.switchTo(SceneId::ProfileStats);
    }
}

void ProfileAchievementsScene::select(std::size_t index)
{
    selected_ = index;
    for (std::size_t button = 0; button < buttons_.size(); ++button) {
        buttons_[button].setSelected(button == selected_ || (button < 4 && button == category_));
    }
}

void ProfileAchievementsScene::moveScroll(int direction)
{
    const std::size_t maximum = filtered_.size() > visibleRows ? filtered_.size() - visibleRows : 0;
    if (direction < 0 && scroll_ > 0) --scroll_;
    if (direction > 0 && scroll_ < maximum) ++scroll_;
    rebuildFilter();
}

void ProfileAchievementsScene::rebuildFilter()
{
    filtered_.clear();
    std::copy_if(statuses_.begin(), statuses_.end(), std::back_inserter(filtered_),
                 [this](const auto& status) { return matchesFilter(status); });
    const std::size_t maximum = filtered_.size() > visibleRows ? filtered_.size() - visibleRows : 0;
    scroll_ = std::min(scroll_, maximum);
    const std::size_t first = filtered_.empty() ? 0 : scroll_ + 1;
    const std::size_t last = std::min(filtered_.size(), scroll_ + visibleRows);
    scrollStatus_.setString("Showing " + std::to_string(first) + "-" + std::to_string(last) +
                            " of " + std::to_string(filtered_.size()) +
                            "  |  Mouse wheel or Up / Down scrolls");
    select(selected_);
}

bool ProfileAchievementsScene::matchesFilter(const achievements::AchievementStatus& status) const
{
    if (category_ == 0) return true;
    if (category_ == 1) return status.definition.category == achievements::AchievementCategory::General;
    if (category_ == 2) return status.definition.category == achievements::AchievementCategory::TicTacToe;
    return status.definition.category == achievements::AchievementCategory::PingPong;
}
