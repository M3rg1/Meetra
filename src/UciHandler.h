#ifndef POPPER_UCIHANDLER_H
#define POPPER_UCIHANDLER_H


namespace Popper {

    class UciHandler {

    public:
        static void listen();

    private:
        static void uciCommand();
        static void isReadyCommand();
        static void goCommand();
        static void uciNewGameCommand();
        static void positionCommand();
        static void stopCommand();
        static void quitCommand();
    };

}

#endif //POPPER_UCIHANDLER_H
