#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

namespace Theme {

inline constexpr sf::Vector2f logicalSize{1280.f, 720.f};

inline const sf::Color background{11, 17, 30};
inline const sf::Color backgroundRaised{17, 26, 43};
inline const sf::Color panel{24, 35, 56};
inline const sf::Color panelHover{34, 49, 75};
inline const sf::Color primary{74, 144, 245};
inline const sf::Color primaryBright{103, 166, 255};
inline const sf::Color secondary{80, 211, 181};
inline const sf::Color textPrimary{242, 246, 255};
inline const sf::Color textSecondary{162, 177, 201};
inline const sf::Color textMuted{112, 128, 154};
inline const sf::Color border{52, 69, 96};
inline const sf::Color warning{245, 184, 82};
inline const sf::Color danger{242, 112, 128};
inline const sf::Color overlay{5, 9, 18, 220};
inline const sf::Color markX{103, 166, 255};
inline const sf::Color markO{80, 211, 181};
inline const sf::Color winningCell{42, 83, 88};
inline const sf::Color arcadeLeft{103, 166, 255};
inline const sf::Color arcadeRight{80, 211, 181};
inline const sf::Color playfield{13, 23, 39};
inline const sf::Color divider{69, 87, 116, 150};
inline const sf::Color ballGlow{235, 245, 255, 54};

inline constexpr float pageMargin = 72.f;
inline constexpr float buttonWidth = 360.f;
inline constexpr float buttonHeight = 64.f;
inline constexpr float buttonGap = 14.f;
inline constexpr float cardRadius = 18.f;
inline constexpr float animationSpeed = 11.f;
inline constexpr float boardSize = 390.f;
inline constexpr float aiThinkingDelay = 0.35f;

inline constexpr unsigned int titleSize = 58;
inline constexpr unsigned int pageTitleSize = 42;
inline constexpr unsigned int subtitleSize = 20;
inline constexpr unsigned int bodySize = 18;
inline constexpr unsigned int buttonTextSize = 20;
inline constexpr unsigned int labelSize = 15;

} // namespace Theme
