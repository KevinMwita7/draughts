#include "draughts/search.h"
#include "draughts/eval.h"
#include "draughts/movegen.h"
#include <chrono>

namespace draughts {

namespace {

struct State {
    const SearchParams& params;
    Evaluator&          eval;
    uint64_t            nodes   = 0;
    bool                stopped = false;
};

Score negamax(Position& pos, int depth, Score alpha, Score beta, State& st) {
    if (st.params.max_nodes > 0 && st.nodes >= st.params.max_nodes) {
        st.stopped = true;
        return SCORE_NONE;
    }
    ++st.nodes;

    if (depth == 0)
        return st.eval.evaluate(pos);

    MoveList moves;
    generate_moves(pos, moves);

    if (moves.empty())
        return SCORE_LOSS;

    for (int i = 0; i < moves.count; ++i) {
        pos.do_move(moves[i]);
        Score score = -negamax(pos, depth - 1, -beta, -alpha, st);
        pos.undo_move(moves[i]);

        if (st.stopped)
            return SCORE_NONE;

        if (score > alpha) {
            alpha = score;
            if (alpha >= beta)
                break;
        }
    }
    return alpha;
}

} // namespace

SearchResult search(Position& pos, const SearchParams& params,
                    Evaluator& eval, InfoCallback cb) {
    SearchResult result;

    MoveList root_moves;
    generate_moves(pos, root_moves);
    if (root_moves.empty()) {
        result.score = SCORE_LOSS;
        return result;
    }

    int  max_depth = (params.max_depth > 0) ? params.max_depth : 128;
    auto start     = std::chrono::steady_clock::now();
    State st{params, eval};

    for (int depth = 1; depth <= max_depth && !st.stopped; ++depth) {
        Score alpha = -SCORE_INF;
        Move  best  = root_moves[0];

        for (int i = 0; i < root_moves.count; ++i) {
            pos.do_move(root_moves[i]);
            Score score = -negamax(pos, depth - 1, -SCORE_INF, -alpha, st);
            pos.undo_move(root_moves[i]);

            if (st.stopped)
                break;

            if (score > alpha) {
                alpha = score;
                best  = root_moves[i];
            }
        }

        auto now = std::chrono::steady_clock::now();
        result.elapsed_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());
        result.nodes = st.nodes;

        if (!st.stopped) {
            result.best_move = best;
            result.score     = alpha;
            result.depth     = depth;
        }

        if (cb)
            cb(result);

        if (params.time_ms > 0 && result.elapsed_ms >= params.time_ms)
            break;

        if (result.score >= SCORE_WIN)
            break;
    }

    return result;
}

} // namespace draughts
