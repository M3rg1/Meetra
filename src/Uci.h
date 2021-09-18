#ifndef MEETRA_UCI_H
#define MEETRA_UCI_H

#include <string>

namespace Meetra::Uci {

    void Init();
    void Listen();
    void Send(std::string_view data);
    void SendInfo(const std::string &data);
}

#endif //MEETRA_UCI_H
