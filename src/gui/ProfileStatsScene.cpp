#include "ProfileStatsScene.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <ctime>

namespace {
std::string rate(double value) { std::ostringstream out; out << std::fixed << std::setprecision(1) << value * 100.0 << '%'; return out.str(); }
std::string duration(std::int64_t ms)
{ const auto minutes=ms/60000; const auto hours=minutes/60; return hours>0 ? std::to_string(hours)+"h "+std::to_string(minutes%60)+"m" : std::to_string(minutes)+"m"; }
std::string played(const std::optional<std::int64_t>& value)
{
    if (!value) return "Never";
    std::time_t raw=static_cast<std::time_t>(*value/1000); std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc,&raw);
#else
    gmtime_r(&raw,&utc);
#endif
    std::ostringstream out; out<<std::put_time(&utc,"%Y-%m-%d %H:%M UTC"); return out.str();
}
}

ProfileStatsScene::ProfileStatsScene(AppContext& context,
    persistence::ProfileService& profiles, persistence::StatisticsRepository& statistics,
    persistence::AchievementService& achievements,
    std::int64_t& selectedProfileId)
    : context_(context),scenes_(context.scenes),profiles_(profiles),statistics_(statistics),achievements_(achievements),selectedProfileId_(selectedProfileId),regular_(context.regularFont),
      kicker_(context.semiboldFont,"PLAYER PROFILE",Theme::labelSize),title_(context.semiboldFont,"Statistics",Theme::pageTitleSize),
      subtitle_(context.regularFont,"",Theme::bodySize),empty_(context.regularFont,"",22),achievementSummary_(context.semiboldFont,"",18),
      cards_{sf::Text(context.regularFont,"",18),sf::Text(context.regularFont,"",18),sf::Text(context.regularFont,"",18)},
      buttons_{UiButton(context.semiboldFont,"Recent Matches",{220.f,54.f}),UiButton(context.semiboldFont,"Achievements",{220.f,54.f}),UiButton(context.semiboldFont,"Back",{170.f,54.f})}
{
    kicker_.setPosition({72.f,42.f}); kicker_.setFillColor(Theme::secondary); title_.setPosition({72.f,70.f}); title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({72.f,125.f}); subtitle_.setFillColor(Theme::textSecondary); empty_.setPosition({72.f,260.f}); empty_.setFillColor(Theme::textSecondary);
    cards_[0].setPosition({72.f,185.f}); cards_[1].setPosition({452.f,185.f}); cards_[2].setPosition({832.f,185.f});
    for(auto& card:cards_){card.setFillColor(Theme::textPrimary);card.setLineSpacing(1.35f);}
    achievementSummary_.setPosition({72.f,590.f}); achievementSummary_.setFillColor(Theme::warning);
    buttons_[0].setPosition({746.f,635.f}); buttons_[1].setPosition({976.f,635.f}); buttons_[2].setPosition({72.f,635.f}); select(0);
}
void ProfileStatsScene::onActivate()
{
    const auto list=profiles_.listProfiles();
    auto found=std::find_if(list.begin(),list.end(),[this](const auto& p){return p.id==selectedProfileId_;});
    if(found==list.end()){const auto active=profiles_.activeProfile();if(!active){scenes_.switchTo(SceneId::Profiles);return;} selectedProfileId_=active->id;found=std::find_if(list.begin(),list.end(),[this](const auto& p){return p.id==selectedProfileId_;});}
    const auto active=profiles_.activeProfile(); subtitle_.setString(found->displayName+(active&&active->id==found->id?"  |  ACTIVE":""));
    persistence::OverallStatistics overall; persistence::GameStatistics ttt; persistence::GameStatistics pong;
    try {
        overall=statistics_.overall(selectedProfileId_); ttt=statistics_.forGame(selectedProfileId_,persistence::GameKey::ClassicTicTacToe);
        pong=statistics_.forGame(selectedProfileId_,persistence::GameKey::PingPong);
        achievementSummary_.setString("Achievements: "+std::to_string(achievements_.unlockedCount(selectedProfileId_))+" / 12");
    } catch (const std::exception& error) {
        std::cerr << "GameVerseArenaGUI: statistics could not be loaded: " << error.what() << '\n';
        for (auto& card : cards_) card.setString("");
        achievementSummary_.setString("");
        empty_.setString("Statistics could not be loaded.\nOther profile data is unaffected; details were logged.");
        select(0);
        return;
    }
    empty_.setString(overall.matches==0?"No matches yet.\nPlay any game from the library to build your history.":"");
    cards_[0].setString("OVERALL\n\nMatches  "+std::to_string(overall.matches)+"\nWins / Losses / Draws  "+std::to_string(overall.wins)+" / "+std::to_string(overall.losses)+" / "+std::to_string(overall.draws)+"\nWin rate  "+rate(overall.winRate)+"\nPlay time  "+duration(overall.totalDurationMs)+"\nCurrent / best streak  "+std::to_string(overall.currentWinStreak)+" / "+std::to_string(overall.bestWinStreak)+"\nLast played  "+played(overall.lastPlayedAt));
    cards_[1].setString("CLASSIC TIC-TAC-TOE\n\nMatches  "+std::to_string(ttt.matches)+"\nW / L / D  "+std::to_string(ttt.wins)+" / "+std::to_string(ttt.losses)+" / "+std::to_string(ttt.draws)+"\nWin rate  "+rate(ttt.winRate)+"\nPlay time  "+duration(ttt.totalDurationMs)+"\nAs X / O  "+std::to_string(ttt.ticTacToeAsX)+" / "+std::to_string(ttt.ticTacToeAsO)+"\nSingle / BO3 / BO5  "+std::to_string(ttt.singleMatches)+" / "+std::to_string(ttt.bestOfThreeMatches)+" / "+std::to_string(ttt.bestOfFiveMatches));
    cards_[2].setString("PING PONG\n\nMatches  "+std::to_string(pong.matches)+"\nWins / Losses  "+std::to_string(pong.wins)+" / "+std::to_string(pong.losses)+"\nWin rate  "+rate(pong.winRate)+"\nPlay time  "+duration(pong.totalDurationMs)+"\nPoints for / against  "+std::to_string(pong.pointsScored)+" / "+std::to_string(pong.pointsConceded)+"\nBest final margin  "+std::to_string(pong.bestFinalMargin));
    if (overall.matches == 0) for (auto& card : cards_) card.setString("");
    select(0);
}
void ProfileStatsScene::select(std::size_t index, bool withSound){if(withSound&&index!=selected_)context_.play(audio::SoundId::UiFocus);selected_=index;for(std::size_t i=0;i<buttons_.size();++i)buttons_[i].setSelected(i==selected_);}
void ProfileStatsScene::activate(std::size_t index){context_.play(index==2?audio::SoundId::UiBack:audio::SoundId::UiConfirm);if(index==0)scenes_.switchTo(SceneId::MatchHistory);else if(index==1)scenes_.switchTo(SceneId::ProfileAchievements);else scenes_.switchTo(SceneId::Profiles);}
void ProfileStatsScene::handleEvent(const sf::Event& e,sf::RenderWindow&w){if(const auto*k=e.getIf<sf::Event::KeyPressed>()){if(k->code==sf::Keyboard::Key::Escape)activate(2);else if(k->code==sf::Keyboard::Key::Right||(k->code==sf::Keyboard::Key::Tab&&!k->shift))select((selected_+1)%buttons_.size(),true);else if(k->code==sf::Keyboard::Key::Left||k->code==sf::Keyboard::Key::Tab)select((selected_+buttons_.size()-1)%buttons_.size(),true);else if(k->code==sf::Keyboard::Key::Enter||k->code==sf::Keyboard::Key::Space)activate(selected_);}if(const auto*m=e.getIf<sf::Event::MouseMoved>()){const auto p=w.mapPixelToCoords(m->position);for(std::size_t i=0;i<buttons_.size();++i){buttons_[i].setHovered(buttons_[i].contains(p));if(buttons_[i].contains(p))select(i,true);}}if(const auto*c=e.getIf<sf::Event::MouseButtonReleased>();c&&c->button==sf::Mouse::Button::Left){const auto p=w.mapPixelToCoords(c->position);for(std::size_t i=0;i<buttons_.size();++i)if(buttons_[i].contains(p))activate(i);}}
void ProfileStatsScene::update(sf::Time dt){for(auto& b:buttons_)b.update(dt,context_.reducedMotion());}
void ProfileStatsScene::render(sf::RenderWindow&w)const{w.draw(kicker_);w.draw(title_);w.draw(subtitle_);for(const auto&c:cards_)w.draw(c);w.draw(empty_);w.draw(achievementSummary_);for(const auto&b:buttons_)b.draw(w);}
