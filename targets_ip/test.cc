#include <iostream>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MyTest

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(my_test) {
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(my_second_test) {
    BOOST_CHECK(false);
}
