#pragma once
#include <string>
#include <vector>
#include "../api/S21Api.h"
#include "../models/Participant.h"

enum class UserState {
    Idle,
    WaitingForAuthMethod,
    WaitingForToken,
    WaitingForLogin,
    WaitingForPassword,
    WaitingForProjectId,
    WaitingForStatus
};

struct UserSession {
    UserState state = UserState::Idle;
    std::vector<std::string> campusIds;
    std::string statusStr;
    std::string login;
    int projectId = 0;
    bool isAuthenticated = false;
    S21Api api;

    std::vector<Participant> participants;
    int currentPage = 0;
    int currentOffset = 0;
    static const int PageSize = 25;
    bool hasMore = true;

    void Reset() {
        state = UserState::WaitingForLogin;
        campusIds.clear();
        statusStr = "";
        login = "";
        projectId = 0;
        isAuthenticated = false;
        participants.clear();
        currentPage = 0;
        currentOffset = 0;
        hasMore = true;
    }
};
