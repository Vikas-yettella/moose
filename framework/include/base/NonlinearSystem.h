/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/

#ifndef NONLINEARSYSTEM_H
#define NONLINEARSYSTEM_H

#include "SystemBase.h"
#include "KernelWarehouse.h"
#include "BCWarehouse.h"
#include "DiracKernelWarehouse.h"
#include "DGKernelWarehouse.h"
#include "DamperWarehouse.h"
#include "ConstraintWarehouse.h"
#include "MooseException.h"
#include "MoosePreconditioner.h"

// libMesh includes
#include "transient_system.h"
#include "nonlinear_implicit_system.h"
#include "numeric_vector.h"
#include "sparse_matrix.h"
#include "petsc_matrix.h"
#include "coupling_matrix.h"

class FEProblem;
class TimeScheme;

/**
 * Nonlinear system to be solved
 *
 * It is a part of FEProblem ;-)
 */
class NonlinearSystem : public SystemTempl<TransientNonlinearImplicitSystem>
{
public:
  NonlinearSystem(FEProblem & problem, const std::string & name);
  virtual ~NonlinearSystem();

  virtual void init();
  virtual void solve();
  virtual void restoreSolutions();

  /**
   * Returns true if this system is currently computing the initial residual for a solve.
   * @return Whether or not we are currently computing the initial residual.
   */
  virtual bool computingInitialResidual() { return _computing_initial_residual; }

  // Setup Functions ////
  virtual void initialSetup();
  virtual void initialSetupBCs();
  virtual void initialSetupKernels();
  virtual void timestepSetup();

  void setupFiniteDifferencedPreconditioner();

  /**
   * Returns the convergence state
   * @return true if converged, otherwise false
   */
  virtual bool converged();

  /**
   * Adds a kernel
   * @param kernel_name The type of the kernel
   * @param name The name of the kernel
   * @param parameters Kernel parameters
   */
  void addKernel(const std::string & kernel_name, const std::string & name, InputParameters parameters);

  /**
   * Adds a scalar kernel
   * @param kernel_name The type of the kernel
   * @param name The name of the kernel
   * @param parameters Kernel parameters
   */
  void addScalarKernel(const std::string & kernel_name, const std::string & name, InputParameters parameters);

  /**
   * Adds a boundary condition
   * @param bc_name The type of the boundary condition
   * @param name The name of the boundary condition
   * @param parameters Boundary condition parameters
   */
  void addBoundaryCondition(const std::string & bc_name, const std::string & name, InputParameters parameters);

  /**
   * Adds a Constraint
   * @param bc_name The type of the boundary condition
   * @param name The name of the boundary condition
   * @param parameters Boundary condition parameters
   */
  void addConstraint(const std::string & c_name, const std::string & name, InputParameters parameters);

  /**
   * Adds a Dirac kernal
   * @param kernel_name The type of the dirac kernel
   * @param name The name of the Dirac kernel
   * @param parameters Dirac kernel parameters
   */
  void addDiracKernel(const std::string & kernel_name, const std::string & name, InputParameters parameters);

  /**
   * Adds a DG kernal
   * @param dg_kernel_name The type of the DG kernel
   * @param name The name of the DG kernel
   * @param parameters DG kernel parameters
   */
  void addDGKernel(std::string dg_kernel_name, const std::string & name, InputParameters parameters);

  /**
   * Adds a damper
   * @param damper_name The type of the damper
   * @param name The name of the damper
   * @param parameters Damper parameters
   */
  void addDamper(const std::string & damper_name, const std::string & name, InputParameters parameters);

  /**
   * Adds a solution length vector to the system.
   *
   * @param vector_name The name of the vector.
   * @param project Whether or not to project this vector when doing mesh refinement.
   *                If the vector is just going to be recomputed then there is no need to project it.
   * @param type What type of parallel vector.  This is usually either PARALLEL or GHOSTED.
   *                                            GHOSTED is needed if you are going to be accessing off-processor entries.
   *                                            The ghosting pattern is the same as the solution vector.
   * @param zero_for_residual Whether or not to zero this vector at the beginning of computeResidual.  Useful when
   *                          you are going to accumulate something into this vector during computeResidual
   */
  void addVector(const std::string & vector_name, const bool project, const ParallelType type, bool zero_for_residual);

  void setInitialSolution();

  /**
   * Sets the value of constrained variables in the solution vector.
   */
  void setConstraintSlaveValues(NumericVector<Number> & solution, bool displaced);

  /**
   * Modify the initial solution vector to apply a predictor
   * @param initial_solution The initial solution vector
   */
  void applyPredictor(NumericVector<Number> & initial_solution);

  /**
   * Add residual contributions from Constraints
   *
   * @param displaced Controls whether to do the displaced Constraints or non-displaced
   */
  void constraintResiduals(NumericVector<Number> & residual, bool displaced);

  /**
   * Computes residual
   * @param residual Residual is formed in here
   */
  void computeResidual(NumericVector<Number> & residual);

  /**
   * For computing all of little f given a big F. Currently it saves the current solution,
   * sets the current solution to bigF, computes little f.
   *
   * The M matrix is not gotten or inverted and applied currently.
   * TODO: Retrieve M matrix, invert, apply.
   */
  void computeLittlef(const NumericVector<Number> & bigF, NumericVector<Number> & littlef, Real time = -1, bool mass = true);

  /**
   * Finds the implicit sparsity graph between geometrically related dofs.
   */
  void findImplicitGeometricCouplingEntries(GeometricSearchData & geom_search_data, std::map<unsigned int, std::vector<unsigned int> > & graph);

  /**
   * Adds entries to the Jacobian in the correct positions for couplings coming from dofs being coupled that
   * are related geometrically (ie near eachother across a gap).
   */
  void addImplicitGeometricCouplingEntries(SparseMatrix<Number> & jacobian, GeometricSearchData & geom_search_data);

  /**
   * Add jacobian contributions from Constraints
   *
   * @param displaced Controls whether to do the displaced Constraints or non-displaced
   */
  void constraintJacobians(SparseMatrix<Number> & jacobian, bool displaced);

  /**
   * Computes Jacobian
   * @param jacobian Jacobian is formed in here
   */
  void computeJacobian(SparseMatrix<Number> &  jacobian);
  /**
   * Computes a Jacobian block. Used by Physics-based preconditioning
   * @param jacobian Where the block is stored
   * @param precond_system libMesh system that is used for the block Jacobian
   * @param ivar number of i-th variable
   * @param jvar number of j-th variable
   */
  void computeJacobianBlock(SparseMatrix<Number> & jacobian, libMesh::System & precond_system, unsigned int ivar, unsigned int jvar);

  /**
   * Compute damping
   * @param update
   * @return returns The damping factor
   */
  Real computeDamping(const NumericVector<Number>& update);

  /**
   * Print the L2-norm of variable residuals
   */
  void printVarNorms();

  /**
   * Sets the time-stepping scheme
   * @param scheme Time-stepping scheme to be set
   */
  void timeSteppingScheme(Moose::TimeSteppingScheme scheme);

  /**
   * Gets the time-stepping scheme
   * @return Time-stepping scheme being used
   */
  Moose::TimeSteppingScheme timeSteppingScheme() { return _time_stepping_scheme; }

  /**
   * Get the order of used time integration scheme
   */
  Real getTimeSteppingOrder() { return _time_stepping_order; }

  /**
   * Called at the beginning of th time step
   */
  void onTimestepBegin();

  /**
   * Called from assembling when we hit a new subdomain
   * @param subdomain ID of the new subdomain
   * @param tid Thread ID
   */
  virtual void subdomainSetup(unsigned int subdomain, THREAD_ID tid);

  virtual void set_solution(const NumericVector<Number> & soln);

  virtual NumericVector<Number> & solutionUDot();
  virtual NumericVector<Number> & solutionDuDotDu();

  virtual const NumericVector<Number> * & currentSolution() { return _current_solution; }

  virtual void serializeSolution();
  virtual NumericVector<Number> & serializedSolution();

  virtual NumericVector<Number> & residualCopy();
  virtual NumericVector<Number> & residualGhosted();

  virtual void augmentSendList(std::vector<unsigned int> & send_list);

  /**
   * computes the residual of all the kernels except for the time kernels for the purpose
   * of solving the time ODE M(dU/dt) = f(U,t)
   * @param residual The residual which is f(U,t)
   */
  virtual void computeNonTimeResidual(NumericVector<Number> & residual);
  virtual void computeTimeResidual(NumericVector<Number> & mmmatrix);

  virtual void augmentSparsity(SparsityPattern::Graph & sparsity,
                               std::vector<unsigned int> & n_nz,
                               std::vector<unsigned int> & n_oz);

  /**
   * Sets a preconditioner
   * @param pc The preconditioner to be set
   */
  void setPreconditioner(MoosePreconditioner *pc);

  /**
   * If called with true this system will use a finite differenced form of
   * the Jacobian as the preconditioner
   */
  void useFiniteDifferencedPreconditioner(bool use=true) { _use_finite_differenced_preconditioner = use; }

  /**
   * If called with true this will add entries into the jacobian to link together degrees of freedom that are found to
   * be related through the geometric search system.
   *
   * These entries are really only used by the Finite Difference Preconditioner right now.
   */
  void addImplicitGeometricCouplingEntriesToJacobian(bool add=true) { _add_implicit_geometric_coupling_entries_to_jacobian = add; }

  /**
   * Setup damping stuff (called before we actually start)
   */
  void setupDampers();
  /**
   * Reinit dampers. Called before we use damping
   * @param tid Thread ID
   */
  void reinitDampers(THREAD_ID tid);

  /// System Integrity Checks
  void checkKernelCoverage(const std::set<SubdomainID> & mesh_subdomains) const;
  void checkBCCoverage() const;
  bool containsTimeKernel();

  /**
   * Return the number of non-linear iterations
   */
  unsigned int nNonlinearIterations() { return _n_iters; }

  /**
   * Returns the current nonlinear iteration number.  In libmesh, this is
   * updated during the nonlinear solve, so it should be up-to-date.
   */
  unsigned int getCurrentNonlinearIterationNumber() { return _sys.get_current_nonlinear_iteration_number(); }

  /**
   * Return the number of linear iterations
   */
  unsigned int nLinearIterations() { return _n_linear_iters; }

  /**
   * Return the final nonlinear residual
   */
  Real finalNonlinearResidual() { return _final_residual; }

  /**
   * Print n top residuals with variable name and node number
   * @param residual The residual we work with
   * @param n The number of residuals to print
   */
  void printTopResiduals(const NumericVector<Number> & residual, unsigned int n);

  void debuggingResiduals(bool state) { _debugging_residuals = state; }

  void setPredictorScale(Real scale)
  {
    _use_predictor = true;
    _predictor_scale = scale;
  }

public:
  FEProblem & _fe_problem;
  // FIXME: make these protected and create getters/setters
  Real _last_rnorm;
  Real _last_nl_rnorm;
  Real _l_abs_step_tol;
  Real _initial_residual;
  std::vector<unsigned int> _current_l_its;
  unsigned int _current_nl_its;

protected:
  /**
   * Computes the time derivative vector
   */
  void computeTimeDerivatives();

  /**
   * Compute the residual
   * @param residual[out] Residual is formed here
   */
  void computeResidualInternal(NumericVector<Number> & residual);

  /**
   * Completes the assembly of residual
   * @param residual[out] Residual is formed here
   */
  void finishResidual(NumericVector<Number> & residual);

  void computeDiracContributions(NumericVector<Number> * residual, SparseMatrix<Number> * jacobian = NULL);

  void computeScalarKernelsJacobians(SparseMatrix<Number> & jacobian);

  /**
   * Enforce nodal constraints
   */
  void enforceNodalConstraintsResidual(NumericVector<Number> & residual);
  void enforceNodalConstraintsJacobian(SparseMatrix<Number> & jacobian);


  /// solution vector from nonlinear solver
  const NumericVector<Number> * _current_solution;
  /// solution vector from step prior to previous step
  //NumericVector<Number> & _older_solution;
  /// solution vector for u^dot
  //NumericVector<Number> & _solution_u_dot;
  /// solution vector for {du^dot}\over{du}
  //NumericVector<Number> & _solution_du_dot_du;
  /// residual evaluated at the old time step (need for Crank-Nicolson)
  //NumericVector<Number> * _residual_old;
  /// ghosted form of the residual
  NumericVector<Number> & _residual_ghosted;

  /// Serialized version of the solution vector
  NumericVector<Number> & _serialized_solution;

  /// Copy of the residual vector
  NumericVector<Number> & _residual_copy;

  /// time
  Real & _t;
  /// size of the time step
  Real & _dt;
  /// previous time step size
  Real & _dt_old;
  /// time step (number)
  int & _t_step;
  /// Coefficients (weights) for the time discretization
  std::vector<Real> & _time_weight;
  /// Time stepping scheme used for time discretization
  Moose::TimeSteppingScheme _time_stepping_scheme;
  /// The order of the time stepping scheme
  Real _time_stepping_order;

  // holders
  /// Kernel storage for each thread
  std::vector<KernelWarehouse> _kernels;
  /// BC storage for each thread
  std::vector<BCWarehouse> _bcs;
  /// Dirac Kernel storage for each thread
  std::vector<DiracKernelWarehouse> _dirac_kernels;
  /// DG Kernel storage for each thread
  std::vector<DGKernelWarehouse> _dg_kernels;
  /// Dampers for each thread
  std::vector<DamperWarehouse> _dampers;

public:
  /// Constraints for each thread
  std::vector<ConstraintWarehouse> _constraints;


protected:
  /// increment vector
  NumericVector<Number> * _increment_vec;
  /// Preconditioner
  MoosePreconditioner * _preconditioner;

  /// Whether or not to use a finite differenced preconditioner
  bool _use_finite_differenced_preconditioner;
#ifdef LIBMESH_HAVE_PETSC
  MatFDColoring _fdcoloring;
#endif

  /// Whether or not to add implicit geometric couplings to the Jacobian for FDP
  bool _add_implicit_geometric_coupling_entries_to_jacobian;

  /// Whether or not a copy of the residual needs to be made
  bool _need_serialized_solution;

  /// Whether or not a copy of the residual needs to be made
  bool _need_residual_copy;
  /// Whether or not a ghosted copy of the residual needs to be made
  bool _need_residual_ghosted;
  /// true if debugging residuals
  bool _debugging_residuals;

  /// true if DG is active (optimization reasons)
  bool _doing_dg;

  /// NumericVectors that will be zeroed before a residual computation
  std::vector<NumericVector<Number> *> _vecs_to_zero_for_residual;

  unsigned int _n_iters;
  unsigned int _n_linear_iters;
  Real _final_residual;

  /// true if predictor is active
  bool _use_predictor;
  /// Scale factor to use with predictor
  Real _predictor_scale;

  bool _computing_initial_residual;

  /**
   * If this is non-NULL, it holds an exception that we will re-throw
   */
 MooseException * _exception;
public:
  /// Time stepping scheme class where the actual work is done
  TimeScheme * _time_scheme;
  friend class ComputeResidualThread;
  friend class ComputeJacobianThread;
  friend class ComputeFullJacobianThread;
  friend class ComputeExplicitJacobianThread;
  friend class ComputeDiracThread;
  friend class ComputeDampingThread;
  friend class TimeScheme;
};

#endif /* NONLINEARSYSTEM_H */
