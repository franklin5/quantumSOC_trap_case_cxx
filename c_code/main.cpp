
static char help[] = "Solves the steady state of Master Equation.\n\n";

/*T
   Concepts: KSP^basic parallel example; Sparse matrix construction;
   Processors: n
   mpirun -n 6 SteadyState -ksp_monitor_short   -pc_type jacobi   -ksp_type gmres -ksp_gmres_restart 200
   or
   ./SteadyState -ksp_monitor_short   -pc_type jacobi   -ksp_type gmres -ksp_gmres_restart 200
T*/

/*
  Include "petscksp.h" so that we can use KSP solvers.  Note that this file
  automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h - vectors
     petscmat.h - matrices
     petscis.h     - index sets            petscksp.h - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h  - preconditioners

*/
#include <petscksp.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <math.h>
#include <cmath>
using namespace std;
const PetscInt __MAXNOZEROS__ = 1000; // This is the max number in a row. Need to check if it is large enough...
void initialize();
  void block(int i, int &r, int &m, int &n, int &p, int &q,int N, int Q);
//  void zombie_call_of_block(){block(i,r,c,m,n,p,q);}
  int compute_kt(int, int, int, int, int, int, int);
  PetscErrorCode construction();
  PetscErrorCode assemblance();
  PetscErrorCode seek_steady_state();
  PetscErrorCode viewMatrix();
  PetscErrorCode MatInsert(PetscScalar _val_, int &nonzeros, PetscInt* col, PetscScalar* value,
  		int ct, int mt, int nt, int pt, int qt,int N, int Q);
  PetscErrorCode observables_photon();
  PetscErrorCode observables_oscillator();
  void get_NQ(int &N, int &Q);
#undef __FUNCT__
#define __FUNCT__ "main"
#define root 0
int main(int argc,char **args){
  PetscErrorCode ierr;
  PetscInitialize(&argc,&args,(char*)0,help);
  ierr = PetscPrintf(PETSC_COMM_WORLD,
		     "======================================================================\n"
		     "The purpose of this program is to study the steady state solution of\n"
		     " quantum master equation, that governs the laser assisted SOC system.\n"
		     "Motivated, proposed, designed, implemented and researched \n"
		     "by Lin Dong at Rice University. \n"
		     "at "  __TIME__  ", on "  __DATE__  "\n"
		     "Petsc is initialized and program starts from \n"
		     __FILE__  "\n"
		     "======================================================================\n");CHKERRQ(ierr);
  	Vec            b,u;          /* RHS, test_exact solutions */
    Mat            G;                /* linear system matrix */
    KSP            ksp;              /* linear solver context */
    PC             pc;               /* preconditioner context */
    PetscReal      norm,tol;  /* norm of solution error */
    PetscViewer    viewer;
    PetscInt       ROW,COLUMN,m,n,p,q,k,r,c,its,rstart,rend,nlocal,col[__MAXNOZEROS__];
    PetscScalar    value[__MAXNOZEROS__], one, neg_one, val;
    PetscInt       N,Q,DIM;
    PetscScalar    omega, qr,Omega,delta,varepsilon,delta_c,kappa;
    Vec            x;          /* steady state solution */
    PetscMPIInt      rank, size;
    double    	PhotonNumber, PhotonFluc, tmpRhoDiagonal;
    MPI_Comm_size(PETSC_COMM_WORLD,&size);
    	MPI_Comm_rank(PETSC_COMM_WORLD,&rank);
    //	cout << rank << '\t' << size << endl;
    	for (int ig = 0; ig < size; ++ig) {
    	    if (ig ==rank){
    	    	char dummyname[100];
    	    	double dummyvalue;
    	    	int intdummyvalue;
    	        FILE *input;
    	        input = fopen("input.txt","r");
    	        assert(input != NULL);
    	        if (ig == 0)  cout << "Starting to read in parameters from file input.txt" << endl;
    	        fscanf(input,"%s %d", dummyname, &intdummyvalue);
    	        N = intdummyvalue;    if (ig == 0) cout << dummyname << "=" << N << endl;
    	        fscanf(input,"%s %d", dummyname, &intdummyvalue);
    	        Q = intdummyvalue;    if (ig == 0) cout << dummyname << "=" << Q << endl;
    	        fscanf(input,"%s %lf", dummyname, &dummyvalue);
    	        omega = dummyvalue;    if (ig == 0) cout << dummyname << "=" << omega << endl;
    	        fscanf(input,"%s %lf", dummyname, &dummyvalue);
    	        qr = dummyvalue*sqrt(omega);    if (ig == 0) cout << dummyname << "=" << qr << endl;
    	        fscanf(input,"%s %lf", dummyname, &dummyvalue);
    	        Omega = dummyvalue;    if (ig == 0) cout << dummyname << "=" << Omega << endl;
    	        fscanf(input,"%s %lf", dummyname, &dummyvalue);
    	        delta = dummyvalue;    if (ig == 0) cout << dummyname << "=" << delta << endl;
    	        fscanf(input,"%s %lf", dummyname, &dummyvalue);
    	        varepsilon = dummyvalue;    if (ig == 0) cout << dummyname << "=" << varepsilon << endl;
    	        fscanf(input,"%s %lf", dummyname, &dummyvalue);
    	        delta_c = dummyvalue;    if (ig == 0) cout << dummyname << "=" << delta_c << endl;
    	        fscanf(input,"%s %lf", dummyname, &dummyvalue);
    	        kappa = dummyvalue;    if (ig == 0) cout << dummyname << "=" << kappa << endl;
    	        fscanf(input,"%s %lf", dummyname, &dummyvalue);
    	        tol = dummyvalue;    if (ig == 0) cout << dummyname << "=" << tol << endl;
    	        fclose(input);
    	    }
    	}
    //  qr = 1;
    //  Omega = 1;
    //  delta = 4;
    //  varepsilon = 1;
    //  delta_c = 1;
    //  kappa = 1;
    //  N=2;
    //  Q=1;
    	//  tol=1.e-11;
    	DIM=4*(N+1)*(N+1)*(Q+1)*(Q+1);
      one =1.0;neg_one=-1.0;

//    construction();
      /*
          Create vectors.  Note that we form 1 vector from scratch and
          then duplicate as needed. For this simple case let PETSc decide how
          many elements of the vector are stored on each processor. The second
          argument to VecSetSizes() below causes PETSc to decide.
        */
        ierr = VecCreate(PETSC_COMM_WORLD,&x);CHKERRQ(ierr);
        ierr = VecSetSizes(x,PETSC_DECIDE,DIM);CHKERRQ(ierr);
        ierr = VecSetFromOptions(x);CHKERRQ(ierr);
        ierr = VecDuplicate(x,&b);CHKERRQ(ierr);
        ierr = VecDuplicate(x,&u);CHKERRQ(ierr);

        /* Identify the starting and ending mesh points on each
           processor for the interior part of the mesh. We let PETSc decide
           above. */

        ierr = VecGetOwnershipRange(x,&rstart,&rend);CHKERRQ(ierr);
        ierr = VecGetLocalSize(x,&nlocal);CHKERRQ(ierr);

        /*
          Create matrix.  When using MatCreate(), the matrix format can
          be specified at runtime.

          Performance tuning note:  For problems of substantial size,
          preallocation of matrix memory is crucial for attaining good
          performance. See the matrix chapter of the users manual for details.

          We pass in nlocal as the "local" size of the matrix to force it
          to have the same parallel layout as the vector created above.
        */
        ierr = MatCreate(PETSC_COMM_WORLD,&G);CHKERRQ(ierr);
        ierr = MatSetSizes(G,nlocal,nlocal,DIM,DIM);CHKERRQ(ierr);
        ierr = MatSetFromOptions(G);CHKERRQ(ierr);
        ierr = MatSetUp(G);CHKERRQ(ierr);

        /*
            Assemble matrix.

            The linear system is distributed across the processors by
            chunks of contiguous rows, which correspond to contiguous
            sections of the mesh on which the problem is discretized.
            For matrix assembly, each processor contributes entries for
            the part that it owns locally.
          */
          int nonzeros; // TODO: check if nonzeros < __MAXNOZEROS__ is true.
          int ct, mt, nt, pt, qt;
          int kt;
          PetscScalar _val_;
          for (ROW=rstart; ROW<rend; ROW++) {
        	  nonzeros = 0;
        	  block(ROW,r,m,n,p,q,N,Q);
            switch (r) {
            case 0:
            	// MUU block
            	ct = r; mt = m; nt = n; pt = p; qt = q;
        	_val_ = ((p+0.5)*omega+delta)/PETSC_i-((q+0.5)*omega+delta)/PETSC_i+PETSC_i*delta_c*(m-n)-kappa*(m+n);
        	MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	ct = r; mt = m; nt = n; pt = p+1; qt = q;
            	if (pt <= Q) {
        	  _val_ = -qr/sqrt(2)*sqrt(p+1);
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p-1; qt = q;
            	if (pt >= 0) {
        	  _val_ =qr/sqrt(2)*sqrt(p);
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p; qt = q+1;
            	if (qt <= Q) {
        	  _val_ = -qr/sqrt(2)*sqrt(q+1);
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p; qt = q-1;
            	if (qt >= 0) {
        	  _val_ = qr/sqrt(2)*sqrt(q);
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m+1; nt = n+1; pt = p; qt = q;
            	if (mt <= N && nt <= N) {
        	  _val_ = kappa*2*sqrt(m+1)*sqrt(n+1);
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m+1; nt = n; pt = p; qt = q;
            	if (mt <= N) {
        	  _val_ = -varepsilon*sqrt(m+1);
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m-1; nt = n; pt = p; qt = q;
            	if (mt >= 0) {
        	  _val_ = varepsilon*sqrt(m);
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n+1; pt = p; qt = q;
            	if (nt <= N) {
        	  _val_ = -varepsilon*sqrt(n+1);
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n-1; pt = p; qt = q;
            	if (nt >= 0) {
        	  _val_ = varepsilon*sqrt(n);
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	// S1 block
            	ct = 1;mt = m; nt = n+1; pt = p; qt = q;
            	if (nt <= N) {
        	  _val_ = -Omega/2*sqrt(n+1)/PETSC_i;
        	  // TODO: potential bugs for MatView with pure imaginary number display.
        	  //    		cout << compute_kt(ct,mt,nt,pt,qt) << '\t' << PetscAbsScalar(_val_) << endl;
        	  MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	// S2 block
            	ct = 2;mt = m+1; nt = n; pt = p; qt = q;
            	if (mt <= N) {
            		_val_ = Omega/2*sqrt(m+1)/PETSC_i;
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
                if (nonzeros > __MAXNOZEROS__){
                	cerr << "nonzeros on a row " <<  nonzeros << " is larger than the pre-allocated range of"
                	<<  __MAXNOZEROS__ <<" const arrays. Try increasing the max number in steady.h" << endl;exit(1);
                }
                ierr   = MatSetValues(G,1,&ROW,nonzeros,col,value,INSERT_VALUES);CHKERRQ(ierr);
            	break;
            case 1:
            	// MUD block
            	ct = r; mt = m; nt = n; pt = p; qt = q;
        			_val_ = ((p+0.5)*omega+delta)/PETSC_i-((q+0.5)*omega-delta)/PETSC_i+PETSC_i*delta_c*(m-n)-kappa*(m+n);
        			MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	ct = r; mt = m; nt = n; pt = p+1; qt = q;
            	if (pt <= Q) {
            		_val_ = -qr/sqrt(2)*sqrt(p+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p-1; qt = q;
            	if (pt >= 0) {
            		_val_ = qr/sqrt(2)*sqrt(p);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p; qt = q+1;
            	if (qt <= Q) {
            		_val_ = qr/sqrt(2)*sqrt(q+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p; qt = q-1;
            	if (qt >= 0) {
            		_val_ = -qr/sqrt(2)*sqrt(q);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m+1; nt = n+1; pt = p; qt = q;
            	if (mt <= N && nt <= N) {
            		_val_ = kappa*2*sqrt(m+1)*sqrt(n+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m+1; nt = n; pt = p; qt = q;
            	if (mt <= N) {
            		_val_ = -varepsilon*sqrt(m+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m-1; nt = n; pt = p; qt = q;
            	if (mt >= 0) {
            		_val_ = varepsilon*sqrt(m);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n+1; pt = p; qt = q;
            	if (nt <= N) {
            		_val_ = -varepsilon*sqrt(n+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n-1; pt = p; qt = q;
            	if (nt >= 0) {
            		_val_ = varepsilon*sqrt(n);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	// S3 block
            	ct = 0;mt = m; nt = n-1; pt = p; qt = q;
            	if (nt >= 0) {
            		_val_ = -Omega/2*sqrt(n)/PETSC_i;
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	// S4 block
            	ct = 3;mt = m+1; nt = n; pt = p; qt = q;
            	if (mt <= N) {
            		_val_ = Omega/2*sqrt(m+1)/PETSC_i;
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
                if (nonzeros > __MAXNOZEROS__){
                	cerr << "nonzeros on a row " <<  nonzeros << " is larger than the pre-allocated range of"
                	<<  __MAXNOZEROS__ <<" const arrays. Try increasing the max number in steady.h" << endl;exit(1);
                }
                ierr   = MatSetValues(G,1,&ROW,nonzeros,col,value,INSERT_VALUES);CHKERRQ(ierr);
            	break;
            case 2:
            	// MDU block
            	ct = r; mt = m; nt = n; pt = p; qt = q;
        			_val_ = ((p+0.5)*omega-delta)/PETSC_i-((q+0.5)*omega+delta)/PETSC_i+PETSC_i*delta_c*(m-n)-kappa*(m+n);
        			MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	ct = r; mt = m; nt = n; pt = p+1; qt = q;
            	if (pt <= Q) {
            		_val_ = qr/sqrt(2)*sqrt(p+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p-1; qt = q;
            	if (pt >= 0) {
            		_val_ = -qr/sqrt(2)*sqrt(p);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p; qt = q+1;
            	if (qt <= Q) {
            		_val_ = -qr/sqrt(2)*sqrt(q+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p; qt = q-1;
            	if (qt >= 0) {
            		_val_ =  qr/sqrt(2)*sqrt(q);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m+1; nt = n+1; pt = p; qt = q;
            	if (mt <= N && nt <= N) {
            		_val_ = kappa*2*sqrt(m+1)*sqrt(n+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m+1; nt = n; pt = p; qt = q;
            	if (mt <= N) {
            		_val_ = -varepsilon*sqrt(m+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m-1; nt = n; pt = p; qt = q;
            	if (mt >= 0) {
            		_val_ = varepsilon*sqrt(m);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n+1; pt = p; qt = q;
            	if (nt <= N) {
            		_val_ = -varepsilon*sqrt(n+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n-1; pt = p; qt = q;
            	if (nt >= 0) {
            		_val_ = varepsilon*sqrt(n);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	// S5 block
            	ct = 0;mt = m-1; nt = n; pt = p; qt = q;
            	if (mt >= 0) {
            		_val_ = Omega/2*sqrt(m)/PETSC_i;
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	// S6 block
            	ct = 3;mt = m; nt = n+1; pt = p; qt = q;
            	if (nt <= N) {
            		_val_ = -Omega/2*sqrt(n+1)/PETSC_i;
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
                if (nonzeros > __MAXNOZEROS__){
                	cerr << "nonzeros on a row " <<  nonzeros << " is larger than the pre-allocated range of"
                	<<  __MAXNOZEROS__ <<" const arrays. Try increasing the max number in steady.h" << endl;exit(1);
                }
                ierr   = MatSetValues(G,1,&ROW,nonzeros,col,value,INSERT_VALUES);CHKERRQ(ierr);
                break;
            case 3:
            	// MDD block
            	ct = r; mt = m; nt = n; pt = p; qt = q;
        			_val_ = ((p+0.5)*omega-delta)/PETSC_i-((q+0.5)*omega-delta)/PETSC_i+PETSC_i*delta_c*(m-n)-kappa*(m+n);
        			MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	ct = r; mt = m; nt = n; pt = p+1; qt = q;
            	if (pt <= Q) {
            		_val_ = qr/sqrt(2)*sqrt(p+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p-1; qt = q;
            	if (pt >= 0) {
            		_val_ = -qr/sqrt(2)*sqrt(p);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p; qt = q+1;
            	if (qt <= Q) {
            		_val_ = qr/sqrt(2)*sqrt(q+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n; pt = p; qt = q-1;
            	if (qt >= 0) {
            		_val_ = -qr/sqrt(2)*sqrt(q);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m+1; nt = n+1; pt = p; qt = q;
            	if (mt <= N && nt <= N) {
            		_val_ = kappa*2*sqrt(m+1)*sqrt(n+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m+1; nt = n; pt = p; qt = q;
            	if (mt <= N) {
            		_val_ = -varepsilon*sqrt(m+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m-1; nt = n; pt = p; qt = q;
            	if (mt >= 0) {
            		_val_ = varepsilon*sqrt(m);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n+1; pt = p; qt = q;
            	if (nt <= N) {
            		_val_ = -varepsilon*sqrt(n+1);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	ct = r; mt = m; nt = n-1; pt = p; qt = q;
            	if (nt >= 0) {
            		_val_ = varepsilon*sqrt(n);
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	// S7 block
            	ct = 1;mt = m-1; nt = n; pt = p; qt = q;
            	if (mt >= 0) {
            		_val_ = Omega/2*sqrt(m)/PETSC_i;
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
            	// S8 block
            	ct = 2;mt = m; nt = n-1; pt = p; qt = q;
            	if (nt >= 0) {
            		_val_ = -Omega/2*sqrt(n)/PETSC_i;
            		MatInsert(_val_, nonzeros, col, value, ct, mt, nt, pt, qt, N,  Q);
            	}
                if (nonzeros > __MAXNOZEROS__){
                	cerr << "nonzeros on a row " <<  nonzeros << " is larger than the pre-allocated range of"
                	<<  __MAXNOZEROS__ <<" const arrays. Try increasing the max number in steady.h" << endl;exit(1);
                }
                ierr   = MatSetValues(G,1,&ROW,nonzeros,col,value,INSERT_VALUES);CHKERRQ(ierr);
                break;
            default:
            	cerr << "Sub-block row index" << r << " is out of range from 0 to 3. Stopping now..." << endl;
            	exit(1);
            	break;
            }
          }

        ierr = MatAssemblyBegin(G,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(G,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

        // Modify matrix and impose Trace of \rho = 1 as a constraint explicitly.
        PetscInt ival; ival = 0;
        ierr = MatZeroRows(G, 1, &ival, 0.0, 0, 0);CHKERRQ(ierr); // This has to be done AFTER matrix final assembly by petsc
        if (rstart == 0) {
          int nonzeros = 0;
          for (m=0;m<=N;m++){
            n = m;
            for (p=0;p<=Q;p++){
      	q = p;
      	col[nonzeros] = compute_kt(0,m,n,p,q, N, Q); // rho_up_up
      	value[nonzeros] = 1.0;
      	nonzeros ++;
      	col[nonzeros] = compute_kt(3,m,n,p,q, N, Q); // rho_dn_dn
      	value[nonzeros] = 1.0;
      	nonzeros ++;
            }
          }
          ierr   = MatSetValues(G,1,&rstart,nonzeros,col,value,INSERT_VALUES);CHKERRQ(ierr);
        }
        /* Re-Assemble the matrix */
        ierr = MatAssemblyBegin(G,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(G,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
//      seek_steady_state();
        /*
            Set exact solution; then compute right-hand-side vector.
          */
        //  ierr = VecSet(u,one);CHKERRQ(ierr);
        //  ierr = MatMult(G,u,b);CHKERRQ(ierr);
          for (ROW=rstart;ROW<rend;ROW++){
            if (ROW==0) {
              val=1.0;
              ierr = VecSetValues(b,1,&ROW,&val, INSERT_VALUES);
            }
            else {
              val=0.0;
              ierr = VecSetValues(b,1,&ROW,&val, INSERT_VALUES);
            }
          }

          ierr =  VecAssemblyBegin(b); CHKERRQ(ierr);
          ierr =  VecAssemblyEnd(b); CHKERRQ(ierr);
        //  ierr = PetscViewerSetFormat(PETSC_VIEWER_STDOUT_WORLD,	PETSC_VIEWER_ASCII_MATLAB  );CHKERRQ(ierr);
        //  ierr = VecView(b,	PETSC_VIEWER_STDOUT_WORLD );CHKERRQ(ierr);
          /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                         Create the linear solver and set various options
              - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
           /*
              Create linear solver context
           */
           ierr = KSPCreate(PETSC_COMM_WORLD,&ksp);CHKERRQ(ierr);

           /*
              Set operators. Here the matrix that defines the linear system
              also serves as the preconditioning matrix.
           */
           ierr = KSPSetOperators(ksp,G ,G);CHKERRQ(ierr);
           /*
               Set linear solver defaults for this problem (optional).
               - By extracting the KSP and PC contexts from the KSP context,
                 we can then directly call any KSP and PC routines to set
                 various options.
               - The following four statements are optional; all of these
                 parameters could alternatively be specified at runtime via
                 KSPSetFromOptions();
            */
        //    ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
        //    ierr = PCSetType(pc,PCJACOBI);CHKERRQ(ierr);
            ierr = KSPSetTolerances(ksp,tol,PETSC_DEFAULT,PETSC_DEFAULT,PETSC_DEFAULT);CHKERRQ(ierr);

            /*
              Set runtime options, e.g.,
                  -ksp_type <type> -pc_type <type> -ksp_monitor -ksp_rtol <rtol>
              These options will override those specified above as long as
              KSPSetFromOptions() is called _after_ any other customization
              routines.
            */
            ierr = KSPSetFromOptions(ksp);CHKERRQ(ierr);

            /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                Solve the linear system
               - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
            /*
               Solve linear system
            */
        //    KSPSetInitialGuessNonzero(ksp,PETSC_TRUE);
            ierr = KSPSolve(ksp,b,x);CHKERRQ(ierr);

            /*
               View solver info; we could instead use the option -ksp_view to
               print this info to the screen at the conclusion of KSPSolve().
            */
            ierr = KSPView(ksp,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
            /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                 Check solution and clean up
                - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
             /*
                Check the error
             */
        //    ierr = VecView(x,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
        //     ierr = VecAXPY(x,neg_one,u);CHKERRQ(ierr);
        //     ierr = VecNorm(x,NORM_2,&norm);CHKERRQ(ierr);
        //     ierr = KSPGetIterationNumber(ksp,&its);CHKERRQ(ierr);
        //       if (norm > tol) {
        //       ierr = PetscPrintf(PETSC_COMM_WORLD,"Norm of error %g, Iterations %D\n",(double)norm,its);CHKERRQ(ierr);
        //       }

//      viewMatrix();
            // Runtime option using database keys:  -mat_view draw -draw_pause -1

            //	ierr = PetscViewerSetFormat(PETSC_VIEWER_STDOUT_WORLD,	PETSC_VIEWER_ASCII_DENSE  );CHKERRQ(ierr);
            //  ierr = PetscViewerSetFormat(PETSC_VIEWER_STDOUT_WORLD,	PETSC_VIEWER_ASCII_MATLAB  );CHKERRQ(ierr);
            //  ierr = MatView(G,	PETSC_VIEWER_STDOUT_WORLD );CHKERRQ(ierr);

            //Vec tmpu;
            //ierr = VecDuplicate(x,&tmpu);CHKERRQ(ierr);
            //ierr = MatMult(G,b,tmpu);CHKERRQ(ierr);
            //ierr = VecView(tmpu,	PETSC_VIEWER_STDOUT_WORLD );CHKERRQ(ierr);

            //  PetscViewerDrawOpen(PETSC_COMM_WORLD,0,"",300,0,300,300,&viewer);
            //	  ierr = MatView(G,	viewer );CHKERRQ(ierr);
            //	  ierr = VecView(b,	PETSC_VIEWER_STDOUT_WORLD );CHKERRQ(ierr);
            //    ierr = VecView(x,	PETSC_VIEWER_STDOUT_WORLD );CHKERRQ(ierr);
//      observables_photon();
             nonzeros = 0;double phtn_n_r, phtn_fluc_r, tmpdiagrho;
            	cout.precision(16);
            	phtn_n_r = 0;phtn_fluc_r = 0;tmpdiagrho = 0;
            	for (ROW=rstart;ROW<rend;ROW++){
            		block(ROW, r, m, n, p, q,N,Q);
            		if (r==0 || r==3){ // Getting diagonal elements of rho_up_up and rho_dn_dn for all photon and orbital numbers
            			if (m==n && p==q){
            				ierr = VecGetValues(x,1,&ROW,&value[nonzeros]);CHKERRQ(ierr);
            				col[nonzeros] = m; // saved for photon number computation
            //				cout << value[nonzeros] << '\t' << r+1 << '\t' << m << '\t' << p << endl; //
            //				if (PetscImaginaryPart(value[nonzeros]) > 1.e-5) {
            //					cerr << "check the convergence, the imaginary part is intolerably large, stopping now... " << endl;
            //					exit(1);
            //				}
            //				tmpdiagrho += PetscRealPart(value[nonzeros]);
            				phtn_n_r += PetscRealPart(value[nonzeros])*m; // checked the imaginary part is exceedingly small as it should be.
            				phtn_fluc_r += PetscRealPart(value[nonzeros])*m*m;
            				nonzeros++;
            			}
            		}
            	}
            //	cout << "rank " << rank << " has photon number: " << phtn_n_r << " photon fluc: " << phtn_fluc_r << endl;
            	MPI_Reduce(&phtn_n_r, &PhotonNumber, 1, MPI_DOUBLE, MPI_SUM, 0, PETSC_COMM_WORLD);
            	MPI_Reduce(&phtn_fluc_r, &PhotonFluc, 1, MPI_DOUBLE, MPI_SUM, 0, PETSC_COMM_WORLD);
            //	MPI_Reduce(&tmpdiagrho, &tmpRhoDiagonal, 1, MPI_DOUBLE, MPI_SUM, 0, PETSC_COMM_WORLD);
            	if (rank == 0) {
            		PhotonFluc = (PhotonFluc - PhotonNumber*PhotonNumber)/PhotonNumber; // normalization to 1 for coherent state at root
            	cout << "photon number is " << PhotonNumber  << '\t'
            			<< "photon number fluctuation is " << PhotonFluc << endl;
            //	cout << "sum of diag " << tmpRhoDiagonal << endl;
            	}

            //	for (int jtmp = 0; jtmp < size; ++jtmp) {
            //		if (jtmp == rank) {
            //			for (int itmp = 0; itmp < nonzeros; itmp++) {
            ////				if (col[itmp] !=0){
            //					cout << "rank " << rank << " has " << value[itmp] << '\t' << "and photon number is " << col[itmp] << endl;
            //			}
            //		}
            //	}
//      observables_oscillator();
            	 nonzeros = 0;
            		cout.precision(16);
            		phtn_n_r = 0;phtn_fluc_r = 0;tmpdiagrho = 0;
            		for (ROW=rstart;ROW<rend;ROW++){
            			block(ROW, r, m, n, p, q,N,Q);
            			if (r==0 || r==3){ // Getting diagonal elements of rho_up_up and rho_dn_dn for all photon and orbital numbers
            				if (m==n && p==q){
            					ierr = VecGetValues(x,1,&ROW,&value[nonzeros]);CHKERRQ(ierr);
            					col[nonzeros] = p; // saved for average oscillator number computation
            	//				cout << value[nonzeros] << '\t' << r+1 << '\t' << m << '\t' << p << endl; //
            	//				if (PetscImaginaryPart(value[nonzeros]) > 1.e-5) {
            	//					cerr << "check the convergence, the imaginary part is intolerably large, stopping now... " << endl;
            	//					exit(1);
            	//				}
            	//				tmpdiagrho += PetscRealPart(value[nonzeros]);
            					phtn_n_r += PetscRealPart(value[nonzeros])*p; // checked the imaginary part is exceedingly small as it should be.
            					phtn_fluc_r += PetscRealPart(value[nonzeros])*p*p;
            					nonzeros++;
            				}
            			}
            		}
            	//	cout << "rank " << rank << " has photon number: " << phtn_n_r << " photon fluc: " << phtn_fluc_r << endl;
            		MPI_Reduce(&phtn_n_r, &PhotonNumber, 1, MPI_DOUBLE, MPI_SUM, 0, PETSC_COMM_WORLD);
            		MPI_Reduce(&phtn_fluc_r, &PhotonFluc, 1, MPI_DOUBLE, MPI_SUM, 0, PETSC_COMM_WORLD);
            	//	MPI_Reduce(&tmpdiagrho, &tmpRhoDiagonal, 1, MPI_DOUBLE, MPI_SUM, 0, PETSC_COMM_WORLD);
            		if (rank == 0) {
            			PhotonFluc = (PhotonFluc - PhotonNumber*PhotonNumber)/PhotonNumber; // normalization to 1 for coherent state at root
            		cout << "orbital number is " << PhotonNumber  << '\t'
            				<< "orbital number fluctuation is " << PhotonFluc << endl;
            	//	cout << "sum of diag " << tmpRhoDiagonal << endl;
            		}

            	//	for (int jtmp = 0; jtmp < size; ++jtmp) {
            	//		if (jtmp == rank) {
            	//			for (int itmp = 0; itmp < nonzeros; itmp++) {
            	////				if (col[itmp] !=0){
            	//					cout << "rank " << rank << " has " << value[itmp] << '\t' << "and photon number is " << col[itmp] << endl;
            	//			}
            	//		}
            	//	}

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
           Compute the matrix and right-hand-side vector that define
           the linear system, Gx = b.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /*
     Always call PetscFinalize() before exiting a program.  This routine
       - finalizes the PETSc libraries as well as MPI
       - provides summary and diagnostic information if certain runtime
         options are chosen (e.g., -log_summary).
  */

  ierr = PetscFinalize();
  return 0;
}


PetscErrorCode MatInsert(PetscScalar _val_, int &nonzeros, PetscInt* col, PetscScalar* value,
					int ct, int mt, int nt, int pt, int qt,int N, int Q){
  if (PetscAbsScalar(_val_) != 0 ) {
    col[nonzeros] = compute_kt(ct,mt,nt,pt,qt, N, Q);
    value[nonzeros] = _val_;
    nonzeros ++;
  }
}


int compute_kt(int ct, int mt, int nt, int pt, int qt, int N, int Q){
  return ct*(N+1)*(N+1)*(Q+1)*(Q+1)+mt*(N+1)*(Q+1)*(Q+1)+nt*(Q+1)*(Q+1)+pt*(Q+1)+qt;
}

void block(int irow, int&r, int&m, int&n, int&p, int&q,int N, int Q){
  /*
    Decompose a given global row/column index into block and sub-block indices.
    Based on the formulation we have from Master equation, we have the knowledge of dense construction,
    which has been tested in MATLAB format. Scaling up, we need to deploy sparse format and thus, a map
    that links the correspondance between the two.
  */
  int k;
  r = floor((irow)/((N+1)*(N+1)*(Q+1)*(Q+1)));
  k = irow-r*(N+1)*(N+1)*(Q+1)*(Q+1);
  m = floor((k)/((N+1)*(Q+1)*(Q+1)));
  n = floor((k-m*((N+1)*(Q+1)*(Q+1)))/((Q+1)*(Q+1)));
  p = floor((k-m*((N+1)*(Q+1)*(Q+1))-n*(Q+1)*(Q+1))/(Q+1));
  q = k-(m*(N+1)*(Q+1)*(Q+1)+n*(Q+1)*(Q+1)+p*(Q+1));
}