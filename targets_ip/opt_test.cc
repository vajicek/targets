#include <Eigen/Core>
#include <iostream>

#include <LBFGS.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE OptTest

#include <boost/test/unit_test.hpp>

using Eigen::VectorXd;
using LBFGSpp::LBFGSParam;
using LBFGSpp::LBFGSSolver;

class Rosenbrock {
 private:
	int n;
 public:
	explicit Rosenbrock(int n_) : n(n_) {}
	double operator()(const VectorXd& x, VectorXd& grad) {
		double fx = 0.0;
		for (int i = 0; i < n; i += 2) {
			double t1 = 1.0 - x[i];
			double t2 = 10 * (x[i + 1] - x[i] * x[i]);
			grad[i + 1] = 20 * t2;
			grad[i] = -2.0 * (x[i] * grad[i + 1] + t1);
			fx += t1 * t1 + t2 * t2;
		}
		return fx;
	}
};

BOOST_AUTO_TEST_CASE(test_lbfgs_rosenbrock) {
	const int n = 10;
	// Set up parameters
	LBFGSParam<double> param;
	param.epsilon = 1e-6;
	param.max_iterations = 100;

	// Create solver and function object
	LBFGSSolver<double> solver(param);
	Rosenbrock rosenbrock(n);

	// Initial guess
	VectorXd x = VectorXd::Zero(n);
	double fx;
	int iterations_count = solver.minimize(rosenbrock, x, fx);

	BOOST_CHECK_EQUAL(iterations_count, 23);
	BOOST_CHECK_SMALL(fx, 0.0001);
}
