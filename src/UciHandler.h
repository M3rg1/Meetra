#ifndef POPPER_UCIHANDLER_H
#define POPPER_UCIHANDLER_H

#include <string>

namespace Popper {

    class UciHandler {

    public:
        void Listen();

    private:
        void UciCommand();
        void IsReadyCommand();
        void GoCommand();
        void UciNewGameCommand();
        void PositionCommand();
        void StopCommand();
        void QuitCommand();
    };

}

#endif //POPPER_UCIHANDLER_H
