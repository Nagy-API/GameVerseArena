#include "ProfilesScene.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <string>

ProfilesScene::ProfilesScene(const sf::Font& regularFont, const sf::Font& semiboldFont,
                             SceneManager& sceneManager, persistence::ProfileService& profileService)
    : sceneManager_(sceneManager), profileService_(profileService), regularFont_(regularFont),
      kicker_(semiboldFont, "LOCAL PLAYERS", Theme::labelSize),
      title_(semiboldFont, "Player Profiles", Theme::pageTitleSize),
      subtitle_(regularFont, "Choose the local name used as Player 1's default in graphical games.", Theme::bodySize),
      help_(regularFont, "Up / Down selects a profile  |  Tab or Left / Right selects an action  |  Enter activates", 14),
      scrollStatus_(regularFont, "", 14),
      actions_{UiButton(semiboldFont, "Create Profile", {318.f, 56.f}),
               UiButton(semiboldFont, "Rename", {318.f, 56.f}),
               UiButton(semiboldFont, "Delete", {318.f, 56.f}),
               UiButton(semiboldFont, "Set Active", {318.f, 56.f}),
               UiButton(semiboldFont, "Back", {318.f, 56.f})},
      editOverlay_(regularFont, semiboldFont), deleteOverlay_(regularFont, semiboldFont)
{
    kicker_.setPosition({Theme::pageMargin, 42.f}); kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 70.f}); title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 125.f}); subtitle_.setFillColor(Theme::textSecondary);
    help_.setPosition({Theme::pageMargin, 680.f}); help_.setFillColor(Theme::textMuted);
    scrollStatus_.setPosition({72.f, 642.f}); scrollStatus_.setFillColor(Theme::textMuted);

    names_.reserve(visibleCount); badges_.reserve(visibleCount);
    for (std::size_t index = 0; index < visibleCount; ++index) {
        const float y = 180.f + static_cast<float>(index) * 74.f;
        cards_[index].setSize({760.f, 62.f}); cards_[index].setPosition({72.f, y});
        cards_[index].setFillColor(Theme::backgroundRaised); cards_[index].setOutlineThickness(1.f);
        names_.emplace_back(regularFont_, "", 21); names_.back().setPosition({96.f, y + 18.f});
        names_.back().setFillColor(Theme::textPrimary);
        badges_.emplace_back(semiboldFont, "", Theme::labelSize); badges_.back().setPosition({680.f, y + 21.f});
        badges_.back().setFillColor(Theme::secondary);
    }
    for (std::size_t index = 0; index < actionCount; ++index) {
        actions_[index].setPosition({890.f, 190.f + static_cast<float>(index) * 74.f});
    }
    refresh();
}

void ProfilesScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (editOverlay_.isOpen()) {
        const auto result = editOverlay_.handleEvent(event, window);
        if (result == ProfileEditOverlay::Result::Confirm) {
            try {
                persistence::Profile changed;
                if (pendingProfileId_ == 0) changed = profileService_.createProfile(editOverlay_.value());
                else changed = profileService_.renameProfile(pendingProfileId_, editOverlay_.value());
                editOverlay_.close();
                reload(changed.id);
            } catch (const persistence::ProfileError& error) {
                editOverlay_.setError(error.what());
            }
        }
        return;
    }
    if (deleteOverlay_.isOpen()) {
        const auto result = deleteOverlay_.handleEvent(event, window);
        if (result == ProfileDeleteOverlay::Result::Confirm) {
            profileService_.deleteProfile(pendingProfileId_);
            deleteOverlay_.close();
            reload();
        }
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) sceneManager_.switchTo(SceneId::MainMenu);
        else if (key->code == sf::Keyboard::Key::Up) moveProfileSelection(-1);
        else if (key->code == sf::Keyboard::Key::Down) moveProfileSelection(1);
        else if (key->code == sf::Keyboard::Key::Tab || key->code == sf::Keyboard::Key::Right) moveActionSelection(1);
        else if (key->code == sf::Keyboard::Key::Left) moveActionSelection(-1);
        else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) activateAction(selectedAction_);
    }
    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        moveProfileSelection(wheel->delta > 0.f ? -1 : 1);
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        for (std::size_t slot = 0; slot < visibleCount; ++slot) {
            if (firstVisible_ + slot < profiles_.size() && cards_[slot].getGlobalBounds().contains(point)) {
                selectedProfileIndex_ = firstVisible_ + slot;
            }
        }
        for (std::size_t action = 0; action < actionCount; ++action) {
            const bool hovered = actions_[action].contains(point);
            actions_[action].setHovered(hovered);
            if (hovered) selectedAction_ = action;
        }
        refresh();
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        for (std::size_t slot = 0; slot < visibleCount; ++slot) {
            if (firstVisible_ + slot < profiles_.size() && cards_[slot].getGlobalBounds().contains(point)) {
                selectedProfileIndex_ = firstVisible_ + slot; refresh(); return;
            }
        }
        for (std::size_t action = 0; action < actionCount; ++action) {
            if (actions_[action].contains(point)) { selectedAction_ = action; activateAction(action); return; }
        }
    }
}

void ProfilesScene::update(sf::Time deltaTime)
{
    if (editOverlay_.isOpen()) { editOverlay_.update(deltaTime); return; }
    if (deleteOverlay_.isOpen()) { deleteOverlay_.update(deltaTime); return; }
    for (auto& action : actions_) action.update(deltaTime);
}

void ProfilesScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_); window.draw(title_); window.draw(subtitle_);
    for (std::size_t slot = 0; slot < visibleCount && firstVisible_ + slot < profiles_.size(); ++slot) {
        window.draw(cards_[slot]); window.draw(names_[slot]); window.draw(badges_[slot]);
    }
    for (const auto& action : actions_) action.draw(window);
    window.draw(scrollStatus_); window.draw(help_);
    editOverlay_.render(window); deleteOverlay_.render(window);
}

void ProfilesScene::onResize(sf::Vector2u) {}

void ProfilesScene::onActivate()
{
    selectedAction_ = 3;
    reload();
}

void ProfilesScene::moveProfileSelection(int offset)
{
    if (profiles_.empty()) return;
    const int next = std::clamp(static_cast<int>(selectedProfileIndex_) + offset, 0,
                                static_cast<int>(profiles_.size()) - 1);
    selectedProfileIndex_ = static_cast<std::size_t>(next);
    if (selectedProfileIndex_ < firstVisible_) firstVisible_ = selectedProfileIndex_;
    if (selectedProfileIndex_ >= firstVisible_ + visibleCount) firstVisible_ = selectedProfileIndex_ - visibleCount + 1;
    refresh();
}

void ProfilesScene::moveActionSelection(int offset)
{
    selectedAction_ = static_cast<std::size_t>((static_cast<int>(selectedAction_) + offset +
                                                static_cast<int>(actionCount)) % static_cast<int>(actionCount));
    refresh();
}

void ProfilesScene::activateAction(std::size_t action)
{
    const auto selected = selectedProfile();
    if (action == 0) {
        pendingProfileId_ = 0; editOverlay_.open(ProfileEditOverlay::Mode::Create);
    } else if (action == 1 && selected.has_value()) {
        pendingProfileId_ = selected->id; editOverlay_.open(ProfileEditOverlay::Mode::Rename, selected->displayName);
    } else if (action == 2 && selected.has_value()) {
        pendingProfileId_ = selected->id; deleteOverlay_.open(selected->displayName, selectedIsActive());
    } else if (action == 3 && selected.has_value()) {
        profileService_.setActiveProfile(selected->id); reload(selected->id);
    } else if (action == 4) {
        sceneManager_.switchTo(SceneId::MainMenu);
    }
}

void ProfilesScene::reload(std::optional<std::int64_t> preferredId)
{
    profiles_ = profileService_.listProfiles();
    selectedProfileIndex_ = 0;
    if (preferredId.has_value()) {
        const auto found = std::find_if(profiles_.begin(), profiles_.end(),
            [preferredId](const persistence::Profile& profile) { return profile.id == *preferredId; });
        if (found != profiles_.end()) selectedProfileIndex_ = static_cast<std::size_t>(found - profiles_.begin());
    }
    firstVisible_ = selectedProfileIndex_ >= visibleCount ? selectedProfileIndex_ - visibleCount + 1 : 0;
    refresh();
}

void ProfilesScene::refresh()
{
    const auto active = profileService_.activeProfile();
    for (std::size_t slot = 0; slot < visibleCount; ++slot) {
        const std::size_t index = firstVisible_ + slot;
        const bool present = index < profiles_.size();
        names_[slot].setString(present ? profiles_[index].displayName : "");
        badges_[slot].setString(present && active.has_value() && profiles_[index].id == active->id ? "ACTIVE" : "");
        cards_[slot].setOutlineColor(present && index == selectedProfileIndex_ ? Theme::primaryBright : Theme::border);
        cards_[slot].setOutlineThickness(present && index == selectedProfileIndex_ ? 2.f : 1.f);
    }
    for (std::size_t action = 0; action < actionCount; ++action) actions_[action].setSelected(action == selectedAction_);
    scrollStatus_.setString(profiles_.size() > visibleCount
        ? "Showing " + std::to_string(firstVisible_ + 1) + "-" +
          std::to_string(std::min(firstVisible_ + visibleCount, profiles_.size())) + " of " +
          std::to_string(profiles_.size()) + " profiles"
        : std::to_string(profiles_.size()) + (profiles_.size() == 1 ? " profile" : " profiles"));
}

std::optional<persistence::Profile> ProfilesScene::selectedProfile() const
{
    if (selectedProfileIndex_ >= profiles_.size()) return std::nullopt;
    return profiles_[selectedProfileIndex_];
}

bool ProfilesScene::selectedIsActive() const
{
    const auto selected = selectedProfile();
    const auto active = profileService_.activeProfile();
    return selected.has_value() && active.has_value() && selected->id == active->id;
}
