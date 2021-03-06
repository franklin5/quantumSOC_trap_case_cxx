#ifndef STEADY_H_
#define STEADY_H_
#include <petscksp.h>
#include <slepceps.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <math.h>
#include <cmath> 
//#include <ctgmath>
#include "obs.h"
using namespace std;
const PetscInt __MAXNOZEROS__ = 23; // This is the max number in a row --> theoretically largest recursion relation index given by the master equation.

class cMasterMatrix{
	friend class cMasterObservables;
private:
  Vec            b,u;          /* RHS, test_exact solutions */
  Mat            G;                /* linear system matrix */
  KSP            ksp;              /* linear solver context */
  PC             pc;               /* preconditioner context */
  PetscReal      norm,tol;  /* norm of solution error */
  PetscViewer    viewer;
  PetscInt       ROW,COLUMN,m,n,p,q,k,r,c,its,rstart,rend,nlocal,col[__MAXNOZEROS__];
  PetscScalar    value[__MAXNOZEROS__], one, neg_one, val;
protected:
  PetscErrorCode ierr;
  PetscInt       N,Q,DIM,tDIM1,tDIM2,tDIM3,tDIM4;
  PetscScalar    omega, qr,Omega,delta,varepsilon,delta_c,kappa;
  Vec            x;          /* steady state solution */
  PetscMPIInt      rank, size;
  double    	PhotonNumber, PhotonFluc, tmpRhoDiagonal;
public:
  cMasterMatrix(){}
  ~cMasterMatrix(){}
  PetscErrorCode destruction();
  void initialize();
  void block(int i, int &r, int &m, int &n, int &p, int &q);
  PetscErrorCode construction();
  PetscErrorCode assemblance();
  PetscErrorCode seek_steady_state();
  PetscErrorCode viewMatrix();
};
#endif
