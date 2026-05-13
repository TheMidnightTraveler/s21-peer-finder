#pragma once
#include <string>
#include <vector>

struct Cluster {
    std::string name;
    std::vector<std::string> campusShortNames;
};

inline std::vector<std::string> GetExcludedCampuses() {
    return {
        "21 Test",
        "21 Test QA",
        "Canary 21",
        "School 21 online",
        "Школа 21 Старт, Новый Уренгой",
        "Школа 21 Старт, Челябинск",
        "Школа 21 Старт, Уфа",
        "Школа 21 Старт, Тестовый кампус",
        "Школа 21 Старт, Москва"
    };
}

inline std::vector<Cluster> GetClusters() {
    return {
        {
            "Казань / Москва / Новосибирск",
            {"21 Kazan", "21 Moscow", "21 Novosibirsk"}
        },
        {
            "Ташкент / Самарканд",
            {"21 Tashkent", "21 Samarkand"}
        },
        {
            "Остальные",
            {}
        }
    };
}