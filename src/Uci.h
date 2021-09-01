#ifndef MEETRA_UCI_H
#define MEETRA_UCI_H

#include <string>

namespace Meetra::Uci {

    void Init();
    void Listen();
    void Send(const std::string &data);
}

#endif //MEETRA_UCI_H
