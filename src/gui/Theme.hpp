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

inline constexpr float pageMargin = 72.f;
inline constexpr float buttonWidth = 360.f;
inline constexpr float buttonHeight = 64.f;
inline constexpr float buttonGap = 14.f;
inline constexpr float cardRadius = 18.f;
inline constexpr float animationSpeed = 11.f;

inline constexpr unsigned int titleSize = 58;
inline constexpr unsigned int pageTitleSize = 42;
inline constexpr unsigned int subtitleSize = 20;
inline constexpr unsigned int bodySize = 18;
inline constexpr unsigned int buttonTextSize = 20;
inline constexpr unsigned int labelSize = 15;

} // namespace Theme
