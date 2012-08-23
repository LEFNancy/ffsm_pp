/***************************************************************************
 *   Copyright (C) 2012 by Laboratoire d'Economie Forestière               *
 *   http://www.nancy.inra.fr/lef_eng                                      *
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
#ifndef STDOPT_H
#define STDOPT_H


#include "IpTNLP.hpp"
#include <adolc.h>

//regmas headers
#include "BaseClass.h"
#include "ThreadManager.h"
#include "ModelData.h"

#define tag_f 1
#define tag_g 2
#define tag_L 3



/// Class containing the optimization problem (the matrix and its methods)

/**

@author Antonello Lobianco
*/

using namespace Ipopt;

struct constrain;

class Opt: public BaseClass, public TNLP{
public:
						Opt(ThreadManager* MTHREAD_h); ///< Constructor
					   ~Opt();
	/**@name Overloaded from TNLP */
	//@{
	/** Method to return some info about the nlp */
	virtual bool        get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
									 Index& nnz_h_lag, IndexStyleEnum& index_style);
	/** Method to return the bounds for my problem */
	virtual bool        get_bounds_info(Index n, Number* x_l, Number* x_u,
										Index m, Number* g_l, Number* g_u);

	/** Method to return the starting point for the algorithm */
	virtual bool        get_starting_point(Index n, bool init_x, Number* x,
										   bool init_z, Number* z_L, Number* z_U,
										   Index m, bool init_lambda,
										   Number* lambda);
	/** Template to return the objective value */
	template<class T> bool eval_obj(Index n, const T *x, T& obj_value);

	/** Template to compute contraints */
	template<class T> bool eval_constraints(Index n, const T *x, Index m, T *g);

	/** Original method from Ipopt to return the objective value */
	/** remains unchanged */
	virtual bool        eval_f(Index n, const Number* x, bool new_x, Number& obj_value);

	/** Original method from Ipopt to return the gradient of the objective */
	/** remains unchanged */
	virtual bool        eval_grad_f(Index n, const Number* x, bool new_x, Number* grad_f);

	/**  Original method from Ipopt to return the constraint residuals */
	/** remains unchanged */
	virtual bool        eval_g(Index n, const Number* x, bool new_x, Index m, Number* g);

	/** Original method from Ipopt to return:
	 *   1) The structure of the jacobian (if "values" is NULL)
	*   2) The values of the jacobian (if "values" is not NULL)
	*/
	/** remains unchanged */
	virtual bool        eval_jac_g(Index n, const Number* x, bool new_x,
								   Index m, Index nele_jac, Index* iRow, Index *jCol,
								   Number* values);

	/** Original method from Ipopt to return:
	*   1) The structure of the hessian of the lagrangian (if "values" is NULL)
	*   2) The values of the hessian of the lagrangian (if "values" is not NULL)
	*/
	/** remains unchanged */
	virtual bool        eval_h(Index n, const Number* x, bool new_x,
							   Number obj_factor, Index m, const Number* lambda,
							   bool new_lambda, Index nele_hess, Index* iRow,
							   Index* jCol, Number* values);

	//@}

	/** @name Solution Methods */
	//@{
	/** This method is called when the algorithm is complete so the TNLP can store/write the solution */
	virtual void finalize_solution(SolverReturn status,
									 Index n, const Number* x, const Number* z_L, const Number* z_U,
									 Index m, const Number* g, const Number* lambda,
									 Number obj_value,
									 const IpoptData* ip_data,
									 IpoptCalculatedQuantities* ip_cq);
	//@}

	//***************    start ADOL-C part ***********************************

	/** Method to generate the required tapes */
	virtual void generate_tapes(Index n, Index m);

	//***************    end   ADOL-C part ***********************************

	// **************   start PYTHIA part ************************************
	void                declareVariables(); ///< declare the variables and their domains
	void                declareConstrains(); ///< declare the constrains, their domain, their direction and their associated evaluation function
	void                cacheInitialPosition(); ///< cache the initial positions of the variables and the constrains
	void                calculateNumberVariablesConstrains(); ///< calculate the number of variables and constrains
	void                cachePositions(); ///< cache the exact position index (initial+f(r1,r2,p,r2To) for each variable and constrain
	int                 getDomainElements(int domain); ///< return the number of elements of a domain
template<class T> vector < vector < vector < vector <int> > > > buildPositionVector(const T &v_or_c, int dType); ///< build the matrix of the positions for a given variable or contrain
template<class T> vector < vector < vector < vector <int> > > > buildPositionVector(const T &v_or_c, vector<string> &prVector);

///< build the matrix of the positions for a given variable or contrain
void                  calculateSparsityPatternJ();
void                  calculateSparsityPatternH();

	// **************   end PYTHIA part **************************************


	//************** start constrains (MODELLER PART) *******************************************
	template<class T> static void mkteq2f (const T *x, T *g, Index *cix);
	template<class T> static void mkteq3f (const T *x, T *g, Index *cix);

	//************** end constrains (MODELLER PART) *****************************************

protected:

	// convenient handles to equivalent ModelData functions..
	const double        gpd(const string &type_h, int regId_h, const string &prodId_h, int year=DATA_NOW, const string &freeDim_h=""){return MTHREAD->MD->getProdData(type_h, regId_h, prodId_h, year, freeDim_h);};
	const double        gfd(const string &type_h, int regId_h, const string &forType_h, const string &diamClass_h, int year=DATA_NOW){return MTHREAD->MD->getForData(type_h, regId_h, forType_h, diamClass_h, year);};
	void                spd(double value_h, const string &type_h, int regId_h, const string &prodId_h, int year=DATA_NOW, bool allowCreate=false, const string &freeDim_h=""){MTHREAD->MD->setProdData(value_h, type_h, regId_h, prodId_h, year, allowCreate, freeDim_h);};
	void                sfd(double value_h, const string &type_h, int regId_h, const string &forType_h, const string &diamClass_h, int year=DATA_NOW, bool allowCreate=false){MTHREAD->MD->setForData(value_h, type_h, regId_h, forType_h, diamClass_h, year, allowCreate);};
	bool                app(const string &prod_h, const string &forType_h, const string &dClass_h){return MTHREAD->MD->assessProdPossibility(prod_h, forType_h, dClass_h);};
	const int           gip(const string &varName); ///< Get the initial index position of a given variable in the concatenated array
	const int           gip(const int &cn); ///< Return the initial index position of a certain constrain
	template<class T> const int gix_uncached(const T &v_or_c, int r1Ix, int r2Ix, int prIx, int r2IxTo=0); ///< Get the index in the concatenated array gived a certain var name (string) or constrain index (int), the reg lev1 index, the reg lev2 index and the prod. index
	const int           gix(const string &varName, int r1Ix, int r2Ix, int prIx, int r2IxTo=0); ///< Get the index in the concatenated array gived a certain var name, the reg lev1 index, the reg lev2 index and the prod. index
	const int           gix(const int &cn, int r1Ix, int r2Ix, int prIx, int r2IxTo=0); ///< Get the index in the concatenated array gived a certain constrain, the reg lev1 index, the reg lev2 index and the prod. index
	const int           gdt(const string &varName); ///< Get the domain type of a given variable
	const int           gdt(const int &cn); ///< Get the domain type of a given constrain
	int                 getConstrainDirectionByIndex(int idx); ///< Return the direction of a given constrain
	constrain*          getConstrainByIndex(int idx);
	void                unpack(int ix_h, int domain, int initial, int &r1_h, int &r2_h, int&p_h, int&r2to_h, bool fullp=false); ///< Return the dimensions given a certain index, domain type and initial position
	int                 getConNumber(constrain* con); ///< Return the position in the cons vector


	void tempDebug();

	//virtual void        eval_obj (Index n, const T *x, T& obj_value);

	vector<string>                           priPr;
	vector<string>                           secPr;
	vector<string>                           allPr;
	vector < vector <int> >                    l2r;
	map <string, int>                      initPos; ///< A map that return the initial index position in the concatenated array for each variable
	vector<int>                           cInitPos; ///< A vector that return the initial index position in the concatenated array for each constrain
	map <string, int>                         vars; ///< List of variables in the model and their domain: pr product, sec prod, all products or all products over each subregion pair (exports)
	map <string, vector < vector < vector < vector <int> > > > > vpositions; ///< cached position in the concatenated vector for each variables. Dimensions are l1reg, l2reg, prod, (l2To region).
	vector < vector < vector < vector < vector <int> > > > > cpositions; ///< cached position in the concatenated vector for each variables. Dimensions are contrain number, l1reg, l2reg, prod, (l2To region).
	int                                     nPriPr;
	int                                     nSecPr;
	int                                     nAllPr;
	int                                       nL2r;
	int                                       nVar;
	int                                      nCons;
	int                        nEqualityConstrains;
	int                  nLowerEqualZeroConstrains;
	int                nGreaterEqualZeroConstrains;
	int                               previousYear;
	int                                  firstYear;
	int                                 secondYear;
	int                              worldCodeLev2;
	bool                              debugRunOnce;
	void                    debugPrintParameters();
	bool                                   initOpt;
	vector <constrain>                        cons;
	vector <vector <Index> >						 nzjelements; ///< nzero elements for the jacobian matrix. nzelements[i][0] -> row (constrain), nzelements[i][1] -> column (variable)
	vector <vector <Index> >						 nzhelements; ///< nzero elements for the hessian matrix
	//int                               nnzjelements; ///< number of non zero elements



	/**@name Methods to block default compiler methods.
	 * The compiler automatically generates the following three methods.
	 *  Since the default compiler implementation is generally not what
	 *  you want (for all but the most simple classes), we usually
	 *  put the declarations of these methods in the private section
	 *  and never implement them. This prevents the compiler from
	 *  implementing an incorrect "default" behavior without us
	 *  knowing. (See Scott Meyers book, "Effective C++")
	 *
	*/
	//@{
	//  MyADOLC_NLP();
	Opt(const Opt&);
	Opt& operator=(const Opt&);
	//@}

	//@{
	double **Jac;

	double *x_lam;
	double **Hess;
	//@}

};

struct constrain{
												constrain(){comment="";};
	string                                    name;
	string                                 comment;
	int                                     domain;
	int                                  direction;
	//std::function < template <class T> void (const T *, T *, Index *) >  evaluate; // std::function can't hold templates
};


#endif

