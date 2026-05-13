#include "PeerFinderBot.h"
#include "../models/Cluster.h"
#include <sstream>

using std::string;
using std::vector;

PeerFinderBot::PeerFinderBot(const string& token)
    : _bot(token) {}

void PeerFinderBot::Run() {
    _bot.getEvents().onCommand("start", [this](TgBot::Message::Ptr msg) { OnStart(msg); });
    _bot.getEvents().onCommand("search", [this](TgBot::Message::Ptr msg) { OnSearch(msg); });
    _bot.getEvents().onNonCommandMessage([this](TgBot::Message::Ptr msg) { OnMessage(msg); });
    _bot.getEvents().onCallbackQuery([this](TgBot::CallbackQuery::Ptr q) { OnCallback(q); });

    TgBot::TgLongPoll longPoll(_bot);
    while (true) {
        try {
            longPoll.start();
        } catch (std::exception& e) {
            printf("Error: %s\n", e.what());
        }
    }
}

bool PeerFinderBot::IsAuthenticated(int64_t userId) {
    return GetSession(userId).isAuthenticated;
}

UserSession& PeerFinderBot::GetSession(int64_t userId) {
    return _sessions[userId];
}

void PeerFinderBot::OnStart(TgBot::Message::Ptr message) {
    auto& session = GetSession(message->from->id);
    session.Reset();
    SendAuthMethodSelection(message->chat->id);
}

void PeerFinderBot::OnSearch(TgBot::Message::Ptr message) {
    if (!IsAuthenticated(message->from->id)) {
        _bot.getApi().sendMessage(message->chat->id, "Сначала авторизуйся через /start.");
        return;
    }
    SendClusterSelection(message->chat->id, message->from->id);
}

void PeerFinderBot::OnMessage(TgBot::Message::Ptr message) {
    auto& session = GetSession(message->from->id);

    if (session.state == UserState::WaitingForToken) {
        session.api.SetToken(message->text);
        session.isAuthenticated = true;
        session.state = UserState::Idle;
        _bot.getApi().sendMessage(
            message->chat->id,
            "Токен принят! Используй /search для поиска."
        );
    } else if (session.state == UserState::WaitingForLogin) {
        session.login = message->text;
        session.state = UserState::WaitingForPassword;
        _bot.getApi().sendMessage(message->chat->id, "Теперь введи пароль:");
    } else if (session.state == UserState::WaitingForPassword) {
        string password = message->text;

        try {
            _bot.getApi().deleteMessage(message->chat->id, message->messageId);
        } catch (...) {}

        if (session.api.Authenticate(session.login, password)) {
            session.isAuthenticated = true;
            session.state = UserState::Idle;
            _bot.getApi().sendMessage(
                message->chat->id,
                "Авторизация успешна! Используй /search для поиска."
            );
        } else {
            session.state = UserState::WaitingForLogin;
            _bot.getApi().sendMessage(
                message->chat->id,
                "Неверный логин или пароль. Введи логин снова:"
            );
        }
    } else if (session.state == UserState::WaitingForProjectId) {
        try {
            session.projectId = std::stoi(message->text);
            session.state = UserState::WaitingForStatus;
            SendStatusSelection(message->chat->id);
        } catch (...) {
            _bot.getApi().sendMessage(message->chat->id, "Введи числовой id проекта.");
        }
    }
}

void PeerFinderBot::OnCallback(TgBot::CallbackQuery::Ptr query) {
    auto& session = GetSession(query->from->id);
    string data = query->data;

    if (data == "auth:token") {
        session.state = UserState::WaitingForToken;
        _bot.getApi().sendMessage(query->message->chat->id, "Вставь свой JWT токен:");
    } else if (data == "auth:login") {
        session.state = UserState::WaitingForLogin;
        _bot.getApi().sendMessage(query->message->chat->id, "Введи логин от платформы:");
    } else if (data.starts_with("cluster:")) {
        int clusterIndex = std::stoi(data.substr(8));
        auto clusters = GetClusters();
        auto excluded = GetExcludedCampuses();
        auto allCampuses = session.api.GetCampuses();
        auto& cluster = clusters[clusterIndex];

        session.campusIds.clear();
        for (auto& campus : allCampuses) {
            bool isExcluded = std::find(excluded.begin(), excluded.end(), campus.shortName) != excluded.end();
            if (isExcluded) continue;

            if (cluster.campusShortNames.empty()) {
                auto& c1 = clusters[0].campusShortNames;
                auto& c2 = clusters[1].campusShortNames;
                bool inOtherCluster =
                    std::find(c1.begin(), c1.end(), campus.shortName) != c1.end() ||
                    std::find(c2.begin(), c2.end(), campus.shortName) != c2.end();
                if (!inOtherCluster)
                    session.campusIds.push_back(campus.id);
            } else {
                bool inCluster = std::find(
                    cluster.campusShortNames.begin(),
                    cluster.campusShortNames.end(),
                    campus.shortName) != cluster.campusShortNames.end();
                if (inCluster)
                    session.campusIds.push_back(campus.id);
            }
        }

        session.state = UserState::WaitingForProjectId;
        _bot.getApi().sendMessage(query->message->chat->id, "Введи id проекта:");
    } else if (data.starts_with("status:")) {
        session.statusStr = data.substr(7);
        SendParticipants(query->message->chat->id, query->from->id);
    } else if (data == "page:prev") {
        if (session.currentPage > 0) {
            session.currentPage--;
            SendParticipantsPage(query->message->chat->id, query->from->id);
        }
    } else if (data == "page:next") {
        int nextPageStart = (session.currentPage + 1) * UserSession::PageSize;
        if (nextPageStart < (int)session.participants.size()) {
            session.currentPage++;
            SendParticipantsPage(query->message->chat->id, query->from->id);
        } else if (session.hasMore) {
            if (LoadMoreParticipants(query->from->id)) {
                session.currentPage++;
                SendParticipantsPage(query->message->chat->id, query->from->id);
            } else {
                _bot.getApi().answerCallbackQuery(query->id, "Больше участников нет.");
            }
        }
    }

    _bot.getApi().answerCallbackQuery(query->id);
}

bool PeerFinderBot::LoadMoreParticipants(int64_t userId) {
    auto& session = GetSession(userId);
    session.currentOffset += UserSession::PageSize;

    ParticipantStatus status = StatusFromString(session.statusStr);

    vector<Participant> nextBatch;
    for (auto& campusId : session.campusIds) {
        auto batch = session.api.GetParticipants(
            session.projectId, campusId, status,
            UserSession::PageSize, session.currentOffset
        );
        for (auto& p : batch) {
            bool exists = std::find_if(
                nextBatch.begin(), nextBatch.end(),
                [&p](const Participant& x) { return x.login == p.login; }
            ) != nextBatch.end();
            if (!exists)
                nextBatch.push_back(p);
        }
    }

    if (nextBatch.empty())
        return false;

    session.participants.insert(session.participants.end(), nextBatch.begin(), nextBatch.end());
    if ((int)nextBatch.size() < UserSession::PageSize)
        session.hasMore = false;
    return true;
}

void PeerFinderBot::SendAuthMethodSelection(int64_t chatId) {
    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

    auto makeButton = [](const string& text, const string& data) {
        TgBot::InlineKeyboardButton::Ptr btn(new TgBot::InlineKeyboardButton);
        btn->text = text;
        btn->callbackData = data;
        return btn;
    };

    keyboard->inlineKeyboard.push_back({makeButton("🔑 JWT токен", "auth:token")});
    keyboard->inlineKeyboard.push_back({makeButton("🔐 Логин и пароль", "auth:login")});

    _bot.getApi().sendMessage(
        chatId,
        "Привет! Я помогу найти участников проектов в Школе 21.\n"
        "Как хочешь авторизоваться?",
        nullptr, nullptr, keyboard
    );
}

void PeerFinderBot::SendClusterSelection(int64_t chatId, int64_t userId) {
    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    auto clusters = GetClusters();

    for (int i = 0; i < (int)clusters.size(); i++) {
        TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);
        button->text = clusters[i].name;
        button->callbackData = "cluster:" + std::to_string(i);
        keyboard->inlineKeyboard.push_back({button});
    }

    _bot.getApi().sendMessage(chatId, "Выбери кластер:", nullptr, nullptr, keyboard);
}

void PeerFinderBot::SendStatusSelection(int64_t chatId) {
    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

    vector<std::pair<string, string>> statuses = {
        {"Все",             "status:ALL"},
        {"Назначен",        "status:ASSIGNED"},
        {"Зарегистрирован", "status:REGISTERED"},
        {"В процессе",      "status:IN_PROGRESS"},
        {"На проверке",     "status:IN_REVIEWS"},
        {"Принят",          "status:ACCEPTED"},
        {"Провалил",        "status:FAILED"}
    };

    for (auto& [label, cb] : statuses) {
        TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);
        button->text = label;
        button->callbackData = cb;
        keyboard->inlineKeyboard.push_back({button});
    }

    _bot.getApi().sendMessage(chatId, "Выбери статус участников:", nullptr, nullptr, keyboard);
}

void PeerFinderBot::SendParticipants(int64_t chatId, int64_t userId) {
    auto& session = GetSession(userId);
    ParticipantStatus status = StatusFromString(session.statusStr);

    vector<Participant> firstPage;
    for (auto& campusId : session.campusIds) {
        auto participants = session.api.GetParticipants(
            session.projectId, campusId, status, UserSession::PageSize);

        for (auto& p : participants) {
            bool exists = std::find_if(
                firstPage.begin(), firstPage.end(),
                [&p](const Participant& x) { return x.login == p.login; }
            ) != firstPage.end();
            if (!exists)
                firstPage.push_back(p);
        }
    }

    if (firstPage.empty()) {
        _bot.getApi().sendMessage(chatId, "Участники не найдены.");
        return;
    }

    session.participants = firstPage;
    session.currentPage = 0;
    session.currentOffset = 0;
    session.state = UserState::Idle;

    SendParticipantsPage(chatId, userId);
}

void PeerFinderBot::SendParticipantsPage(int64_t chatId, int64_t userId) {
    auto& session = GetSession(userId);
    int page = session.currentPage;
    int start = page * UserSession::PageSize;
    int end = std::min(start + UserSession::PageSize, (int)session.participants.size());

    std::ostringstream oss;
    oss << "Участники " << (start + 1) << "-" << end << ":\n\n";
    for (int i = start; i < end; i++)
        oss << "• " << session.participants[i].login << "\n";

    auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
    vector<TgBot::InlineKeyboardButton::Ptr> row;

    if (page > 0) {
        TgBot::InlineKeyboardButton::Ptr btn(new TgBot::InlineKeyboardButton);
        btn->text = "◀️ Назад";
        btn->callbackData = "page:prev";
        row.push_back(btn);
    }

    if (end < (int)session.participants.size() || session.hasMore) {
        TgBot::InlineKeyboardButton::Ptr btn(new TgBot::InlineKeyboardButton);
        btn->text = "▶️ Вперёд";
        btn->callbackData = "page:next";
        row.push_back(btn);
    }

    if (!row.empty())
        keyboard->inlineKeyboard.push_back(row);

    _bot.getApi().sendMessage(chatId, oss.str(), nullptr, nullptr, keyboard);
}
