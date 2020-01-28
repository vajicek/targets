#include <iostream>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE UtilsTest

#include <boost/test/unit_test.hpp>

#include <utils.h>

using namespace cv;

BOOST_AUTO_TEST_CASE(test_dist) {
	BOOST_CHECK_CLOSE(dist(Vec2f(0, 0), Vec2f(1, 0)), 1.0, 0.0001);
	BOOST_CHECK_CLOSE(dist(Vec2f(0, 0), Vec2f(0, 1)), 1.0, 0.0001);
	BOOST_CHECK_CLOSE(dist(Vec2f(0, 0), Vec2f(1, 1)), std::sqrt(2), 0.0001);
}

BOOST_AUTO_TEST_CASE(test_intersection) {
	Vec2f r;
	BOOST_CHECK(intersection(Line(Vec2f(0, 0), Vec2f(1, 0)), Line(Vec2f(0, 0), Vec2f(0, 1)), r));
	BOOST_CHECK_SMALL(dist(r, Vec2f(0, 0)), 0.0001f);
	BOOST_CHECK(intersection(Line(Vec2f(444, 1025), Vec2f(549, -973)), Line(Vec2f(0, 0), Vec2f(0, 1)), r));
	BOOST_CHECK_SMALL(dist(r, Vec2f(0, 0)), 0.0001f);
}
