#ifndef __SOLVER_HPP
#define __SOLVER_HPP

#include "geometry/Mesh.hpp"
#include "simobject/XPBDMeshObject.hpp"
#include "config/XPBDMeshObjectConfig.hpp"

#include <memory>

namespace Solver
{

/** Enforces constraints with the XPBD algorithm, described in https://matthias-research.github.io/pages/publications/XPBD.pdf 
 * 
 * Owns ConstraintProjectors, which do the actual projection of constraints - this class decides what to do with/how to apply the position updates.
 * 
 * Also has functionality to compute the primary and constraint residuals associated with the equations of motion (Equations (8) and (9) in XPBD paper)
*/
class XPBDSolver
{
    public:
    /** Creates XPBDSolver for the passed in XPBDMeshObject. It will initially have no ConstraintProjectors, these will be added with addConstraintProjector().
     * @param obj - pointer to XPBDMeshObject which is being updated by this solver
     * @param num_iter - number of solver loop iterations
     * @param residual_policy - how often to compute the residual
     */
    explicit XPBDSolver(const Sim::XPBDMeshObject* obj, int num_iter, XPBDResidualPolicy residual_policy);

    virtual ~XPBDSolver() = default;

    /** Project the constraints and apply the position updates.
     * Consists of constraint initialization, a solver loop that will run num_iter times, and optional residual calculation.
     */
    virtual void solve();

    /** Returns the current primary residual (Equation (8) from XPBD paper).
     * Does NOT recalculate if stale.
     */
    const Eigen::VectorXd primaryResidual() const { return Eigen::Map<const Eigen::VectorXd>(_primary_residual.data(), _primary_residual.size()); };

    /** Returns the current constraint residual (Equation (9) from XPBD paper).
     * Does NOT recalculate if stale.
     */
    const Eigen::VectorXd constraintResidual() const { return Eigen::Map<const Eigen::VectorXd>(_constraint_residual.data(), _constraint_residual.size()); };

    /** Returns the number of solver iterations. */
    int numIterations() const { return _num_iter; }

    const std::vector<std::unique_ptr<ConstraintProjector>>& constraintProjectors() const { return _constraint_projectors; }

    /** Adds a constraint projector to this Solver. During solve(), the ConstraintProjector::project() method is called on each ConstraintProjector.
     * This object takes ownership of the ConstraintProjector passed in.
     *
     * Each time a ConstraintProjector, the size of the data buffer is checked to make sure that it is large enough to accomodate the new constraint.
     * 
     * Returns the index of the constraint projector in the member list, which can be used to remove it later. We do this frequently with collision constraints and attachment constraints.
     */
    int addConstraintProjector(std::unique_ptr<ConstraintProjector> projector); 


    void removeConstraintProjector(const int index);

    protected:
    /** Helper function that will perform 1 iteration of the solver. 
     * This method is pure virtual because its implementation depends on the solver type (Gauss-Seidel, Jacobi, etc.) to know what to do with the position updates given by the ConstraintProjectors.
     * @param data - the pre-allocated data block to use for evaluating the constraints and their gradients. Assumes that it is large enough to accomodate the ConstraintProjector with the largest memory requirement.
     */
    virtual void _solveConstraints(double* data) = 0;
    
    /** Calculates the primary residual (Equation (8) from XPBD paper). */
    void _calculatePrimaryResidual();

    /** Calculates the constraint residual (Equation (9) from XPBD paper). */
    void _calculateConstraintResidual();


    protected:
    std::vector<std::unique_ptr<ConstraintProjector>> _constraint_projectors;       // constraint projectors that will be used to project constraints to get position updates
    std::vector<int> _empty_indices;                       // indices in the _constraint_projectors vector that are empty (the ConstraintProjector they used to have got removed)

    const Sim::XPBDMeshObject* _obj;                                 // pointer to the XPBDMeshObject that owns this Solver and is updated by the solver loop
    int _num_iter;                                         // number of solver iterations per solve() call
    Geometry::Mesh::VerticesMat _inertial_positions;                // stores the positions after the inertial update - useful for primary residual calculation

    XPBDResidualPolicy _residual_policy;                        // determines how often to calculate the residuals - this varies depending on the information needs of the user

    bool _constraints_using_primary_residual;                   // whether or not any of the ConstraintProjectors require the primary residual to do the position update

    int _num_constraints;                                  // total number of constraints projected (note this may be different from number of ConstraintProjectors if some constraints are solved simultaneously)

    std::vector<double> _primary_residual;                      // primary residual vector
    std::vector<double> _constraint_residual;                   // constraint residual vector - use std::vector instead of Eigen::VectorXd to minimize dynamic reallocations as number of constraints change

    mutable std::vector<double> _data;                          // the vector class is used to pre-allocate data for the solver loop
    std::vector<double> _coordinate_updates;                    // stores updates to the coordinates determined by the constraint projections - also pre-allocated. Needs to be have a size >= the max number of positions affected by a single constraint projection.
    std::vector<double> _rigid_body_updates;                    // stores updates to any rigid bodies involved in the constraint projections - also pre-allocated. Each rigid body update consists of a position update and an orientation update, which is 7 doubles.
};

} // namespace Solver

#endif // __SOLVER_HPP