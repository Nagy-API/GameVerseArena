#include "ProfilesScene.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

ProfilesScene::ProfilesScene(AppContext& context, persistence::ProfileService& profileService,
                             std::int64_t& selectedStatsProfileId)
    : context_(context), profileService_(profileService), selectedStatsProfileId_(selectedStatsProfileId),
      regularFont_(context.regularFont),
      kicker_(context.semiboldFont, "LOCAL PLAYERS", Theme::labelSize),
      title_(context.semiboldFont, "Player Profiles", Theme::pageTitleSize),
      subtitle_(context.regularFont, "Choose the local name used as Player 1's default in graphical games.", Theme::bodySize),
      help_(context.regularFont, "Up / Down selects a profile  |  Tab or Left / Right selects an action  |  Enter activates", 14),
      scrollStatus_(context.regularFont, "", 14),
      actionError_(context.regularFont, "", 14),
      actions_{UiButton(context.semiboldFont, "Create Profile", {318.f, 56.f}),
               UiButton(context.semiboldFont, "Rename", {318.f, 56.f}),
               UiButton(context.semiboldFont, "Delete", {318.f, 56.f}),
               UiButton(context.semiboldFont, "Set Active", {318.f, 56.f}),
               UiButton(context.semiboldFont, "View Stats", {318.f, 56.f}),
               UiButton(context.semiboldFont, "Back", {318.f, 56.f})},
      editOverlay_(context), deleteOverlay_(context)
{
    kicker_.setPosition({Theme::pageMargin, 42.f}); kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 70.f}); title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 125.f}); subtitle_.setFillColor(Theme::textSecondary);
    help_.setPosition({Theme::pageMargin, 680.f}); help_.setFillColor(Theme::textMuted);
    scrollStatus_.setPosition({72.f, 642.f}); scrollStatus_.setFillColor(Theme::textMuted);
    actionError_.setPosition({300.f, 642.f}); actionError_.setFillColor(Theme::danger);

    names_.reserve(visibleCount); badges_.reserve(visibleCount);
    for (std::size_t index = 0; index < visibleCount; ++index) {
        const float y = 180.f + static_cast<float>(index) * 74.f;
        cards_[index].setSize({760.f, 62.f}); cards_[index].setPosition({72.f, y});
        cards_[index].setFillColor(Theme::backgroundRaised); cards_[index].setOutlineThickness(1.f);
        names_.emplace_back(regularFont_, "", 21); names_.back().setPosition({96.f, y + 18.f});
        names_.back().setFillColor(Theme::textPrimary);
        badges_.emplace_back(context.semiboldFont, "", Theme::labelSize); badges_.back().setPosition({680.f, y + 21.f});
        badges_.back().setFillColor(Theme::secondary);
    }
    for (std::size_t index = 0; index < actionCount; ++index) {
        actions_[index].setPosition({890.f, 160.f + static_cast<float>(index) * 68.f});
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
                context_.play(audio::SoundId::UiConfirm);
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
            try {
                profileService_.deleteProfile(pendingProfileId_);
                actionError_.setString("");
                context_.play(audio::SoundId::UiConfirm);
            } catch (const std::exception& error) {
                std::cerr << "GameVerseArenaGUI: profile delete failed: " << error.what() << '\n';
                actionError_.setString("The profile could not be deleted; details were logged.");
                context_.play(audio::SoundId::UiError);
            }
            deleteOverlay_.close();
            reload();
        }
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) { activateAction(5); return; }
        else if (key->code == sf::Keyboard::Key::Up) moveProfileSelection(-1);
        else if (key->code == sf::Keyboard::Key::Down) moveProfileSelection(1);
        else if (key->code == sf::Keyboard::Key::Tab) moveActionSelection(key->shift ? -1 : 1);
        else if (key->code == sf::Keyboard::Key::Right) moveActionSelection(1);
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
                selectProfile(firstVisible_ + slot, true);
            }
        }
        for (std::size_t action = 0; action < actionCount; ++action) {
            const bool hovered = actions_[action].contains(point);
            actions_[action].setHovered(hovered);
            if (hovered) selectAction(action, true);
        }
        refresh();
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        for (std::size_t slot = 0; slot < visibleCount; ++slot) {
            if (firstVisible_ + slot < profiles_.size() && cards_[slot].getGlobalBounds().contains(point)) {
                selectProfile(firstVisible_ + slot, true); refresh(); return;
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
    for (auto& action : actions_) action.update(deltaTime, context_.reducedMotion());
}

void ProfilesScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_); window.draw(title_); window.draw(subtitle_);
    for (std::size_t slot = 0; slot < visibleCount && firstVisible_ + slot < profiles_.size(); ++slot) {
        window.draw(cards_[slot]); window.draw(names_[slot]); window.draw(badges_[slot]);
    }
    for (const auto& action : actions_) action.draw(window);
    window.draw(scrollStatus_); window.draw(actionError_); window.draw(help_);
    editOverlay_.render(window); deleteOverlay_.render(window);
}

void ProfilesScene::onResize(sf::Vector2u) {}

void ProfilesScene::onActivate()
{
    selectedAction_ = 3;
    actionError_.setString("");
    for (auto& action : actions_) action.setHovered(false);
    reload();
}

void ProfilesScene::selectAction(std::size_t action, bool withSound)
{
    if (withSound && action != selectedAction_) context_.play(audio::SoundId::UiFocus);
    selectedAction_ = action;
}

void ProfilesScene::selectProfile(std::size_t index, bool withSound)
{
    if (withSound && index != selectedProfileIndex_) context_.play(audio::SoundId::UiFocus);
    selectedProfileIndex_ = index;
}

void ProfilesScene::moveProfileSelection(int offset)
{
    if (profiles_.empty()) return;
    const int next = std::clamp(static_cast<int>(selectedProfileIndex_) + offset, 0,
                                static_cast<int>(profiles_.size()) - 1);
    selectProfile(static_cast<std::size_t>(next), true);
    if (selectedProfileIndex_ < firstVisible_) firstVisible_ = selectedProfileIndex_;
    if (selectedProfileIndex_ >= firstVisible_ + visibleCount) firstVisible_ = selectedProfileIndex_ - visibleCount + 1;
    refresh();
}

void ProfilesScene::moveActionSelection(int offset)
{
    selectAction(static_cast<std::size_t>((static_cast<int>(selectedAction_) + offset +
                                           static_cast<int>(actionCount)) % static_cast<int>(actionCount)), true);
    refresh();
}

void ProfilesScene::activateAction(std::size_t action)
{
    const auto selected = selectedProfile();
    if (action == 0) {
        pendingProfileId_ = 0; editOverlay_.open(ProfileEditOverlay::Mode::Create);
        context_.play(audio::SoundId::UiConfirm);
    } else if (action == 1 && selected.has_value()) {
        pendingProfileId_ = selected->id; editOverlay_.open(ProfileEditOverlay::Mode::Rename, selected->displayName);
        context_.play(audio::SoundId::UiConfirm);
    } else if (action == 2 && selected.has_value()) {
        pendingProfileId_ = selected->id; deleteOverlay_.open(selected->displayName, selectedIsActive());
        context_.play(audio::SoundId::UiConfirm);
    } else if (action == 3 && selected.has_value()) {
        try {
            profileService_.setActiveProfile(selected->id);
            actionError_.setString("");
            context_.play(audio::SoundId::UiConfirm);
        } catch (const std::exception& error) {
            std::cerr << "GameVerseArenaGUI: set active profile failed: " << error.what() << '\n';
            actionError_.setString("The active profile could not be changed; details were logged.");
            context_.play(audio::SoundId::UiError);
        }
        reload(selected->id);
    } else if (action == 4 && selected.has_value()) {
        selectedStatsProfileId_ = selected->id;
        context_.play(audio::SoundId::UiConfirm);
        context_.scenes.switchTo(SceneId::ProfileStats);
    } else if (action == 5) {
        context_.play(audio::SoundId::UiBack);
        context_.scenes.switchTo(SceneId::MainMenu);
    } else {
        context_.play(audio::SoundId::UiError);
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
