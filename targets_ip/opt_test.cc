#include <gsl/gsl_multimin.h>
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

double my_f(const gsl_vector *v, void *params) {
	double x, y;
	double *p = reinterpret_cast<double*>(params);

	x = gsl_vector_get(v, 0);
	y = gsl_vector_get(v, 1);

	return p[2] * (x - p[0]) * (x - p[0]) +
		p[3] * (y - p[1]) * (y - p[1]) + p[4];
}

BOOST_AUTO_TEST_CASE(test_gsl_paraboloid) {
	double par[5] = {1.0, 2.0, 10.0, 20.0, 30.0};

	// Starting point
	gsl_vector *solution = gsl_vector_alloc(2);
	gsl_vector_set(solution, 0, 5.0);
	gsl_vector_set(solution, 1, 7.0);

	// Set initial step sizes to 1
	gsl_vector *step = gsl_vector_alloc(2);
	gsl_vector_set_all(step, 1.0);

	// Initialize method and iterate
	gsl_multimin_function minex_func;
	minex_func.n = 2;
	minex_func.f = my_f;
	minex_func.params = par;

	gsl_multimin_fminimizer *minimizer =
		gsl_multimin_fminimizer_alloc(gsl_multimin_fminimizer_nmsimplex2, 2);
	gsl_multimin_fminimizer_set(minimizer, &minex_func, solution, step);

	int iter;
	for (iter = 0; iter < 100; iter++) {
		int iterate_status = gsl_multimin_fminimizer_iterate(minimizer);

		if (iterate_status) {
			break;
		}

		double size = gsl_multimin_fminimizer_size(minimizer);
		int status = gsl_multimin_test_size(size, 1e-2);

		if (status != GSL_CONTINUE) {
			break;
		}
	}

	gsl_vector_free(solution);
	gsl_vector_free(step);
	gsl_multimin_fminimizer_free(minimizer);

	BOOST_CHECK_EQUAL(iter, 23);
}
