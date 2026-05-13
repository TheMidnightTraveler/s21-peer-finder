#include "bot/PeerFinderBot.h"
#include <cstdlib>
#include <stdexcept>

int main() {
    const char* token = std::getenv("BOT_TOKEN");
    if (!token)
        throw std::runtime_error("Переменная окружения BOT_TOKEN не задана");

    PeerFinderBot bot(token);
    bot.Run();
    return 0;
}
