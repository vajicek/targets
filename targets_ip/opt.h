#ifndef _OPT_H
#define _OPT_H
#pragma once

#include <gsl/gsl_vector.h>

#include <vector>

int optimize(const std::vector<double> &initial_solution,
	const std::vector<double> &initial_step_size,
	void* parameters,
	double (*optimized_function)(const gsl_vector *, void *),
	std::vector<double> *result);

#endif  // _OPT_H
