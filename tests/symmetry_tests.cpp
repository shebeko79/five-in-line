#include <fstream>
#include "gtest/gtest.h"
#include "../db/solution_tree.h"
#include "../db/bin_index.h"
#include <boost/algorithm/string.hpp>
#include "../algo/symmetry.h"

namespace fs=std::filesystem;

namespace Gomoku{

class symmetry_test : public testing::Test
{
protected:
	virtual void SetUp();
	virtual void TearDown();
	std::string minimize(const std::string& steps_str);
};

void symmetry_test::SetUp()
{
}

void symmetry_test::TearDown()
{
}

std::string symmetry_test::minimize(const std::string& steps_str)
{
	steps_t steps = scan_steps(steps_str);

	auto tr = Symmetry::minimal(steps);
	Symmetry::transform(steps, tr);
	sort_steps(steps);
	return print_steps(steps);
}

TEST_F(symmetry_test, common)
{
	std::string lowest = "(0,0:X);(0,1:O);(0,2:O);(0,3:O)";

	std::string self = minimize(lowest);
	ASSERT_EQ(lowest, self);

	std::string h1 = minimize("(0,0:X);(1,0:O);(2,0:O);(3,0:O)");
	ASSERT_EQ(lowest, h1);

	std::string h2 = minimize("(0,0:X);(-1,0:O);(-2,0:O);(-3,0:O)");
	ASSERT_EQ(lowest, h2);

	std::string shifted_h = minimize("(-11,5:O);(-10,5:O);(-9,5:O);(-8,5:X)");
	ASSERT_EQ(lowest, shifted_h);

	std::string not_in_line1 = minimize("(-11,6:O);(-10,5:O);(-9,5:O);(-8,5:X)");
	ASSERT_NE(lowest, not_in_line1);
}

}//namespace
