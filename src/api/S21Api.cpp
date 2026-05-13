#include "S21Api.h"
#include <httplib.h>
#include <nlohmann/json.hpp>

using std::string;
using std::vector;


bool S21Api::Authenticate(const string& login, const string& password) {
    httplib::SSLClient client("auth.21-school.ru");

    string body =   "client_id=s21-open-api"
                    "&username=" + login +
                    "&password=" + password +
                    "&grant_type=password";
    
    httplib::Result response = client.Post(
        "/auth/realms/EduPowerKeycloak/protocol/openid-connect/token",
        body,
        "application/x-www-form-urlencoded"
    );

    if (!ResponseSuccessful(response)) {
        return false;
    }

    auto json = nlohmann::json::parse(response->body);
    _token = json["access_token"].get<string>();
    return true;
}

vector<Campus> S21Api::GetCampuses() {
    httplib::SSLClient client(_sslHost);

    client.set_connection_timeout(10);
    client.set_read_timeout(10);

    httplib::Headers headers = {
        {"Authorization", "Bearer " + _token},
        {"accept", "*/*"}
    };

    httplib::Result response = client.Get(_basePath + "campuses", headers);

    if (!ResponseSuccessful(response)) {
        printf("GetCampuses failed, status: %d\n", response ? response->status : -1);
        return {};
    }

    auto json = nlohmann::json::parse(response->body);
    vector<Campus> campuses;

    for (auto& item : json["campuses"]) {
        Campus campus;
        campus.id = item["id"].get<string>();
        campus.shortName = item["shortName"].get<string>();
        campus.fullName = item["fullName"].get<string>();
        campuses.push_back(campus);
    }

    return campuses;
}

vector<Participant> S21Api::GetParticipants(int projectId, const string &campusId, ParticipantStatus status, int limit, int offset)
{
    httplib::SSLClient client (_sslHost);

    httplib::Headers headers = {
        {"Authorization", "Bearer " + _token},
        {"accept", "*/*"}
    };

    string path = _basePath + "projects/"
                + std::to_string(projectId)
                + "/participants?limit=" + std::to_string(limit)
                + "&offset=" + std::to_string(offset);
    
    string statusStr = StatusToString(status);
    if (!statusStr.empty())
        path += "&status=" + statusStr;

    if (!campusId.empty())
        path+= "&campusId=" + campusId;

    httplib::Result response = client.Get(path, headers);

    if (!ResponseSuccessful(response)) {
        return {};
    }

    auto json = nlohmann::json::parse(response->body);
    vector<Participant> participants;

    for (auto& item : json["participants"]) {
        Participant p;
        p.login = item.get<string>();
        participants.push_back(p);
    }

    return participants;
}

bool S21Api::ResponseSuccessful(const httplib::Result& response)
{
    return response && response->status == 200;
}
