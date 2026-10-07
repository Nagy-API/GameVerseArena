#include "Schema.hpp"
#include "TestSchemaSupport.hpp"
#include "Database.hpp"
#include "MatchRepository.hpp"
#include "MatchRecorder.hpp"
#include "MatchService.hpp"
#include "ProfileService.hpp"
#include "StatisticsRepository.hpp"
#include "TicTacToeSession.hpp"
#include "PingPongSession.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
int failures = 0;
int checks = 0;

void check(bool condition, const std::string& message)
{
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}

struct TemporaryDirectory {
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() / ("GameVerseArena-match-tests-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
    }
    ~TemporaryDirectory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
    std::filesystem::path path;
};

persistence::CompletedMatch match(std::int64_t profileId, persistence::GameKey game,
                                  persistence::MatchResult result, std::int64_t completedAt)
{
    persistence::CompletedMatch value;
    value.profileId = profileId;
    value.game = game;
    value.mode = persistence::MatchMode::HumanVsHuman;
    value.opponentName = "Guest";
    value.profileDisplayName = "Owner";
    value.profileSideOrMark = game == persistence::GameKey::ClassicTicTacToe ? "X" : "Left";
    value.opponentSideOrMark = game == persistence::GameKey::ClassicTicTacToe ? "O" : "Right";
    value.result = result;
    if (game == persistence::GameKey::ClassicTicTacToe) {
        value.profileScore = result == persistence::MatchResult::Win ? 1 : 0;
        value.opponentScore = result == persistence::MatchResult::Loss ? 1 : 0;
    } else {
        value.profileScore = result == persistence::MatchResult::Win ? 5 : 2;
        value.opponentScore = result == persistence::MatchResult::Loss ? 5 : 1;
    }
    value.drawValue = game == persistence::GameKey::ClassicTicTacToe ? std::optional<int>{result == persistence::MatchResult::Draw ? 1 : 0} : std::nullopt;
    value.difficulty = persistence::DifficultyKey::None;
    value.matchFormat = game == persistence::GameKey::ClassicTicTacToe ? "single" : "first_to_5";
    value.durationMs = 1000 + completedAt;
    value.startedAt = completedAt - 100;
    value.completedAt = completedAt;
    return value;
}

void testMigration(const std::filesystem::path& directory)
{
    const auto path = directory / "migration.db";
    {
        persistence::Database database(path);
        check(database.userVersion() == persistence::schema::currentVersion, "new database opens at the current schema");
        persistence::ProfileService profiles(database); profiles.bootstrap();
        profiles.createProfile("Survivor");
    }
    {
        persistence::Database database(path);
        persistence::ProfileService profiles(database);
        check(database.userVersion() == persistence::schema::currentVersion && profiles.listProfiles().size() == 2,
              "current-schema reopen is idempotent and profiles survive");
        check(profiles.activeProfile().has_value(), "active profile survives schema reopen");
    }

    const auto v1Path = directory / "v1.db";
    std::int64_t legacyActiveId = 0;
    {
        persistence::Database database(v1Path);
        persistence::ProfileService profiles(database); profiles.bootstrap();
        profiles.createProfile("Legacy One");
        const auto legacyActive = profiles.createProfile("Legacy Active");
        profiles.setActiveProfile(legacyActive.id); legacyActiveId = legacyActive.id;
        test_support::downgradeToVersion(database, 1);
    }
    {
        persistence::Database migrated(v1Path);
        check(migrated.userVersion() == persistence::schema::currentVersion,
              "an existing v1 database migrates through every version to the current schema");
        auto table = migrated.prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='matches';");
        check(table.step() && table.integer(0) == 1, "v1 to v2 migration creates matches table");
        persistence::ProfileService profiles(migrated);
        check(profiles.listProfiles().size() == 3, "all existing profiles survive v1 to v2 migration");
        check(profiles.activeProfile().has_value() && profiles.activeProfile()->id == legacyActiveId,
              "active profile identity survives v1 to v2 migration");
    }

    const auto futurePath = directory / "future.db";
    const int future = persistence::schema::currentVersion + 1;
    { persistence::Database database(futurePath);
      database.execute(("PRAGMA user_version = " + std::to_string(future) + ";").c_str()); }
    try { persistence::Database unsupported(futurePath); check(false, "future schema must be rejected"); }
    catch (const std::runtime_error& error) {
        check(std::string(error.what()).find("version " + std::to_string(future)) != std::string::npos,
              "future-version rejection identifies the unsupported version");
    }
}

void testInsertHistoryAndStats(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "history.db");
    persistence::ProfileService profiles(database); profiles.bootstrap();
    const auto owner = profiles.activeProfile().value();
    const auto other = profiles.createProfile("Other");
    const auto empty = profiles.createProfile("Empty");
    persistence::MatchService service(database);
    persistence::MatchRepository history(database);
    persistence::StatisticsRepository statistics(database);
    const auto initiallyEmpty = statistics.overall(empty.id);
    check(initiallyEmpty.matches == 0 && initiallyEmpty.wins == 0 && initiallyEmpty.losses == 0 &&
          initiallyEmpty.draws == 0 && initiallyEmpty.winRate == 0.0 && !initiallyEmpty.lastPlayedAt,
          "zero-match profile has stable zero statistics without dividing by zero");

    auto sensitive = match(owner.id, persistence::GameKey::ClassicTicTacToe, persistence::MatchResult::Win, 100);
    sensitive.opponentName = "Robert'); DROP TABLE matches;--";
    service.recordCompleted(sensitive);
    service.recordCompleted(match(owner.id, persistence::GameKey::ClassicTicTacToe, persistence::MatchResult::Win, 200));
    service.recordCompleted(match(owner.id, persistence::GameKey::PingPong, persistence::MatchResult::Loss, 300));
    service.recordCompleted(match(owner.id, persistence::GameKey::ClassicTicTacToe, persistence::MatchResult::Draw, 400));
    service.recordCompleted(match(owner.id, persistence::GameKey::PingPong, persistence::MatchResult::Win, 500));
    service.recordCompleted(match(other.id, persistence::GameKey::PingPong, persistence::MatchResult::Win, 600));

    const auto page = history.recent(owner.id, {}, 2, 0);
    check(page.size() == 2 && page[0].completedAt == 500 && page[1].completedAt == 400, "history is newest-first and paginated");
    check(history.count(owner.id, {}) == 5, "All filter counts only the requested profile");
    persistence::MatchFilter gameFilter; gameFilter.game = persistence::GameKey::PingPong;
    check(history.count(owner.id, gameFilter) == 2, "game filter runs in repository query");
    persistence::MatchFilter resultFilter; resultFilter.result = persistence::MatchResult::Win;
    check(history.count(owner.id, resultFilter) == 3, "result filter runs in repository query");
    gameFilter.result = persistence::MatchResult::Loss;
    check(history.count(owner.id, gameFilter) == 1, "combined game/result filter works");
    check(history.recent(owner.id, {}, 20, 4).size() == 1, "history offset returns the final page");
    check(history.recent(owner.id, {}, 20, 0).back().opponentName == sensitive.opponentName,
          "SQL-sensitive opponent text is stored safely as data");

    const auto overall = statistics.overall(owner.id);
    check(overall.matches == 5 && overall.wins == 3 && overall.losses == 1 && overall.draws == 1,
          "overall totals derive from match rows");
    check(std::abs(overall.winRate - 0.6) < 0.0001, "win rate includes draws in denominator");
    check(overall.totalDurationMs == 6500 && overall.lastPlayedAt.has_value() && *overall.lastPlayedAt == 500,
          "duration and last-played derive from history");
    check(overall.currentWinStreak == 1 && overall.bestWinStreak == 2,
          "draw/loss break streaks and best streak scans complete history");

    const auto ttt = statistics.forGame(owner.id, persistence::GameKey::ClassicTicTacToe);
    check(ttt.matches == 3 && ttt.wins == 2 && ttt.draws == 1 && ttt.ticTacToeAsX == 3 && ttt.singleMatches == 3,
          "Tic-Tac-Toe aggregate and format/mark metrics are isolated");
    const auto pong = statistics.forGame(owner.id, persistence::GameKey::PingPong);
    check(pong.matches == 2 && pong.pointsScored == 7 && pong.pointsConceded == 6 && pong.bestFinalMargin == 4,
          "Ping Pong points and best margin derive correctly");
    const auto zero = statistics.overall(other.id);
    check(zero.matches == 1 && zero.winRate == 1.0, "profile isolation excludes owner history");

    check(profiles.deleteProfile(other.id), "profile with history can be deleted");
    check(history.count(other.id, {}) == 0, "profile deletion cascades match history");
}

void testValidation(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "validation.db");
    persistence::ProfileService profiles(database); profiles.bootstrap();
    persistence::MatchService service(database);
    const auto id = profiles.activeProfile()->id;
    auto invalid = match(id, persistence::GameKey::PingPong, persistence::MatchResult::Win, 10);
    invalid.durationMs = -1;
    try { service.recordCompleted(invalid); check(false, "negative duration must be rejected"); }
    catch (const std::invalid_argument&) { check(true, "negative duration is rejected"); }
    invalid.durationMs = 10; invalid.opponentName.clear();
    try { service.recordCompleted(invalid); check(false, "incomplete match must be rejected"); }
    catch (const std::invalid_argument&) { check(true, "incomplete match is rejected"); }
    invalid = match(id, persistence::GameKey::PingPong, persistence::MatchResult::Win, 20);
    invalid.profileScore = 4;
    try { service.recordCompleted(invalid); check(false, "unfinished Ping Pong score must be rejected"); }
    catch (const std::invalid_argument&) { check(true, "unfinished Ping Pong score is rejected"); }
    invalid = match(id, persistence::GameKey::ClassicTicTacToe, persistence::MatchResult::Win, 30);
    invalid.profileScore = 0; invalid.opponentScore = 1;
    try { service.recordCompleted(invalid); check(false, "result/score contradiction must be rejected"); }
    catch (const std::invalid_argument&) { check(true, "result/score contradiction is rejected"); }
}

void finishXWin(classic_ttt::TicTacToeSession& session)
{
    session.playMove({0,0}); session.playMove({1,0}); session.playMove({0,1});
    session.playMove({1,1}); session.playMove({0,2});
}

void finishOWin(classic_ttt::TicTacToeSession& session)
{
    session.playMove({0,0}); session.playMove({0,1}); session.playMove({1,0});
    session.playMove({1,1}); session.playMove({2,2}); session.playMove({2,1});
}

void testRecorder(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "recorder.db");
    persistence::ProfileService profiles(database); profiles.bootstrap();
    persistence::MatchService matches(database); persistence::MatchRepository history(database);
    const auto profile = profiles.activeProfile().value();
    persistence::MatchRecorder recorder(matches);

    classic_ttt::TicTacToeSession ticTacToe;
    classic_ttt::SessionConfig xConfig; xConfig.mode=classic_ttt::GameMode::HumanVsComputer;
    xConfig.playerOneName="Edited Owner"; xConfig.humanMark=classic_ttt::Cell::X;
    xConfig.difficulty=classic_ttt::AIDifficulty::Hard; xConfig.bestOf=classic_ttt::BestOf::Single;
    ticTacToe.startNewMatch(xConfig); recorder.beginTicTacToe(profile, ticTacToe.config()); finishXWin(ticTacToe);
    check(recorder.completeTicTacToe(ticTacToe), "profile-as-X terminal win records");
    check(recorder.profileId() == profile.id,
          "recorder retains the persistent profile identity after successful finalization");
    check(!recorder.completeTicTacToe(ticTacToe) && history.count(profile.id,{}) == 1,
          "duplicate Tic-Tac-Toe finalization inserts only one row");
    auto rows=history.recent(profile.id,{},20,0);
    check(rows.front().result==persistence::MatchResult::Win && rows.front().profileDisplayName=="Edited Owner" &&
          rows.front().difficulty==persistence::DifficultyKey::Hard,
          "Tic-Tac-Toe X mapping preserves match display name and HVC difficulty");

    classic_ttt::SessionConfig oConfig=xConfig; oConfig.humanMark=classic_ttt::Cell::O; oConfig.difficulty=classic_ttt::AIDifficulty::Easy;
    ticTacToe.startNewMatch(oConfig); recorder.beginTicTacToe(profile,ticTacToe.config()); finishOWin(ticTacToe);
    check(recorder.completeTicTacToe(ticTacToe), "profile-as-O terminal win records");
    rows=history.recent(profile.id,{},20,0);
    check(rows.front().result==persistence::MatchResult::Win && rows.front().profileSideOrMark=="O",
          "Tic-Tac-Toe O winner maps from active-profile perspective");

    ticTacToe.startNewMatch(xConfig); recorder.beginTicTacToe(profile,ticTacToe.config()); finishOWin(ticTacToe);
    check(recorder.completeTicTacToe(ticTacToe), "profile-as-X terminal loss records");
    rows=history.recent(profile.id,{},20,0);
    check(rows.front().result==persistence::MatchResult::Loss, "Tic-Tac-Toe loser maps from active-profile perspective");

    const classic_ttt::Position drawMoves[]{{0,0},{0,1},{0,2},{1,1},{1,0},{1,2},{2,1},{2,0},{2,2}};
    ticTacToe.startNewMatch(xConfig); recorder.beginTicTacToe(profile,ticTacToe.config());
    for(const auto move:drawMoves) ticTacToe.playMove(move);
    check(recorder.completeTicTacToe(ticTacToe), "single-game Tic-Tac-Toe draw records");
    rows=history.recent(profile.id,{},20,0);
    check(rows.front().result==persistence::MatchResult::Draw && rows.front().drawValue==1,
          "draw maps from active-profile perspective with draw count");

    ping_pong::PingPongSession pong; ping_pong::SessionConfig pongConfig;
    pongConfig.leftPlayerName="Edited Pong"; pongConfig.mode=ping_pong::GameMode::HumanVsComputer;
    pongConfig.difficulty=ping_pong::AIDifficulty::Medium; pong.startMatch(pongConfig);
    recorder.beginPingPong(profile,pong.config());
    pong.update(3.0); for(int point=0;point<5;++point) {
        pong.awardPoint(ping_pong::Side::Right);
        if(point<4) { pong.update(ping_pong::PingPongSession::pointPauseSeconds); pong.update(ping_pong::PingPongSession::serveCountdownSeconds); }
    }
    check(recorder.completePingPong(pong), "Ping Pong profile loss records at first-to-five completion");
    check(!recorder.completePingPong(pong) && history.count(profile.id,{})==5,
          "duplicate Ping Pong finalization inserts only one row");
    rows=history.recent(profile.id,{},20,0);
    check(rows.front().result==persistence::MatchResult::Loss && rows.front().profileScore==0 &&
          rows.front().opponentScore==5 && rows.front().difficulty==persistence::DifficultyKey::Medium,
          "Ping Pong loss, score, and HVC difficulty map correctly");

    pongConfig.mode=ping_pong::GameMode::HumanVsHuman; pongConfig.rightPlayerName="Guest"; pong.startMatch(pongConfig);
    recorder.beginPingPong(profile,pong.config()); recorder.pause(); recorder.resume(); recorder.abandon();
    check(!recorder.completePingPong(pong) && history.count(profile.id,{})==5,
          "abandoned or incomplete Ping Pong match is never recorded");

    auto fakeNow=std::chrono::steady_clock::time_point{};
    std::int64_t fakeUtc=9000000000000LL;
    persistence::MatchRecorder timed(matches,[&]{return fakeNow;},[&]{return fakeUtc;});
    pong.startMatch(pongConfig); timed.beginPingPong(profile,pong.config());
    fakeNow+=std::chrono::milliseconds(7000); pong.rematch(); timed.restartPingPong(pong.config());
    fakeNow+=std::chrono::milliseconds(1000); timed.pause();
    fakeNow+=std::chrono::milliseconds(5000); timed.resume();
    fakeNow+=std::chrono::milliseconds(500); fakeUtc=9000000002000LL;
    pong.update(3.0); for(int point=0;point<5;++point){pong.awardPoint(ping_pong::Side::Left);if(point<4){pong.update(0.85);pong.update(3.0);}}
    check(timed.completePingPong(pong), "Ping Pong profile win records");
    rows=history.recent(profile.id,{},20,0);
    check(rows.front().result==persistence::MatchResult::Win && rows.front().durationMs==1500,
          "rematch resets duration and the monotonic timer excludes the paused interval");
    check(rows.front().difficulty==persistence::DifficultyKey::None,
          "Human-vs-human match persists None difficulty");

    const auto seriesProfile = profiles.createProfile("Series");
    classic_ttt::SessionConfig seriesConfig; seriesConfig.bestOf=classic_ttt::BestOf::Three;
    ticTacToe.startNewMatch(seriesConfig); persistence::MatchRecorder seriesRecorder(matches);
    seriesRecorder.beginTicTacToe(seriesProfile,ticTacToe.config()); finishXWin(ticTacToe);
    ticTacToe.nextRound(); finishXWin(ticTacToe);
    check(seriesRecorder.completeTicTacToe(ticTacToe), "completed best-of-three session records once");
    check(!seriesRecorder.completeTicTacToe(ticTacToe) && history.count(seriesProfile.id,{})==1,
          "duplicate best-of-three finalization does not add a round or second match row");
    const auto seriesRow=history.recent(seriesProfile.id,{},1,0).front();
    check(seriesRow.matchFormat=="best_of_3" && seriesRow.profileScore==2 && seriesRow.opponentScore==0,
          "best-of-three persists one final session score");
}
persistence::CompletedMatch boardMatch(std::int64_t profileId, persistence::GameKey game, persistence::MatchResult result,
                                       std::int64_t completedAt)
{
    const auto sides = persistence::boardGameSides(game).value();
    persistence::CompletedMatch value;
    value.profileId = profileId;
    value.game = game;
    value.mode = persistence::MatchMode::HumanVsComputer;
    value.opponentName = "Computer";
    value.profileDisplayName = "Owner";
    value.profileSideOrMark = sides.first;
    value.opponentSideOrMark = sides.second;
    value.result = result;
    if (persistence::boardGameRecordsPoints(game)) {
        value.profileScore = result == persistence::MatchResult::Win ? 3 : 1;
        value.opponentScore = result == persistence::MatchResult::Loss ? 3 : 1;
    }
    value.difficulty = persistence::DifficultyKey::Standard;
    value.matchFormat = "single";
    value.durationMs = 500;
    value.startedAt = completedAt - 100;
    value.completedAt = completedAt;
    return value;
}

void expectRejected(persistence::MatchService& service, const persistence::CompletedMatch& value, const std::string& message)
{
    bool rejected = false;
    try { service.recordCompleted(value); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, message);
}

void testBoardGameValidation(const std::filesystem::path& directory)
{
    using persistence::GameKey;
    using persistence::MatchResult;
    persistence::Database database(directory / "board-validation.db");
    persistence::ProfileService profiles(database); profiles.bootstrap();
    persistence::MatchService service(database);
    persistence::MatchRepository history(database);
    const auto id = profiles.activeProfile()->id;

    std::int64_t completedAt = 1000;
    int boardGames = 0;
    for (const auto game : persistence::allGameKeys()) {
        if (!persistence::boardGameSides(game)) continue;
        ++boardGames;
        bool accepted = true;
        try {
            service.recordCompleted(boardMatch(id, game, MatchResult::Draw, completedAt++));
            auto human = boardMatch(id, game, MatchResult::Win, completedAt++);
            human.mode = persistence::MatchMode::HumanVsHuman;
            human.difficulty = persistence::DifficultyKey::None;
            human.opponentName = "Guest";
            std::swap(human.profileSideOrMark, human.opponentSideOrMark);  // the profile may hold either seat
            service.recordCompleted(human);
        } catch (const std::exception&) {
            accepted = false;
        }
        check(accepted, std::string("valid board-game rows are recorded for ") + persistence::toStorage(game));
    }
    check(boardGames == 13, "13 board games use the shared single-game rules");
    check(!persistence::boardGameSides(GameKey::ClassicTicTacToe) && !persistence::boardGameSides(GameKey::PingPong),
          "Classic Tic-Tac-Toe and Ping Pong keep their own completion rules");
    check(history.count(id, {}) == 26, "every valid board-game row was stored");

    auto invalid = boardMatch(id, GameKey::Sus, MatchResult::Win, 5000);
    invalid.profileScore.reset(); invalid.opponentScore.reset();
    expectRejected(service, invalid, "SUS requires its final points");
    invalid = boardMatch(id, GameKey::MisereTicTacToe, MatchResult::Win, 5001);
    invalid.profileScore = 1; invalid.opponentScore = 0;
    expectRejected(service, invalid, "Misere does not store points");
    invalid = boardMatch(id, GameKey::FiveByFiveTicTacToe, MatchResult::Win, 5002);
    invalid.opponentScore = 5;
    expectRejected(service, invalid, "a 5x5 result that contradicts its points is refused");
    invalid = boardMatch(id, GameKey::Sus, MatchResult::Win, 5003);
    invalid.profileScore.reset();
    expectRejected(service, invalid, "one missing score is refused");
    invalid = boardMatch(id, GameKey::MisereTicTacToe, MatchResult::Win, 5004);
    invalid.difficulty = persistence::DifficultyKey::Hard;
    expectRejected(service, invalid, "a board-game computer opponent must be 'standard'");
    invalid = boardMatch(id, GameKey::Sus, MatchResult::Win, 5005);
    invalid.profileSideOrMark = "X"; invalid.opponentSideOrMark = "O";
    expectRejected(service, invalid, "SUS rows must use the S and U sides");
    invalid = boardMatch(id, GameKey::NumericalTicTacToe, MatchResult::Win, 5006);
    invalid.opponentSideOrMark = "Odd";
    expectRejected(service, invalid, "both players cannot hold the same side");
    invalid = boardMatch(id, GameKey::WordTicTacToe, MatchResult::Win, 5007);
    invalid.opponentSideOrMark.reset();
    expectRejected(service, invalid, "a missing side is refused");
    invalid = boardMatch(id, GameKey::Diamond, MatchResult::Win, 5008);
    invalid.matchFormat = "best_of_3";
    expectRejected(service, invalid, "board games are recorded as single games");
    invalid = boardMatch(id, GameKey::ObstacleTicTacToe, MatchResult::Draw, 5009);
    invalid.drawValue = 1;
    expectRejected(service, invalid, "board games store no draw count");
    invalid = boardMatch(id, GameKey::Sus, MatchResult::Win, 5012);
    invalid.profileScore = 5; invalid.opponentScore = 4;
    expectRejected(service, invalid, "SUS points above the grid's eight lines are refused");
    invalid = boardMatch(id, GameKey::FiveByFiveTicTacToe, MatchResult::Win, 5013);
    invalid.profileScore = 30; invalid.opponentScore = 19;
    expectRejected(service, invalid, "5x5 points above the board's 48 runs are refused");
    invalid = boardMatch(id, GameKey::Sus, MatchResult::Win, 5014);
    invalid.profileScore = 2147483647; invalid.opponentScore = 2147483646;
    expectRejected(service, invalid, "huge point totals are refused without overflowing");
    auto full = boardMatch(id, GameKey::Sus, MatchResult::Win, 5015);
    full.profileScore = 5; full.opponentScore = 3;
    bool accepted = true;
    try { service.recordCompleted(full); } catch (const std::exception&) { accepted = false; }
    check(accepted, "a SUS result using all eight lines is accepted");

    auto classic = match(id, GameKey::ClassicTicTacToe, MatchResult::Win, 5010);
    classic.mode = persistence::MatchMode::HumanVsComputer;
    classic.difficulty = persistence::DifficultyKey::Standard;
    expectRejected(service, classic, "Classic Tic-Tac-Toe difficulty must be Easy, Medium, or Hard");
    auto pong = match(id, GameKey::PingPong, MatchResult::Win, 5011);
    pong.mode = persistence::MatchMode::HumanVsComputer;
    pong.difficulty = persistence::DifficultyKey::Standard;
    expectRejected(service, pong, "Ping Pong difficulty must be Easy, Medium, or Hard");
    check(history.count(id, {}) == 27, "rejected rows are never stored (only the full SUS result was added)");
}

void testBoardGameRecorder(const std::filesystem::path& directory)
{
    using persistence::GameKey;
    using persistence::MatchResult;
    persistence::Database database(directory / "board-recorder.db");
    persistence::ProfileService profiles(database); profiles.bootstrap();
    persistence::MatchService matches(database);
    persistence::MatchRepository history(database);
    persistence::StatisticsRepository statistics(database);
    const auto profile = profiles.activeProfile().value();

    auto fakeNow = std::chrono::steady_clock::time_point{};
    std::int64_t fakeUtc = 8000000000000LL;
    persistence::MatchRecorder recorder(matches, [&] { return fakeNow; }, [&] { return fakeUtc; });
    check(!recorder.completeBoardGame(MatchResult::Win, std::nullopt, std::nullopt),
          "nothing is recorded before a board game begins");
    bool threw = false;
    try {
        recorder.beginBoardGame(profile, GameKey::ClassicTicTacToe, persistence::MatchMode::HumanVsHuman, true, "A", "B");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "Classic Tic-Tac-Toe cannot be recorded as a shared board game");

    recorder.beginBoardGame(profile, GameKey::Sus, persistence::MatchMode::HumanVsComputer, false, "Ada", "Computer");
    fakeNow += std::chrono::milliseconds(4200);
    fakeUtc += 4200;
    check(recorder.completeBoardGame(MatchResult::Win, 3, 2), "a finished SUS game records");
    check(!recorder.completeBoardGame(MatchResult::Win, 3, 2) && history.count(profile.id, {}) == 1,
          "a finished board game is recorded exactly once");
    auto row = history.recent(profile.id, {}, 1, 0).front();
    check(row.game == GameKey::Sus && row.profileSideOrMark == "U" && row.opponentSideOrMark == "S" &&
              row.profileScore == 3 && row.opponentScore == 2 && !row.drawValue,
          "the profile playing second is stored as U with its points");
    check(row.difficulty == persistence::DifficultyKey::Standard && row.matchFormat == "single" &&
              row.profileDisplayName == "Ada" && row.opponentName == "Computer" && row.durationMs == 4200,
          "mode, difficulty, format, names, and duration are stored");

    recorder.restartBoardGame();
    fakeNow += std::chrono::milliseconds(1000);
    check(recorder.completeBoardGame(MatchResult::Loss, 0, 1), "a rematch records a new game");
    row = history.recent(profile.id, {}, 1, 0).front();
    check(row.result == MatchResult::Loss && row.durationMs == 1000 && row.profileSideOrMark == "U",
          "the rematch keeps the sides and restarts the clock");

    recorder.beginBoardGame(profile, GameKey::MisereTicTacToe, persistence::MatchMode::HumanVsHuman, true, "Ada", "Guest");
    recorder.restartBoardGame();  // Restart mid-game: the abandoned game is never stored.
    check(recorder.completeBoardGame(MatchResult::Draw, std::nullopt, std::nullopt), "a Misere draw records");
    row = history.recent(profile.id, {}, 1, 0).front();
    check(row.game == GameKey::MisereTicTacToe && row.result == MatchResult::Draw && row.profileSideOrMark == "X" &&
              !row.profileScore && !row.opponentScore && row.difficulty == persistence::DifficultyKey::None,
          "a human-vs-human Misere draw stores X, no points, and no difficulty");
    check(history.count(profile.id, {}) == 3, "restarting mid-game stores nothing for the abandoned game");

    recorder.beginBoardGame(profile, GameKey::FiveByFiveTicTacToe, persistence::MatchMode::HumanVsComputer, true, "Ada",
                            "Computer");
    recorder.abandon();
    recorder.restartBoardGame();
    check(!recorder.completeBoardGame(MatchResult::Win, 4, 1) && history.count(profile.id, {}) == 3,
          "an abandoned board game is never stored, even after a restart request");

    // Starting another game before the first one ends replaces it: only the second is recorded.
    recorder.beginBoardGame(profile, GameKey::Sus, persistence::MatchMode::HumanVsComputer, true, "Ada", "Computer");
    recorder.beginBoardGame(profile, GameKey::MisereTicTacToe, persistence::MatchMode::HumanVsHuman, false, "Ada", "Guest");
    check(recorder.completeBoardGame(MatchResult::Win, std::nullopt, std::nullopt) && history.count(profile.id, {}) == 4,
          "a game begun over an unfinished one is recorded once");
    row = history.recent(profile.id, {}, 1, 0).front();
    check(row.game == GameKey::MisereTicTacToe && row.profileSideOrMark == "O" && row.opponentName == "Guest" &&
              row.difficulty == persistence::DifficultyKey::None,
          "the replacing game's own settings are stored, not the abandoned game's");
    check(history.count(profile.id, [] { persistence::MatchFilter filter; filter.game = GameKey::Sus; return filter; }()) == 2,
          "the replaced SUS game was never stored");

    persistence::MatchRecorder failing(matches, [&] { return fakeNow; }, [&] { return fakeUtc; });
    failing.beginBoardGame(profile, GameKey::FiveByFiveTicTacToe, persistence::MatchMode::HumanVsHuman, true, "Ada",
                           "Guest");
    threw = false;
    try { failing.completeBoardGame(MatchResult::Win, std::nullopt, std::nullopt); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw && !failing.completeBoardGame(MatchResult::Win, 4, 1) && history.count(profile.id, {}) == 4,
          "a refused completion is not retried into a second write");

    const auto sus = statistics.forGame(profile.id, GameKey::Sus);
    check(sus.matches == 2 && sus.wins == 1 && sus.losses == 1 && sus.pointsScored == 3 && sus.pointsConceded == 3,
          "board-game statistics derive from the stored rows");
    check(statistics.overall(profile.id).matches == 4, "board games count toward overall statistics");
}

void testPerGameStatistics(const std::filesystem::path& directory)
{
    using persistence::GameKey;
    using persistence::MatchResult;
    persistence::Database database(directory / "per-game.db");
    persistence::ProfileService profiles(database); profiles.bootstrap();
    persistence::MatchService service(database);
    persistence::StatisticsRepository statistics(database);
    const auto owner = profiles.activeProfile().value();
    const auto other = profiles.createProfile("Other");
    const auto& keys = persistence::allGameKeys();

    const auto empty = statistics.perGame(owner.id);
    bool zeroRows = empty.size() == keys.size() && keys.size() == 15;
    for (std::size_t index = 0; zeroRows && index < empty.size(); ++index) {
        const auto& row = empty[index];
        zeroRows = row.game == keys[index] && row.matches == 0 && row.wins == 0 && row.losses == 0 &&
                   row.draws == 0 && row.winRate == 0.0 && row.totalDurationMs == 0 && !row.lastPlayedAt;
    }
    check(zeroRows, "a profile without matches has a zero row for each of the 15 games, in catalogue order");

    service.recordCompleted(match(owner.id, GameKey::ClassicTicTacToe, MatchResult::Win, 100));
    service.recordCompleted(boardMatch(owner.id, GameKey::UltimateXo, MatchResult::Win, 200));
    auto asSecond = boardMatch(owner.id, GameKey::UltimateXo, MatchResult::Loss, 300);
    std::swap(asSecond.profileSideOrMark, asSecond.opponentSideOrMark);
    service.recordCompleted(asSecond);
    // Stored last but completed second: completion order is win, win, loss (best streak 2, current 0),
    // while insertion order would give win, loss, win (best 1, current 1).
    service.recordCompleted(boardMatch(owner.id, GameKey::UltimateXo, MatchResult::Win, 250));
    auto even = boardMatch(owner.id, GameKey::NumericalTicTacToe, MatchResult::Draw, 400);
    std::swap(even.profileSideOrMark, even.opponentSideOrMark);
    service.recordCompleted(even);
    service.recordCompleted(boardMatch(owner.id, GameKey::Sus, MatchResult::Win, 500));
    service.recordCompleted(boardMatch(other.id, GameKey::UltimateXo, MatchResult::Win, 600));

    const auto rows = statistics.perGame(owner.id);
    const auto rowFor = [&rows](GameKey game) {
        const auto found = std::find_if(rows.begin(), rows.end(), [game](const auto& row) { return row.game == game; });
        return found == rows.end() ? persistence::GameSummary{} : *found;
    };
    bool ordered = rows.size() == keys.size();
    for (std::size_t index = 0; ordered && index < rows.size(); ++index) ordered = rows[index].game == keys[index];
    check(ordered, "per-game rows keep catalogue order once games have been played");
    const auto ultimate = rowFor(GameKey::UltimateXo);
    check(ultimate.matches == 3 && ultimate.wins == 2 && ultimate.losses == 1 && ultimate.draws == 0 &&
              std::abs(ultimate.winRate - 2.0 / 3.0) < 1e-9,
          "a game's row counts only that game's results for the profile");
    check(ultimate.totalDurationMs == 1500 && ultimate.lastPlayedAt == 300,
          "play time sums the game's rows and last played is the latest completion, not the latest insert");
    const auto classic = rowFor(GameKey::ClassicTicTacToe);
    check(classic.matches == 1 && classic.wins == 1 && classic.totalDurationMs == 1100 && classic.lastPlayedAt == 100,
          "Classic Tic-Tac-Toe has its own row");
    check(rowFor(GameKey::NumericalTicTacToe).draws == 1 && rowFor(GameKey::Sus).wins == 1,
          "every played game gets its own totals");
    const auto pong = rowFor(GameKey::PingPong);
    check(pong.matches == 0 && !pong.lastPlayedAt, "an unplayed game keeps a zero row between played ones");

    std::int64_t matches = 0, wins = 0, losses = 0, draws = 0, duration = 0;
    for (const auto& row : rows) {
        matches += row.matches; wins += row.wins; losses += row.losses; draws += row.draws;
        duration += row.totalDurationMs;
    }
    const auto overall = statistics.overall(owner.id);
    check(matches == overall.matches && wins == overall.wins && losses == overall.losses && draws == overall.draws &&
              duration == overall.totalDurationMs && matches == 6,
          "the per-game rows add up to the overall totals");
    check(statistics.perGame(other.id)[11].matches == 1 && statistics.perGame(other.id)[11].game == GameKey::UltimateXo,
          "another profile's matches stay in its own rows");

    const auto ultimateDetail = statistics.forGame(owner.id, GameKey::UltimateXo);
    check(ultimateDetail.firstSideMatches == 2 && ultimateDetail.secondSideMatches == 1,
          "board-game side counts separate the side that moves first (X) from the second (O)");
    check(ultimateDetail.currentWinStreak == 0 && ultimateDetail.bestWinStreak == 2,
          "a game's streaks follow completion order, not insertion order");
    const auto numerical = statistics.forGame(owner.id, GameKey::NumericalTicTacToe);
    check(numerical.firstSideMatches == 0 && numerical.secondSideMatches == 1,
          "Numerical side counts use its Odd and Even sides");
    const auto classicDetail = statistics.forGame(owner.id, GameKey::ClassicTicTacToe);
    check(classicDetail.ticTacToeAsX == 1 && classicDetail.firstSideMatches == 0 && classicDetail.secondSideMatches == 0,
          "Classic Tic-Tac-Toe keeps its X / O counts and has no shared board-game sides");

    // The labels the statistics card special-cases, points, and Ping Pong's lack of shared sides.
    auto secondWord = boardMatch(owner.id, GameKey::WordTicTacToe, MatchResult::Win, 700);
    std::swap(secondWord.profileSideOrMark, secondWord.opponentSideOrMark);
    service.recordCompleted(secondWord);
    auto susAsU = boardMatch(owner.id, GameKey::Sus, MatchResult::Loss, 710);
    std::swap(susAsU.profileSideOrMark, susAsU.opponentSideOrMark);
    service.recordCompleted(susAsU);
    service.recordCompleted(match(owner.id, GameKey::PingPong, MatchResult::Win, 720));
    const auto word = statistics.forGame(owner.id, GameKey::WordTicTacToe);
    check(word.firstSideMatches == 0 && word.secondSideMatches == 1, "Word side counts use its First and Second sides");
    const auto sus = statistics.forGame(owner.id, GameKey::Sus);
    check(sus.firstSideMatches == 1 && sus.secondSideMatches == 1, "SUS side counts use its S and U sides");
    check(sus.pointsScored == 4 && sus.pointsConceded == 4 && sus.bestFinalMargin == 2,
          "SUS points and the best final margin come from the stored scores (3-1 and 1-3)");
    const auto pongDetail = statistics.forGame(owner.id, GameKey::PingPong);
    check(pongDetail.matches == 1 && pongDetail.firstSideMatches == 0 && pongDetail.secondSideMatches == 0,
          "Ping Pong has no shared board-game sides");
}
} // namespace

int main()
{
    TemporaryDirectory temporary;
    testMigration(temporary.path);
    testInsertHistoryAndStats(temporary.path);
    testValidation(temporary.path);
    testRecorder(temporary.path);
    testBoardGameValidation(temporary.path);
    testBoardGameRecorder(temporary.path);
    testPerGameStatistics(temporary.path);
    if (failures == 0) { std::cout << "Match history tests passed: " << checks << " checks\n"; return 0; }
    std::cerr << failures << " of " << checks << " match-history checks failed\n";
    return 1;
}
