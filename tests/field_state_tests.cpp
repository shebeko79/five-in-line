#include <fstream>
#include "gtest/gtest.h"
#include "../db/solution_tree.h"
#include "../db/bin_index.h"
#include <boost/algorithm/string.hpp>
#include "../algo/field_state_player.h"

namespace fs=std::filesystem;

namespace Gomoku{

class field_state_test : public testing::Test
{
protected:
	virtual void SetUp();
	virtual void TearDown();
	State5::node_t solve(const std::string& steps_str);
};

void field_state_test::SetUp()
{
}

void field_state_test::TearDown()
{
}

State5::node_t field_state_test::solve(const std::string& steps_str)
{
	steps_t init_state;
	hex_or_str2points(steps_str,init_state);

	reorder_to_proper_last_color(init_state);
	Step last_step=last_color(init_state.size());

	game_t gm;
	gm.field().set_steps(init_state);

	State5::field_state_player_t pl;

	pl.init(gm,other_color(last_step));
	return pl.solve();
}

TEST_F(field_state_test, false_fork)
{
	State5::common_deep=2;
	State5::gl_threat_deep=2;

	State5::node_t node = solve("(0,0:X);(0,-1:O);(1,0:X);(-1,-1:O);(-1,0:X);(-2,0:O);(-3,-2:X);(-3,-1:O);(-3,0:X);(2,-1:O);(2,0:X);(3,0:O)");
	const auto& neutrals=node.get_neutrals();
	points_t pts(neutrals.begin(),neutrals.end());
	sort(pts,less_point_pr());
	ASSERT_EQ(points_t({ {-2,-1},{1,-1} }), pts);
}

TEST_F(field_state_test, false_fork1)
{
	State5::common_deep=2;
	State5::gl_threat_deep=2;

	State5::node_t node = solve("(0,0:X);(0,-1:O);(1,0:X);(-1,-1:O);(-1,0:X);(-2,0:O);(-3,-2:X);(-3,-1:O);(-3,0:X);(2,-1:O);(2,0:X)");
	const auto& neutrals=node.get_neutrals();
	points_t pts(neutrals.begin(),neutrals.end());
	sort(pts,less_point_pr());
	ASSERT_EQ(points_t({ {3,0} }), pts);
}

TEST_F(field_state_test, false_fork2)
{
	State5::common_deep=2;
	State5::gl_threat_deep=2;

	//(0,0:X);(0,-1:O);(1,0:X);(-1,-1:O);(-1,0:X);(-2,0:O);(-3,-2:X);(-3,-1:O);(-3,0:X);(2,-1:O);(-4,2:X);(-5,-1:O)
	State5::node_t node = solve("(0,0:X);(0,-1:O);(1,0:X);(-1,-1:O);(-1,0:X);(-2,0:O);(-3,-2:X);(-3,-1:O);(-3,0:X);(2,-1:O);(-4,2:X);(-5,-1:O);(1,-1:X)");
	ASSERT_EQ(true, node.get_wins().empty());
}

}//namespace
