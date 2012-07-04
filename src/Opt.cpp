/***************************************************************************
 *   Copyright (C) 2006-2008 by Antonello Lobianco                         *
 *   http://regmas.org                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdlib.h>

#include <cassert>


#include "Opt.h"



#include "ModelData.h"
#include "Pixel.h"
#include "ThreadManager.h"
#include "Scheduler.h"

using namespace Ipopt;

Opt::Opt(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
}

Opt::~Opt(){

}
bool
Opt::get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
									Index& nnz_h_lag, IndexStyleEnum& index_style){
	n = 20;

	m = n-2;

	// in this example the jacobian is dense. Hence, it contains n*m nonzeros
	nnz_jac_g = n*m;

	// the hessian is also dense and has n*n total nonzeros, but we
	// only need the lower left corner (since it is symmetric)
	nnz_h_lag = n*(n-1)/2+n;

	generate_tapes(n, m);

	// use the C style indexing (0-based)
	index_style = C_STYLE;

	return true;
}

bool
Opt::get_bounds_info(Index n, Number* x_l, Number* x_u, Index m, Number* g_l, Number* g_u){
	// none of the variables have bounds
	for (Index i=0; i<n; i++) {
		x_l[i] = -1e20;
		x_u[i] =  1e20;
	}

	// Set the bounds for the constraints
	for (Index i=0; i<m; i++) {
		g_l[i] = 0;
		g_u[i] = 0;
	}

	return true;
}

bool
Opt::get_starting_point(Index n, bool init_x, Number* x, bool init_z, Number* z_L, Number* z_U,
												Index m, bool init_lambda, Number* lambda){
	// Here, we assume we only have starting values for x, if you code
	// your own NLP, you can provide starting values for the others if
	// you wish.
	assert(init_x == true);
	assert(init_z == false);
	assert(init_lambda == false);

	// set the starting point
	for (Index i=0; i<n/2; i++) {
		x[2*i] = -1.2;
		x[2*i+1] = 1.;
	}
	if (n != 2*(n/2)) {
		x[n-1] = -1.2;
	}

	return true;
}

template<class T> bool
Opt::eval_obj(Index n, const T *x, T& obj_value){
	T a1, a2;
	obj_value = 0.;
	for (Index i=0; i<n-1; i++) {
		a1 = x[i]*x[i]-x[i+1];
		a2 = x[i] - 1.;
		obj_value += 100.*a1*a1 + a2*a2;
	}

	return true;
}

template<class T> bool
Opt::eval_constraints(Index n, const T *x, Index m, T* g){
	for (Index i=0; i<m; i++) {
		g[i] = 3.*pow(x[i+1],3.) + 2.*x[i+2] - 5.
					 + sin(x[i+1]-x[i+2])*sin(x[i+1]+x[i+2]) + 4.*x[i+1]
					 - x[i]*exp(x[i]-x[i+1]) - 3.;
	}

	return true;
}

//*************************************************************************
//
//
//         Nothing has to be changed below this point !!
//
//
//*************************************************************************


bool
Opt::eval_f(Index n, const Number* x, bool new_x, Number& obj_value){
	eval_obj(n,x,obj_value);

	return true;
}

bool
Opt::eval_grad_f(Index n, const Number* x, bool new_x, Number* grad_f){

	gradient(tag_f,n,x,grad_f);

	return true;
}

bool
Opt::eval_g(Index n, const Number* x, bool new_x, Index m, Number* g){

	eval_constraints(n,x,m,g);

	return true;
}

bool
Opt::eval_jac_g(Index n, const Number* x, bool new_x,Index m, Index nele_jac,
								Index* iRow, Index *jCol, Number* values){
	if (values == NULL) {
		// return the structure of the jacobian,
		// assuming that the Jacobian is dense

		Index idx = 0;
		for(Index i=0; i<m; i++)
			for(Index j=0; j<n; j++)
		{
			iRow[idx] = i;
			jCol[idx++] = j;
		}
 }
	else {
		// return the values of the jacobian of the constraints

		jacobian(tag_g,m,n,x,Jac);

		Index idx = 0;
		for(Index i=0; i<m; i++)
			for(Index j=0; j<n; j++)
			values[idx++] = Jac[i][j];

	}

	return true;
}

bool
Opt::eval_h(Index n, const Number* x, bool new_x, Number obj_factor, Index m, const Number* lambda,
						bool new_lambda, Index nele_hess, Index* iRow, Index* jCol, Number* values){
	if (values == NULL) {
		// return the structure. This is a symmetric matrix, fill the lower left
		// triangle only.

		// the hessian for this problem is actually dense
		Index idx=0;
		for (Index row = 0; row < n; row++) {
			for (Index col = 0; col <= row; col++) {
				iRow[idx] = row;
				jCol[idx] = col;
				idx++;
			}
		}

		assert(idx == nele_hess);
	}
	else {
		// return the values. This is a symmetric matrix, fill the lower left
		// triangle only

		for(Index i = 0; i<n ; i++)
			x_lam[i] = x[i];
		for(Index i = 0; i<m ; i++)
			x_lam[n+i] = lambda[i];
		x_lam[n+m] = obj_factor;

		hessian(tag_L,n+m+1,x_lam,Hess);

		Index idx = 0;

		for(Index i = 0; i<n ; i++)
			{
		for(Index j = 0; j<=i ; j++)
			{
				values[idx++] = Hess[i][j];
			}
			}
	}

	return true;
}

void
Opt::finalize_solution(SolverReturn status,
											 Index n, const Number* x, const Number* z_L, const Number* z_U,
											 Index m, const Number* g, const Number* lambda,
											 Number obj_value, const IpoptData* ip_data, IpoptCalculatedQuantities* ip_cq){

	printf("\n\nObjective value\n");
	printf("f(x*) = %e\n", obj_value);

// Memory deallocation for ADOL-C variables

	delete[] x_lam;

	for(Index i=0;i<m;i++)
		delete[] Jac[i];
	delete[] Jac;

	for(Index i=0;i<n+m+1;i++)
		delete[] Hess[i];
	delete[] Hess;
}


//***************    ADOL-C part ***********************************

void
Opt::generate_tapes(Index n, Index m){
	Number *xp    = new double[n];
	Number *lamp  = new double[m];
	Number *zl    = new double[m];
	Number *zu    = new double[m];

	adouble *xa   = new adouble[n];
	adouble *g    = new adouble[m];
	adouble *lam  = new adouble[m];
	adouble sig;
	adouble obj_value;

	double dummy;

	Jac = new double*[m];
	for(Index i=0;i<m;i++)
		Jac[i] = new double[n];

	x_lam   = new double[n+m+1];

	Hess = new double*[n+m+1];
	for(Index i=0;i<n+m+1;i++)
		Hess[i] = new double[i+1];

	get_starting_point(n, 1, xp, 0, zl, zu, m, 0, lamp);

	trace_on(tag_f);

		for(Index i=0;i<n;i++)
			xa[i] <<= xp[i];

		eval_obj(n,xa,obj_value);

		obj_value >>= dummy;

	trace_off();

	trace_on(tag_g);

		for(Index i=0;i<n;i++)
			xa[i] <<= xp[i];

		eval_constraints(n,xa,m,g);


		for(Index i=0;i<m;i++)
			g[i] >>= dummy;

	trace_off();

	 trace_on(tag_L);

		for(Index i=0;i<n;i++)
			xa[i] <<= xp[i];
		for(Index i=0;i<m;i++)
			lam[i] <<= 1.0;
		sig <<= 1.0;

		eval_obj(n,xa,obj_value);

		obj_value *= sig;
		eval_constraints(n,xa,m,g);

		for(Index i=0;i<m;i++)
			obj_value += g[i]*lam[i];

		obj_value >>= dummy;

	trace_off();

	delete[] xa;
	delete[] xp;
	delete[] g;
	delete[] lam;
	delete[] lamp;
	delete[] zu;
	delete[] zl;

}

