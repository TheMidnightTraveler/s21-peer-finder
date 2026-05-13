#pragma once
#include <tgbot/tgbot.h>
#include <map>
#include "UserSession.h"

class PeerFinderBot {
public:
    explicit PeerFinderBot(const std::string& token);
    void Run();

private:
    TgBot::Bot _bot;
    std::map<int64_t, UserSession> _sessions;

    void OnStart(TgBot::Message::Ptr message);
    void OnSearch(TgBot::Message::Ptr message);
    void OnMessage(TgBot::Message::Ptr message);
    void OnCallback(TgBot::CallbackQuery::Ptr query);

    void SendAuthMethodSelection(int64_t chatId);
    void SendClusterSelection(int64_t chatId, int64_t userId);
    void SendStatusSelection(int64_t chatId);
    void SendParticipants(int64_t chatId, int64_t userId);
    void SendParticipantsPage(int64_t chatId, int64_t userId);
    bool LoadMoreParticipants(int64_t userId);

    bool IsAuthenticated(int64_t userId);
    UserSession& GetSession(int64_t userId);
};
