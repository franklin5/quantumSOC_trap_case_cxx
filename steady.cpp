
static char help[] = "Solves a tridiagonal linear system.\n\n";

/*T
   Concepts: KSP^basic parallel example;
   Processors: n
T*/

/*
  Include "petscksp.h" so that we can use KSP solvers.  Note that this file
  automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h - vectors
     petscmat.h - matrices
     petscis.h     - index sets            petscksp.h - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h  - preconditioners

  Note:  The corresponding uniprocessor example is ex1.c
*/
#include <petscksp.h>
#include <iostream>
using namespace std;
#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Vec            x, b, u;          /* approx solution, RHS, exact solution */
  Mat            A;                /* linear system matrix */
  KSP            ksp;              /* linear solver context */
  PC             pc;               /* preconditioner context */
  PetscReal      norm,tol=1.e-11;  /* norm of solution error */
  PetscErrorCode ierr;
  PetscInt       i,j,n = 6,col[3],its,Istart,Iend,nlocal;
  PetscScalar    neg_one = -1.0,one = 1.0,value[3];
//  PetscViewer    viewer;
  PetscInitialize(&argc,&args,(char*)0,help);
  ierr = PetscOptionsGetInt(NULL,"-n",&n,NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
         Compute the matrix and right-hand-side vector that define
         the linear system, Ax = b.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /*
     Create matrix.  When using MatCreate(), the matrix format can
     be specified at runtime.

     Performance tuning note:  For problems of substantial size,
     preallocation of matrix memory is crucial for attaining good
     performance. See the matrix chapter of the users manual for details.

     We pass in nlocal as the "local" size of the matrix to force it
     to have the same parallel layout as the vector created above.
  */
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,n,n);CHKERRQ(ierr); // TODO: This is good, but I don't quite understand the meaning of "number of local rows/columns" here. Does this mean the matrix A is locally nlocal by nlocal?
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);

  MatGetOwnershipRange(A,&Istart,&Iend);
  /*
     Assemble matrix.

     The linear system is distributed across the processors by
     chunks of contiguous rows, which correspond to contiguous
     sections of the mesh on which the problem is discretized.
     For matrix assembly, each processor contributes entries for
     the part that it owns locally.
  */


//  if (!rstart) {
//    rstart = 1;
//    i      = 0; col[0] = 0; col[1] = 1; value[0] = 2.0; value[1] = -1.0;
//    ierr   = MatSetValues(A,1,&i,2,col,value,INSERT_VALUES);CHKERRQ(ierr);
//  }
//  if (rend == n) {
//    rend = n-1;
//    i    = n-1; col[0] = n-2; col[1] = n-1; value[0] = -1.0; value[1] = 2.0;
//    ierr = MatSetValues(A,1,&i,2,col,value,INSERT_VALUES);CHKERRQ(ierr);
//  }
//
//  /* Set entries corresponding to the mesh interior */
//  value[0] = -1.0; value[1] = 2.0; value[2] = -1.0;
//  for (i=rstart; i<rend; i++) {
//    col[0] = i-1; col[1] = i; col[2] = i+1;
//    ierr   = MatSetValues(A,1,&i,3,col,value,INSERT_VALUES);CHKERRQ(ierr);
//  }

  for (i=Istart; i<Iend; i++) {
	  if (i==0){
		  j = 2;
		  col[0] = 0; col[1] = 4;
		  value[0] = 10; value[1] = -2;
		  ierr   = MatSetValues(A,1,&i,j,col,value,INSERT_VALUES);CHKERRQ(ierr);
	  }
	  else if (i==1){
		  j = 3;
		  col[0] = 0; col[1] = 1; col[2] = 5;
		  value[0] = 3; value[1] = 9; value[2] = 3;
		  ierr   = MatSetValues(A,1,&i,j,col,value,INSERT_VALUES);CHKERRQ(ierr);
	  }
	  else if (i==2){
		  j = 3;
		  col[0] = 1; col[1] = 2; col[2] = 3;
		  value[0] = 7; value[1] = 8; value[2] = 7;
		  ierr   = MatSetValues(A,1,&i,j,col,value,INSERT_VALUES);CHKERRQ(ierr);
	  }
	  else if (i==3){
		  j = 4;
		  col[0] = 0; col[1] = 2; col[2] = 3; col[3] = 4;
		  value[0] = 3; value[1] = 8; value[2] = 7; value[3] = 5;
		  ierr   = MatSetValues(A,1,&i,j,col,value,INSERT_VALUES);CHKERRQ(ierr);
	  }
	  else if (i==4){
		  j = 4;
		  col[0] = 1; col[1] = 3; col[2] = 4; col[3] = 5;
		  value[0] = 8; value[1] = 9; value[2] = 9; value[3] = 13;
		  ierr   = MatSetValues(A,1,&i,j,col,value,INSERT_VALUES);CHKERRQ(ierr);
	  }
	  else if (i==5){
		  j = 3;
		  col[0] = 1; col[1] = 4; col[2] = 5;
		  value[0] = 4; value[1] = 2; value[2] = -1;
		  ierr   = MatSetValues(A,1,&i,j,col,value,INSERT_VALUES);CHKERRQ(ierr);
	  }
  }

  /* Assemble the matrix */
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);


  /*
     Create vectors.  Note that we form 1 vector from scratch and
     then duplicate as needed. For this simple case let PETSc decide how
     many elements of the vector are stored on each processor. The second
     argument to VecSetSizes() below causes PETSc to decide.
  */
  ierr = VecCreate(PETSC_COMM_WORLD,&x);CHKERRQ(ierr);
  ierr = VecSetSizes(x,PETSC_DECIDE,n);CHKERRQ(ierr);
  ierr = VecSetFromOptions(x);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&b);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&u);CHKERRQ(ierr);

  /*
     Set exact solution; then compute right-hand-side vector.
  */
  ierr = VecSet(u,one);CHKERRQ(ierr);
  ierr = MatMult(A,u,b);CHKERRQ(ierr);
//  ierr = PetscViewerASCIIOpen(PETSC_COMM_WORLD, "matrixA.data", 	&viewer );CHKERRQ(ierr);
//  ierr = VecView(u,viewer);CHKERRQ(ierr);
  ierr = PetscViewerSetFormat(PETSC_VIEWER_STDOUT_WORLD,PETSC_VIEWER_ASCII_DENSE  );CHKERRQ(ierr);
  ierr = MatView(A,	PETSC_VIEWER_STDOUT_WORLD );CHKERRQ(ierr);
//  ierr = VecView(b,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
//
//  PetscViewerDestroy(&viewer);
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
  ierr = KSPSetOperators(ksp,A,A);CHKERRQ(ierr);

  /*
     Set linear solver defaults for this problem (optional).
     - By extracting the KSP and PC contexts from the KSP context,
       we can then directly call any KSP and PC routines to set
       various options.
     - The following four statements are optional; all of these
       parameters could alternatively be specified at runtime via
       KSPSetFromOptions();
  */
  ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
  ierr = PCSetType(pc,PCJACOBI);CHKERRQ(ierr);
  ierr = KSPSetTolerances(ksp,1.e-7,PETSC_DEFAULT,PETSC_DEFAULT,PETSC_DEFAULT);CHKERRQ(ierr);

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
  ierr = VecAXPY(x,neg_one,u);CHKERRQ(ierr);
  ierr = VecNorm(x,NORM_2,&norm);CHKERRQ(ierr);
  ierr = KSPGetIterationNumber(ksp,&its);CHKERRQ(ierr);
  if (norm > tol) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Norm of error %g, Iterations %D\n",(double)norm,its);CHKERRQ(ierr);
  }

  /*
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
  */
  ierr = VecDestroy(&x);CHKERRQ(ierr); ierr = VecDestroy(&u);CHKERRQ(ierr);
  ierr = VecDestroy(&b);CHKERRQ(ierr); ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = KSPDestroy(&ksp);CHKERRQ(ierr);

  /*
     Always call PetscFinalize() before exiting a program.  This routine
       - finalizes the PETSc libraries as well as MPI
       - provides summary and diagnostic information if certain runtime
         options are chosen (e.g., -log_summary).
  */
  ierr = PetscFinalize();
  return 0;
}
