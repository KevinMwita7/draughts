#include "draughts/notation.h"
#include "draughts/movegen.h"
#include "draughts/zobrist.h"
#include <cctype>
#include <string>

namespace draughts {

Position parse_pdn_position(std::string_view pdn) {
    Position pos{};

    if (pdn.empty()) return pos;
    pos.side_to_move = (pdn[0] == 'W') ? WHITE : BLACK;

    // Skip to the first ':'
    size_t i = 0;
    while (i < pdn.size() && pdn[i] != ':') ++i;
    ++i;

    while (i < pdn.size()) {
        Color color = (pdn[i] == 'W') ? WHITE : BLACK;
        ++i;

        while (i < pdn.size() && pdn[i] != ':') {
            if (pdn[i] == ',') { ++i; continue; }

            bool king = (pdn[i] == 'K');
            if (king) ++i;

            int pdn_sq = 0;
            while (i < pdn.size() && std::isdigit((unsigned char)pdn[i]))
                pdn_sq = pdn_sq * 10 + (pdn[i++] - '0');

            pos.bb[color][king ? KING : MAN] |= sq_bb(Square(pdn_sq - 1));
        }

        if (i < pdn.size()) ++i; // skip ':'
    }

    pos.rebuild_derived();
    pos.hash = compute_hash(pos);
    return pos;
}

std::string to_pdn_position(const Position& pos) {
    std::string s;
    s += (pos.side_to_move == BLACK) ? 'B' : 'W';
    s += ':';

    auto append = [&](Bitboard pieces, bool king, bool& first) {
        while (pieces) {
            Square sq = pop_lsb(pieces);
            if (!first) s += ',';
            first = false;
            if (king) s += 'K';
            s += std::to_string(sq + 1);
        }
    };

    s += 'W';
    bool first = true;
    append(pos.bb[WHITE][KING], true,  first);
    append(pos.bb[WHITE][MAN],  false, first);

    s += ':';

    s += 'B';
    first = true;
    append(pos.bb[BLACK][KING], true,  first);
    append(pos.bb[BLACK][MAN],  false, first);

    return s;
}

Move parse_move(std::string_view text, const Position& pos) {
    int squares[16];
    int count = 0;
    int num   = 0;
    bool in_num = false;

    for (unsigned char c : text) {
        if (std::isdigit(c)) {
            num = num * 10 + (c - '0');
            in_num = true;
        } else if (in_num) {
            squares[count++] = num;
            num = 0;
            in_num = false;
        }
    }
    if (in_num) squares[count++] = num;
    if (count < 2) return NULL_MOVE;

    Square from = Square(squares[0] - 1);
    Square to   = Square(squares[count - 1] - 1);

    MoveList list;
    generate_moves(pos, list);

    for (const Move& m : list)
        if (m.from == from && m.to == to)
            return m;

    return NULL_MOVE;
}

std::string move_to_string(Move m) {
    std::string s;
    s += std::to_string(m.from + 1);
    s += (m.captured == 0) ? '-' : 'x';
    s += std::to_string(m.to + 1);
    return s;
}

std::vector<Move> parse_pdn(std::string_view pdn, Position start) {
    std::vector<Move> moves;
    Position pos = start;

    std::string token;

    auto process = [&](const std::string& tok) {
        if (tok.empty()) return;
        if (std::isdigit((unsigned char)tok[0]) && tok.back() == '.') return;
        if (tok == "1-0" || tok == "0-1" || tok == "1/2-1/2" || tok == "*") return;

        Move m = parse_move(tok, pos);
        if (!is_null(m)) {
            pos.do_move(m);
            moves.push_back(m);
        }
    };

    for (char c : pdn) {
        if (std::isspace((unsigned char)c)) {
            process(token);
            token.clear();
        } else {
            token += c;
        }
    }
    process(token);

    return moves;
}

} // namespace draughts
