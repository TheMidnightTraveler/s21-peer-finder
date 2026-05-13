#pragma once
#include <string>

enum class ParticipantStatus {
    All,
    Assigned,
    Registered,
    InProgress,
    InReview,
    Accepted,
    Failed
};

inline std::string StatusToString(ParticipantStatus status) {
    switch (status) {
        case ParticipantStatus::Assigned:   return "ASSIGNED";
        case ParticipantStatus::Registered: return "REGISTERED";
        case ParticipantStatus::InProgress: return "IN_PROGRESS";
        case ParticipantStatus::InReview:   return "IN_REVIEWS";
        case ParticipantStatus::Accepted:   return "ACCEPTED";
        case ParticipantStatus::Failed:     return "FAILED";
        default:                            return "";
    }
}

inline ParticipantStatus StatusFromString(const std::string& s) {
    if (s == "ASSIGNED")    return ParticipantStatus::Assigned;
    if (s == "REGISTERED")  return ParticipantStatus::Registered;
    if (s == "IN_PROGRESS") return ParticipantStatus::InProgress;
    if (s == "IN_REVIEWS")  return ParticipantStatus::InReview;
    if (s == "ACCEPTED")    return ParticipantStatus::Accepted;
    if (s == "FAILED")      return ParticipantStatus::Failed;
    return ParticipantStatus::All;
}