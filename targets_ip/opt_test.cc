#include <Eigen/Core>
#include <LBFGS.h>
#include <gsl/gsl_multimin.h>
#include <iostream>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE OptTest

#include <boost/test/unit_test.hpp>

#include "opt.h"

using Eigen::VectorXd;
using LBFGSpp::LBFGSParam;
using LBFGSpp::LBFGSSolver;

class Rosenbrock {
 private:
	int n;

 public:
	explicit Rosenbrock(int n_) : n(n_) {}
	double operator()(const VectorXd &x, VectorXd &grad) {
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

double my_f(const gsl_vector *v, void *params) {
	auto &p = *reinterpret_cast<std::vector<double> *>(params);
	double x = gsl_vector_get(v, 0);
	double y = gsl_vector_get(v, 1);

	return p[2] * (x - p[0]) * (x - p[0]) + p[3] * (y - p[1]) * (y - p[1]) +
		   p[4];
}

BOOST_AUTO_TEST_CASE(test_gsl_paraboloid) {
	std::vector<double> params{1.0, 2.0, 10.0, 20.0, 30.0};
	std::vector<double> result;
	int iter = optimize({5, 7}, {1, 1}, &params, my_f, &result);
	BOOST_CHECK_EQUAL(iter, 23);
}
