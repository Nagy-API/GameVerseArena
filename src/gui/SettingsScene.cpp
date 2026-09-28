#include "SettingsScene.hpp"

#include "Theme.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <cmath>

using audio::SoundId;

namespace {
constexpr float rowX = 72.f;
constexpr float rowWidth = 760.f;
constexpr float rowHeight = 58.f;
constexpr float firstRowY = 176.f;
constexpr float rowStep = 68.f;
constexpr float trackX = rowX + 300.f;
constexpr float trackWidth = 330.f;

constexpr std::array<const char*, 6> rowLabels{
    "MASTER VOLUME", "UI VOLUME", "GAMEPLAY VOLUME", "ACHIEVEMENT VOLUME", "MUTE ALL", "REDUCED MOTION"};

SoundId previewFor(std::size_t slider, const persistence::AppSettings& settings)
{
    if (slider == 1) return SoundId::UiFocus;
    if (slider == 2) return SoundId::MovePrimary;
    if (slider == 3) return SoundId::AchievementUnlocked;
    // Master Volume scales every category: preview it with a category that is not at 0%.
    if (settings.uiVolume > 0) return SoundId::UiConfirm;
    if (settings.gameplayVolume > 0) return SoundId::MovePrimary;
    return SoundId::AchievementUnlocked;
}
} // namespace

SettingsScene::SettingsScene(AppContext& context)
    : context_(context),
      kicker_(context.semiboldFont, "SETTINGS", Theme::labelSize),
      title_(context.semiboldFont, "Audio & accessibility", Theme::pageTitleSize),
      subtitle_(context.regularFont, "Changes apply immediately and are saved for every profile on this computer.",
                Theme::bodySize),
      help_(context.regularFont,
            "Up / Down or Tab moves  |  Left / Right adjusts (Shift for 1%)  |  Enter toggles  |  Escape returns",
            Theme::labelSize),
      status_(context.regularFont, "", 16),
      sideTitle_(context.semiboldFont, "ABOUT THESE OPTIONS", Theme::labelSize),
      sideBody_(context.regularFont,
                "Every category is scaled by Master Volume.\n\n"
                "Mute All silences everything without\nchanging the volume levels.\n\n"
                "Reduced Motion turns off decorative\nmenu, card, and notification motion.\n"
                "Gameplay speed, physics, and timers\nare never changed.",
                16),
      labels_{sf::Text(context.semiboldFont, "", Theme::labelSize), sf::Text(context.semiboldFont, "", Theme::labelSize),
              sf::Text(context.semiboldFont, "", Theme::labelSize), sf::Text(context.semiboldFont, "", Theme::labelSize),
              sf::Text(context.semiboldFont, "", Theme::labelSize), sf::Text(context.semiboldFont, "", Theme::labelSize)},
      values_{sf::Text(context.semiboldFont, "", 20), sf::Text(context.semiboldFont, "", 20),
              sf::Text(context.semiboldFont, "", 20), sf::Text(context.semiboldFont, "", 20),
              sf::Text(context.semiboldFont, "", 20), sf::Text(context.semiboldFont, "", 20)},
      resetButton_(context.semiboldFont, "Reset to Defaults", {260.f, 54.f}),
      backButton_(context.semiboldFont, "Back", {170.f, 54.f})
{
    kicker_.setPosition({Theme::pageMargin, 42.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 70.f});
    title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 125.f});
    subtitle_.setFillColor(Theme::textSecondary);
    help_.setPosition({Theme::pageMargin, 684.f});
    help_.setFillColor(Theme::textMuted);
    status_.setPosition({Theme::pageMargin, 590.f});
    sideTitle_.setPosition({880.f, 190.f});
    sideTitle_.setFillColor(Theme::secondary);
    sideBody_.setPosition({880.f, 222.f});
    sideBody_.setFillColor(Theme::textSecondary);
    sideBody_.setLineSpacing(1.2f);

    for (std::size_t row = 0; row < rowCount; ++row) {
        const auto bounds = rowBounds(row);
        labels_[row].setString(rowLabels[row]);
        labels_[row].setFillColor(Theme::textMuted);
        labels_[row].setPosition({bounds.position.x + 22.f, bounds.position.y + 20.f});
        values_[row].setFillColor(Theme::textPrimary);
    }
    resetButton_.setPosition({Theme::pageMargin, 620.f});
    backButton_.setPosition({352.f, 620.f});
}

sf::FloatRect SettingsScene::rowBounds(std::size_t row) const
{
    return {{rowX, firstRowY + static_cast<float>(row) * rowStep}, {rowWidth, rowHeight}};
}

sf::FloatRect SettingsScene::trackBounds(std::size_t slider) const
{
    const auto row = rowBounds(slider);
    return {{trackX, row.position.y + row.size.y / 2.f - 12.f}, {trackWidth, 24.f}};
}

int SettingsScene::volumeAt(std::size_t slider) const
{
    auto copy = draft_;
    return volumeRef(copy, slider);
}

int& SettingsScene::volumeRef(persistence::AppSettings& settings, std::size_t slider) const
{
    if (slider == 0) return settings.masterVolume;
    if (slider == 1) return settings.uiVolume;
    if (slider == 2) return settings.gameplayVolume;
    return settings.achievementVolume;
}

std::optional<int> SettingsScene::valueAtTrack(std::size_t slider, float x) const
{
    if (slider >= sliderCount) return std::nullopt;
    const auto track = trackBounds(slider);
    const float ratio = std::clamp((x - track.position.x) / track.size.x, 0.f, 1.f);
    return static_cast<int>(std::lround(ratio * 100.f));
}

void SettingsScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (event.is<sf::Event::FocusLost>()) {
        finishDrag();
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (dragging_) finishDrag();
        switch (key->code) {
        case sf::Keyboard::Key::Escape: goBack(); return;
        case sf::Keyboard::Key::Up: moveSelection(-1); break;
        case sf::Keyboard::Key::Down: moveSelection(1); break;
        case sf::Keyboard::Key::Tab: moveSelection(key->shift ? -1 : 1); break;
        case sf::Keyboard::Key::Left: adjustSelected(-1, key->shift); break;
        case sf::Keyboard::Key::Right: adjustSelected(1, key->shift); break;
        case sf::Keyboard::Key::Home:
            if (selected_ < sliderCount) setVolume(selected_, 0, true);
            break;
        case sf::Keyboard::Key::End:
            if (selected_ < sliderCount) setVolume(selected_, 100, true);
            break;
        case sf::Keyboard::Key::Enter:
        case sf::Keyboard::Key::Space: activateSelected(); break;
        default: break;
        }
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        if (dragging_) {
            if (const auto value = valueAtTrack(*dragging_, point.x)) setVolume(*dragging_, *value, false);
            return;
        }
        resetButton_.setHovered(resetButton_.contains(point));
        backButton_.setHovered(backButton_.contains(point));
        for (std::size_t row = 0; row < rowCount; ++row) {
            if (rowBounds(row).contains(point)) select(row, true);
        }
        if (resetButton_.contains(point)) select(resetIndex, true);
        if (backButton_.contains(point)) select(backIndex, true);
    }

    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>();
        pressed && pressed->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(pressed->position);
        for (std::size_t slider = 0; slider < sliderCount; ++slider) {
            auto hit = trackBounds(slider);
            hit.position.x -= 12.f; hit.size.x += 24.f; hit.position.y -= 10.f; hit.size.y += 20.f;
            if (hit.contains(point)) {
                select(slider, false);
                dragging_ = slider;
                if (const auto value = valueAtTrack(slider, point.x)) setVolume(slider, *value, false);
                return;
            }
        }
    }

    if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>();
        released && released->button == sf::Mouse::Button::Left) {
        if (dragging_) {
            finishDrag();
            return;
        }
        const auto point = window.mapPixelToCoords(released->position);
        if (resetButton_.contains(point)) { select(resetIndex, false); resetDefaults(); return; }
        if (backButton_.contains(point)) { goBack(); return; }
        for (std::size_t row = sliderCount; row < rowCount; ++row) {
            if (rowBounds(row).contains(point)) { select(row, false); toggle(row); return; }
        }
    }
}

void SettingsScene::update(sf::Time deltaTime)
{
    resetButton_.update(deltaTime, context_.reducedMotion());
    backButton_.update(deltaTime, context_.reducedMotion());
}

void SettingsScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_);
    window.draw(title_);
    window.draw(subtitle_);

    for (std::size_t row = 0; row < rowCount; ++row) {
        const auto bounds = rowBounds(row);
        sf::RectangleShape card(bounds.size);
        card.setPosition(bounds.position);
        card.setFillColor(Theme::backgroundRaised);
        card.setOutlineThickness(row == selected_ ? 2.f : 1.f);
        card.setOutlineColor(row == selected_ ? Theme::primaryBright : Theme::border);
        window.draw(card);
        window.draw(labels_[row]);

        sf::Text value = values_[row];
        if (row < sliderCount) {
            const int volume = volumeAt(row);
            const auto track = trackBounds(row);
            sf::RectangleShape rail({track.size.x, 6.f});
            rail.setPosition({track.position.x, track.position.y + 9.f});
            rail.setFillColor(Theme::panelHover);
            window.draw(rail);
            sf::RectangleShape filled({track.size.x * static_cast<float>(volume) / 100.f, 6.f});
            filled.setPosition(rail.getPosition());
            filled.setFillColor(draft_.audioMuted ? Theme::textMuted : Theme::primary);
            window.draw(filled);
            sf::CircleShape knob(11.f);
            knob.setOrigin({11.f, 11.f});
            knob.setPosition({track.position.x + track.size.x * static_cast<float>(volume) / 100.f,
                              track.position.y + 12.f});
            knob.setFillColor(row == selected_ ? Theme::primaryBright : Theme::textSecondary);
            window.draw(knob);
            value.setString(std::to_string(volume) + "%" + (draft_.audioMuted ? " (muted)" : ""));
            value.setPosition({trackX + trackWidth + 26.f, bounds.position.y + 16.f});
        } else {
            const bool on = row == 4 ? draft_.audioMuted : draft_.reducedMotion;
            sf::RectangleShape pill({58.f, 28.f});
            pill.setPosition({trackX, bounds.position.y + 15.f});
            pill.setFillColor(on ? Theme::secondary : Theme::panelHover);
            pill.setOutlineThickness(1.f);
            pill.setOutlineColor(Theme::border);
            window.draw(pill);
            sf::CircleShape dot(11.f);
            dot.setPosition({on ? trackX + 33.f : trackX + 3.f, bounds.position.y + 18.f});
            dot.setFillColor(Theme::textPrimary);
            window.draw(dot);
            value.setString(on ? "On" : "Off");
            value.setPosition({trackX + 76.f, bounds.position.y + 16.f});
        }
        window.draw(value);
    }

    window.draw(status_);
    window.draw(sideTitle_);
    window.draw(sideBody_);
    resetButton_.draw(window);
    backButton_.draw(window);
    window.draw(help_);
}

void SettingsScene::onResize(sf::Vector2u)
{
}

void SettingsScene::onActivate()
{
    dragging_.reset();
    draft_ = context_.settings.current();
    resetButton_.setHovered(false);
    backButton_.setHovered(false);
    select(0, false);
    refreshStatus();
}

void SettingsScene::select(std::size_t index, bool withSound)
{
    if (index >= focusCount) return;
    if (withSound && index != selected_) context_.play(SoundId::UiFocus);
    selected_ = index;
    resetButton_.setSelected(selected_ == resetIndex);
    backButton_.setSelected(selected_ == backIndex);
}

void SettingsScene::moveSelection(int offset)
{
    const int count = static_cast<int>(focusCount);
    select(static_cast<std::size_t>((static_cast<int>(selected_) + offset + count) % count), true);
}

void SettingsScene::activateSelected()
{
    if (selected_ < sliderCount) {
        context_.play(previewFor(selected_, draft_));
    } else if (selected_ < rowCount) {
        toggle(selected_);
    } else if (selected_ == resetIndex) {
        resetDefaults();
    } else {
        goBack();
    }
}

void SettingsScene::adjustSelected(int direction, bool fine)
{
    if (selected_ < sliderCount) {
        const int step = fine ? 1 : 5;
        const int current = volumeAt(selected_);
        int next = current + direction * step;
        if (!fine) next = direction > 0 ? (current / 5 + 1) * 5 : ((current + 4) / 5 - 1) * 5;
        setVolume(selected_, std::clamp(next, 0, 100), true);
    } else if (selected_ < rowCount) {
        toggle(selected_, direction > 0);
    } else {
        moveSelection(direction);
    }
}

void SettingsScene::setVolume(std::size_t slider, int value, bool persist)
{
    auto next = draft_;
    auto& target = volumeRef(next, slider);
    const int clamped = std::clamp(value, 0, 100);
    if (target == clamped) return;
    target = clamped;
    commit(next, persist);
    if (persist) context_.play(previewFor(slider, draft_));
}

void SettingsScene::toggle(std::size_t row, std::optional<bool> forced)
{
    auto next = draft_;
    bool& flag = row == 4 ? next.audioMuted : next.reducedMotion;
    const bool value = forced.value_or(!flag);
    if (flag == value) return;
    flag = value;
    commit(next, true);
    context_.play(SoundId::UiConfirm);
}

void SettingsScene::resetDefaults()
{
    context_.settings.resetToDefaults();
    draft_ = context_.settings.current();
    refreshStatus();
    context_.play(SoundId::UiConfirm);
}

void SettingsScene::goBack()
{
    finishDrag();
    context_.play(SoundId::UiBack);
    context_.scenes.switchTo(SceneId::MainMenu);
}

void SettingsScene::finishDrag()
{
    if (!dragging_) return;
    const auto slider = *dragging_;
    dragging_.reset();
    commit(draft_, true);
    context_.play(previewFor(slider, draft_));
}

void SettingsScene::commit(const persistence::AppSettings& settings, bool persist)
{
    draft_ = settings;
    if (persist) {
        context_.settings.apply(draft_);
    } else {
        // Live preview while dragging: audible immediately, saved when the drag ends.
        context_.audio.setMix(SettingsController::mixFor(draft_));
    }
    refreshStatus();
}

void SettingsScene::refreshStatus()
{
    if (!context_.settings.lastError().empty()) {
        status_.setString(context_.settings.lastError());
        status_.setFillColor(Theme::danger);
    } else if (!context_.settings.loadIssues().empty()) {
        status_.setString("Some stored settings were invalid and were replaced by defaults.");
        status_.setFillColor(Theme::warning);
    } else if (!context_.audio.available()) {
        status_.setString(context_.audio.status() + " Settings are still saved.");
        status_.setFillColor(Theme::warning);
    } else {
        status_.setString("All changes are saved automatically.");
        status_.setFillColor(Theme::textMuted);
    }
}
