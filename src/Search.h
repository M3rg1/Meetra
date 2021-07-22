#ifndef MEETRA_SEARCH_H
#define MEETRA_SEARCH_H

#include "Board.h"
#include "TranspositionTable.h"
#include "Timer.h"
#include "Misc.h"
#include "PVTable.h"
#include <mutex>
#include <future>
#include "Evaluation.h"

namespace Meetra {

    class ABSearch {

    public:

        struct SearchSettings {
            Depth max_allowed_depth = DEFAULT_SEARCH_DEPTH;
            bool fixed_timer = false;
            bool infinite = false;
            long allowed_time = DEFAULT_SEARCH_TIME;

            long white_time = 0;
            long black_time = 0;
            long white_increment = 0;
            long black_increment = 0;

            long info_to_ui_ms_timer = DEFAULT_INFO_INTERVAL;
        };

        ABSearch();
        void StartSearch(SearchSettings settings, Board board);

        inline void SetNumThreads(int num) { num_threads = std::clamp(num, 1, MAX_SEARCH_THREADS); }
        inline void ClearTT() { tt.Clear(); pvt.Clear(); }
        inline void SetTTSize(size_t size_mb) { tt.Resize(size_mb);  pvt.Clear(); }
        inline void ShowShowCurrLine(bool show) { show_currline = show; }
        inline void SetPliesMuted(int ply_muted) { plies_muted = ply_muted; }
        inline void ShowCurrMoveInfo(bool show) { show_currmove = show; }
        inline void StopSearch() {
            run = false;
            search_timer.Stop();
            info_timer.Stop();
        }
        inline void SetMultiPv(int pv_num) {
            if (pv_num < 1) return;
            multi_pv = pv_num;
        }
        [[nodiscard]] inline bool IsSearching() const { return run; }

    private:

        struct p_MoveNodes {
            Move move;
            Score score;
            Score previous_score;
            Depth seldepth;
            uint nodes;

            explicit p_MoveNodes(Move m){
                move = m;
                score = NEGATIVE_INF;
                previous_score = NEGATIVE_INF;
                seldepth = 0;
                nodes = 0;
            }

            bool operator==(const Move& m) const { return move == m; }
            bool operator<(const p_MoveNodes& mn) const {
                return mn.nodes != nodes ? mn.nodes < nodes : mn.score < score;
            }
/*            bool operator<(const p_MoveNodes& mn) const {
                return mn.score != score ? mn.score < score : mn.previous_score < previous_score;
            }*/
        };

        [[nodiscard]] std::string GetSearchInfo(Board &board, std::vector<p_MoveNodes> &moves);
        [[nodiscard]] std::string GetCurrMoveInfo(Move move, int num, Board &board) const;
        [[nodiscard]] std::string GetUpdateSearchInfo() const;

        void MainSearch(Board board, int thread, std::vector<p_MoveNodes> moves);
        Score NegaMax(Board &board, Score alpha, Score beta, Depth depth, Depth ply, int thread, uint &nodes);
        Score QSearch(Board &board, Score alpha, Score beta, Depth ply, int thread, uint &nodes);

        void InitSearchTimer(Board &board);
        void InitSearch(SearchSettings &settings, Board &board);
        void RetrievePv(Board &board, Move *pv_line, Depth depth) const;
        void BackupPv(Board &board, Depth depth);
        [[nodiscard]] std::vector<p_MoveNodes> GenRootMoves(Board &board) const;
        [[nodiscard]] bool MateInHorizon(Depth curr_depth, Score score) const;
        [[nodiscard]] bool EnoughTimeLeft() const;
        [[nodiscard]] long ElapsedTimeMs() const;

        std::vector<std::future<void>> futures;

        TranspositionTable tt;
        PVTable pvt;
        volatile bool run;
        SearchSettings settings;
        Timer search_timer;
        Timer info_timer;
        bool show_currline;
        bool show_currmove;
        int plies_muted;
        size_t multi_pv;
        int num_threads;
        Move main_move;

        ulong nodes_explored;
        Depth curr_max_depth;
        Depth qsearch_depth;
        long timer_start;
    };

}

#endif //MEETRA_SEARCH_H
