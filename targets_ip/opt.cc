#include "opt.h"

#include <gsl/gsl_multimin.h>
#include <iostream>

gsl_vector *create_init_vector(const std::vector<double> &values) {
	gsl_vector *vector = gsl_vector_alloc(values.size());
	for (int i = 0; i < values.size(); i++) {
		gsl_vector_set(vector, i, values[i]);
	}
	return vector;
}

void print(gsl_vector *v) {
	for (int i = 0; i < v->size; i++) {
		std::cout << gsl_vector_get(v, i) << ", ";
	}
	std::cout << "\n";
}

void set_vector(std::vector<double> *output, gsl_vector *v) {
	output->resize(v->size);
	for (int i = 0; i < v->size; i++) {
		(*output)[i] = gsl_vector_get(v, i);
	}
}

gsl_multimin_fminimizer *create_minimizer(
	const std::vector<double> &initial_solution,
	const std::vector<double> &initial_step_size,
	gsl_multimin_function *minex_func,
	void *parameters,
	double (*optimized_function)(const gsl_vector *, void *)) {
	gsl_vector *init_vect = create_init_vector(initial_solution);
	gsl_vector *step_vect = create_init_vector(initial_step_size);
	minex_func->n = init_vect->size;
	minex_func->f = optimized_function;
	minex_func->params = parameters;
	gsl_multimin_fminimizer *minimizer = gsl_multimin_fminimizer_alloc(
		gsl_multimin_fminimizer_nmsimplex2, init_vect->size);
	gsl_multimin_fminimizer_set(minimizer, minex_func, init_vect, step_vect);
	gsl_vector_free(init_vect);
	gsl_vector_free(step_vect);
	return minimizer;
}

int optimize(const std::vector<double> &initial_solution,
	const std::vector<double> &initial_step_size,
	void *parameters,
	double (*optimized_function)(const gsl_vector *, void *),
	std::vector<double> *result) {
	gsl_multimin_function minex_func;
	gsl_multimin_fminimizer *minimizer = create_minimizer(initial_solution,
		initial_step_size,
		&minex_func,
		parameters,
		optimized_function);

	int iter;
	for (iter = 0; iter < 1000; iter++) {
		int iterate_status = gsl_multimin_fminimizer_iterate(minimizer);

		if (iterate_status) {
			break;
		}

		double size = gsl_multimin_fminimizer_size(minimizer);
		int status = gsl_multimin_test_size(size, 1e-1);

		if (status != GSL_CONTINUE) {
			break;
		}
	}

	set_vector(result, minimizer->x);

	gsl_multimin_fminimizer_free(minimizer);

	return iter;
}
