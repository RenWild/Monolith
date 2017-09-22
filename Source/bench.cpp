/*
  Monolith 0.3  Copyright (C) 2017 Jonas Mayr

  This file is part of Monolith.

  Monolith is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Monolith is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Monolith. If not, see <http://www.gnu.org/licenses/>.
*/


#include "engine.h"
#include "search.h"
#include "logfile.h"
#include "chronos.h"
#include "position.h"
#include "bench.h"

namespace
{
	struct unit
	{
		std::string fen;
		int max_depth;
		std::string info;
	};

	const unit perft_pos[]
	{
		{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", 6, "119060324" },
		{ "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 5, "193690690" },
		{ "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -", 5, "89941194" },
		{ "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 7, "178633661" },
		{ "8/5p2/8/2k3P1/p3K3/8/1P6/8 b - -", 8, "64451405" }
	};

	const unit search_pos[]
	{
		{ "2k3r1/4b3/4P3/7B/p6K/P3p3/5n1Q/3q4 w - -", 1, "is mate" },
		{ "4k3/4P1p1/4K1P1/2p5/1pP5/1P1N2B1/8/8 b - -", 1, "is stalemate" },
		{ "8/1kn5/pn6/P6P/6r1/5K2/2r5/8 w - - 99 95", 5, "is draw by 50 moves" },
		{ "5B2/6P1/1p6/8/1N6/kP6/2K5/8 w - -", 8, "hakmem 70 | mate in 3 | bm g8N" },
		{ "8/8/p3R3/1p5p/1P5p/6rp/5K1p/7k w - -", 16, "zugzwang | nullmove | mate in 7 | bm Re1" },

		{ "7K/8/k1P5/7p/8/8/8/8 w - -", 13, "reti study | insu.ma.-draw | draw in 6 | bm Kg7" },
		{ "8/1kn5/pn6/P6P/6r1/5K2/2r5/8 w - - 97 115", 7, "50-move-rule-draw | draw in 2 | bm Ke3" },
		{ "k7/P5R1/7P/8/8/6P1/4r3/5K2 b - -", 14, "rep.-draw | razoring | draw in 7 | bm Rf2" },
		{ "6R1/P2k4/r7/5N1P/r7/p7/7K/8 w - -", 12, "djaja study | rep.-draw | bm Nh6 (unsolved)" },

		{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", 11, "startpos" },
		{ "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 11, "kiwipete | midgame search" },
		{ "rn1qk2r/p1pnbppp/bp2p3/3pN3/2PP4/1P4P1/P2BPPBP/RN1QK2R w KQkq -", 11, "silent but deadly | midgame search | bm Nxf7" },

		{ "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -", 28, "lasker-reichhelm | tt-test | mate in 32 | bm a1b1 (unsolved)" },
		{ "4k3/8/8/8/8/8/4P3/4K3 w - -", 25, "fine 70 | tt-test | mate in 22 | bm Kd2 (unsolved)" },
		{ "8/8/8/1k6/8/8/8/RK6 w - -", 25, "rook + king | tt-test | mate in 13 | bm Kc2 (unsolved)" },
		{ "3k4/1p6/1P1K4/2P5/8/8/8/8 w - -", 16, "tt-test | mate in 8 | bm Ke6" }
	};
}

void bench::perft(std::string mode)
{
	pos board;
	chronometer time;

	mode = (mode == "perft" ? "legal" : mode);
	GEN_MODE gen_mode{ mode == "pseudolegal" ? PSEUDO : LEGAL };

	log_file::open();
	analysis::reset();

	log::cout << "\nperft (" << mode << " move generation):\n"
		<< "\n=======================================\n";

	for (auto &p : perft_pos)
	{
		log::cout << p.fen << std::endl;
		engine::parse_fen(board, p.fen);

		analysis::root_perft(board, p.max_depth, gen_mode);
		log::cout << "      " << p.max_depth << ": " << p.info << std::endl;
	}

	analysis::summary(time);
}

void bench::search()
{
	log_file::open();
	uint32 ponder{ 0 };
	auto saved_depth{ engine::depth };

	pos board;
	chronos chrono;
	chronometer time;

	analysis::reset();
	engine::infinite = true;
	engine::nodes = lim::nodes;
	chrono.movetime = lim::movetime;

	log::cout << "\nbenchmark search positions:\n"
		<< "\n=======================================\n";

	for (auto &s : search_pos)
	{
		log::cout << std::endl << s.info
			<< "\n---------------------------------------\n";

		engine::new_game(board);
		engine::parse_fen(board, s.fen);

		engine::depth = s.max_depth;
		engine::stop = false;

		search::id_frame(board, chrono, ponder);
	}

	analysis::summary(time);

	engine::stop = true;
	engine::depth = saved_depth;
}