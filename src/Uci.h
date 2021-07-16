#ifndef MEETRA_UCI_H
#define MEETRA_UCI_H

#include <string>

namespace Meetra::Uci {

        void Listen();
        void SendToGui(const std::string &data);

}

#endif //MEETRA_UCI_H
