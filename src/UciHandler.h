#ifndef MEETRA_UCIHANDLER_H
#define MEETRA_UCIHANDLER_H


namespace Meetra {

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

#endif //MEETRA_UCIHANDLER_H
