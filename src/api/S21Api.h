#pragma once
#include <string>
#include <vector>
#include "../models/Campus.h"
#include "../models/Participant.h"
#include "../models/ParticipantStatus.h"

namespace httplib{
    class Result;
}

class S21Api {
public:
    bool Authenticate(const std::string& login, const std::string& password);
    const std::string& GetToken() const { return _token; }
    void SetToken(const std::string& token) { _token = token; }

    std::vector<Campus> GetCampuses();
    std::vector<Participant> GetParticipants(
        int projectId,
        const std::string& campusId,
        ParticipantStatus status = ParticipantStatus::All,
        int limit = 50,
        int offset = 0
    );

private:
    std::string _token;
    const std::string _basePath = "/services/21-school/api/v1/";
    const std::string _sslHost = "platform.21-school.ru";

    bool ResponseSuccessful(const httplib::Result& response);
};