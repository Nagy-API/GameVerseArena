#include "MatchHistoryScene.hpp"
#include "Theme.hpp"
#include "Utf8Text.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
namespace {
std::string date(std::int64_t ms){std::time_t raw=static_cast<std::time_t>(ms/1000);std::tm utc{};
#ifdef _WIN32
gmtime_s(&utc,&raw);
#else
gmtime_r(&raw,&utc);
#endif
std::ostringstream out;out<<std::put_time(&utc,"%Y-%m-%d %H:%M UTC");return out.str();}
std::string game(persistence::GameKey v){return v==persistence::GameKey::ClassicTicTacToe?"Tic-Tac-Toe":"Ping Pong";}
std::string result(persistence::MatchResult v){return v==persistence::MatchResult::Win?"WIN":v==persistence::MatchResult::Loss?"LOSS":"DRAW";}
std::string shortName(const std::string& value){constexpr std::size_t limit=14;return utf8_text::length(value)<=limit?value:utf8_text::tail(value,limit-3)+"...";}
}
MatchHistoryScene::MatchHistoryScene(const sf::Font& r,const sf::Font& s,SceneManager& scenes,persistence::ProfileService& profiles,persistence::MatchRepository& matches,std::int64_t& id)
:scenes_(scenes),profiles_(profiles),matches_(matches),profileId_(id),regular_(r),title_(s,"Match History",Theme::pageTitleSize),subtitle_(r,"",Theme::bodySize),status_(r,"",14),empty_(r,"",20),rows_{},buttons_{UiButton(s,"Game: All",{210.f,48.f}),UiButton(s,"Result: All",{210.f,48.f}),UiButton(s,"Previous",{160.f,48.f}),UiButton(s,"Next",{160.f,48.f}),UiButton(s,"Back",{140.f,48.f})}
{title_.setPosition({72.f,45.f});title_.setFillColor(Theme::textPrimary);subtitle_.setPosition({72.f,100.f});subtitle_.setFillColor(Theme::textSecondary);status_.setPosition({72.f,655.f});status_.setFillColor(Theme::textMuted);empty_.setPosition({72.f,280.f});empty_.setFillColor(Theme::textSecondary);buttons_[0].setPosition({72.f,135.f});buttons_[1].setPosition({300.f,135.f});buttons_[2].setPosition({680.f,640.f});buttons_[3].setPosition({855.f,640.f});buttons_[4].setPosition({1068.f,640.f});for(std::size_t i=0;i<pageSize;++i){rows_.emplace_back(r,"",16);rows_.back().setPosition({72.f,205.f+static_cast<float>(i)*52.f});rows_.back().setFillColor(Theme::textPrimary);}select(0);}
void MatchHistoryScene::onActivate(){page_=0;reload();}
void MatchHistoryScene::reload()
{
    filter_.game = gameFilter_ == 1 ? std::optional{persistence::GameKey::ClassicTicTacToe}
        : gameFilter_ == 2 ? std::optional{persistence::GameKey::PingPong} : std::nullopt;
    filter_.result = resultFilter_ == 1 ? std::optional{persistence::MatchResult::Win}
        : resultFilter_ == 2 ? std::optional{persistence::MatchResult::Loss}
        : resultFilter_ == 3 ? std::optional{persistence::MatchResult::Draw} : std::nullopt;
    total_ = matches_.count(profileId_, filter_);
    if (page_ * pageSize >= static_cast<std::size_t>(total_) && page_ > 0) --page_;
    const auto data = matches_.recent(profileId_, filter_, pageSize, page_ * pageSize);
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (i >= data.size()) { rows_[i].setString(""); continue; }
        const auto& match = data[i];
        const std::string score = match.profileScore && match.opponentScore
            ? std::to_string(*match.profileScore) + "-" + std::to_string(*match.opponentScore) : "-";
        const std::string mode = match.mode == persistence::MatchMode::HumanVsComputer
            ? "vs Computer / " + std::string(persistence::toStorage(match.difficulty)) : "vs Human";
        rows_[i].setString(result(match.result) + "  |  " + game(match.game) + "  |  " +
            shortName(match.profileDisplayName) + " vs " + shortName(match.opponentName) + "  |  " + score +
            "  |  " + mode + "  |  " + std::to_string(match.durationMs / 1000) + "s  |  " + date(match.completedAt));
    }
    empty_.setString(total_ == 0 ? "No matches match these filters.\nPlay Tic-Tac-Toe or Ping Pong to build your history." : "");
    refresh();
}
void MatchHistoryScene::refresh(){static const std::array<const char*,3> games{"Game: All","Game: Tic-Tac-Toe","Game: Ping Pong"};static const std::array<const char*,4> results{"Result: All","Result: Win","Result: Loss","Result: Draw"};buttons_[0].setText(games[gameFilter_]);buttons_[1].setText(results[resultFilter_]);const auto list=profiles_.listProfiles();const auto found=std::find_if(list.begin(),list.end(),[this](const auto&p){return p.id==profileId_;});subtitle_.setString(found==list.end()?"Selected profile":found->displayName);const std::size_t pages=total_==0?1:(static_cast<std::size_t>(total_)+pageSize-1)/pageSize;status_.setString("Page "+std::to_string(page_+1)+" of "+std::to_string(pages)+"  |  "+std::to_string(total_)+" matches");for(std::size_t i=0;i<buttons_.size();++i)buttons_[i].setSelected(i==selected_);}
void MatchHistoryScene::select(std::size_t i){selected_=i;refresh();}
void MatchHistoryScene::activate(std::size_t i){if(i==0){gameFilter_=(gameFilter_+1)%3;page_=0;reload();}else if(i==1){resultFilter_=(resultFilter_+1)%4;page_=0;reload();}else if(i==2&&page_>0){--page_;reload();}else if(i==3&&(page_+1)*pageSize<static_cast<std::size_t>(total_)){++page_;reload();}else if(i==4)scenes_.switchTo(SceneId::ProfileStats);}
void MatchHistoryScene::handleEvent(const sf::Event&e,sf::RenderWindow&w){if(const auto*k=e.getIf<sf::Event::KeyPressed>()){if(k->code==sf::Keyboard::Key::Escape)activate(4);else if(k->code==sf::Keyboard::Key::Tab||k->code==sf::Keyboard::Key::Right)select((selected_+1)%buttons_.size());else if(k->code==sf::Keyboard::Key::Left)select((selected_+buttons_.size()-1)%buttons_.size());else if(k->code==sf::Keyboard::Key::Enter||k->code==sf::Keyboard::Key::Space)activate(selected_);}if(const auto*m=e.getIf<sf::Event::MouseMoved>()){const auto p=w.mapPixelToCoords(m->position);for(std::size_t i=0;i<buttons_.size();++i){buttons_[i].setHovered(buttons_[i].contains(p));if(buttons_[i].contains(p))select(i);}}if(const auto*c=e.getIf<sf::Event::MouseButtonReleased>();c&&c->button==sf::Mouse::Button::Left){const auto p=w.mapPixelToCoords(c->position);for(std::size_t i=0;i<buttons_.size();++i)if(buttons_[i].contains(p))activate(i);}}
void MatchHistoryScene::update(sf::Time dt){for(auto&b:buttons_)b.update(dt);}void MatchHistoryScene::render(sf::RenderWindow&w)const{w.draw(title_);w.draw(subtitle_);for(const auto&r:rows_)w.draw(r);w.draw(empty_);w.draw(status_);for(const auto&b:buttons_)b.draw(w);}
