#ifndef __XPBD_MESH_OBJECT_CONFIG_HPP
#define __XPBD_MESH_OBJECT_CONFIG_HPP

#include "config/ElasticMeshObjectConfig.hpp"

enum class XPBDSolverType
{
    GAUSS_SEIDEL,
    JACOBI
};

enum class XPBDConstraintType
{
    STABLE_NEOHOOKEAN,
    STABLE_NEOHOOKEAN_COMBINED
};

enum class XPBDResidualPolicy
{
    NEVER=0,
    EVERY_SUBSTEP,
    EVERY_ITERATION
};

class XPBDMeshObjectConfig : public ElasticMeshObjectConfig
{
    /** Static predefined default for the number of solver iterations */
    static std::optional<unsigned>& DEFAULT_NUM_SOLVER_ITERS() { static std::optional<unsigned> num_solver_iters(1); return num_solver_iters; }
    /** Static predefined default for solve mode */
    static std::optional<XPBDSolverType>& DEFAULT_SOLVER_TYPE() { static std::optional<XPBDSolverType> solver_type(XPBDSolverType::GAUSS_SEIDEL); return solver_type; }
    /** Static predefined default for constraint type */
    static std::optional<XPBDConstraintType>& DEFAULT_CONSTRAINT_TYPE() { static std::optional<XPBDConstraintType> constraint_type(XPBDConstraintType::STABLE_NEOHOOKEAN); return constraint_type; }
    
    static std::optional<bool>& DEFAULT_WITH_RESIDUAL() { static std::optional<bool> with_residual(false); return with_residual; }
    static std::optional<bool>& DEFAULT_WITH_DAMPING() { static std::optional<bool> with_damping(false); return with_damping; }

    /** Static predefined default for damping stiffness */
    static std::optional<double>& DEFAULT_DAMPING_GAMMA() { static std::optional<double> damping_stiffness(0); return damping_stiffness; }
    /** Static predefined default for residual policy */
    static std::optional<XPBDResidualPolicy>& DEFAULT_RESIDUAL_POLICY() { static std::optional<XPBDResidualPolicy> residual_policy(XPBDResidualPolicy::EVERY_SUBSTEP); return residual_policy; }

    static std::optional<double>& DEFAULT_MASS_TO_DAMPING_MULTIPLIER() { static std::optional<double> mass_to_damping(1); return mass_to_damping; }

    static std::optional<double>& DEFAULT_G_SCALING() { static std::optional<double> g_scaling(1); return g_scaling; }

    static std::map<std::string, XPBDSolverType>& SOLVER_TYPE_OPTIONS() 
    {
        static std::map<std::string, XPBDSolverType> solver_type_options{
            {"Gauss-Seidel", XPBDSolverType::GAUSS_SEIDEL},
            {"Jacobi", XPBDSolverType::JACOBI}
        };
        return solver_type_options;
    }

    static std::map<std::string, XPBDConstraintType>& CONSTRAINT_TYPE_OPTIONS()
    { 
        static std::map<std::string, XPBDConstraintType> constraint_type_options{
            {"Stable-Neohookean", XPBDConstraintType::STABLE_NEOHOOKEAN},
            {"Stable-Neohookean-Combined", XPBDConstraintType::STABLE_NEOHOOKEAN_COMBINED}
        };
        return constraint_type_options;
    }

    static std::map<std::string, XPBDResidualPolicy>& RESIDUAL_POLICY_OPTIONS()
    {
        static std::map<std::string, XPBDResidualPolicy> residual_policy_options{
            {"Never", XPBDResidualPolicy::NEVER},
            {"Every-Substep", XPBDResidualPolicy::EVERY_SUBSTEP},
            {"Every-Iteration", XPBDResidualPolicy::EVERY_ITERATION}
        };
        return residual_policy_options;
    }

    public:
    /** Creates a Config from a YAML node, which consists of the specialized parameters needed for XPBDMeshObject.
     * @param node : the YAML node (i.e. dictionary of key-value pairs) that information is pulled from
     */
    explicit XPBDMeshObjectConfig(const YAML::Node& node)
        : ElasticMeshObjectConfig(node)
    {
        // extract parameters
        _extractParameter("num-solver-iters", node, _num_solver_iters, DEFAULT_NUM_SOLVER_ITERS());
        _extractParameterWithOptions("solver-type", node, _solve_type, SOLVER_TYPE_OPTIONS(), DEFAULT_SOLVER_TYPE());
        _extractParameterWithOptions("constraint-type", node, _constraint_type, CONSTRAINT_TYPE_OPTIONS(), DEFAULT_CONSTRAINT_TYPE());
        _extractParameter("with-residual", node, _with_residual, DEFAULT_WITH_RESIDUAL());
        _extractParameter("with-damping", node, _with_damping, DEFAULT_WITH_DAMPING());
        _extractParameter("damping-gamma", node, _damping_gamma, DEFAULT_DAMPING_GAMMA());
        _extractParameterWithOptions("residual-policy", node, _residual_policy, RESIDUAL_POLICY_OPTIONS(), DEFAULT_RESIDUAL_POLICY());
        _extractParameter("mass-to-damping-multiplier", node, _mass_to_damping_multiplier, DEFAULT_MASS_TO_DAMPING_MULTIPLIER());
        _extractParameter("g-scaling", node, _g_scaling, DEFAULT_G_SCALING());
        
    }

    // Getters
    std::optional<unsigned> numSolverIters() const { return _num_solver_iters.value; }
    std::optional<XPBDSolverType> solverType() const { return _solve_type.value; }
    std::optional<XPBDConstraintType> constraintType() const { return _constraint_type.value; }
    std::optional<bool> withResidual() const { return _with_residual.value; }
    std::optional<bool> withDamping() const { return _with_damping.value; }
    std::optional<double> dampingGamma() const { return _damping_gamma.value; }
    std::optional<XPBDResidualPolicy> residualPolicy() const { return _residual_policy.value; }
    std::optional<double> massToDampingMultiplier() const { return _mass_to_damping_multiplier.value; }
    std::optional<double> gScaling() const { return _g_scaling.value; }

    protected:
    // Parameters
    ConfigParameter<unsigned> _num_solver_iters;
    ConfigParameter<XPBDSolverType> _solve_type;
    ConfigParameter<XPBDConstraintType> _constraint_type;
    ConfigParameter<bool> _with_residual;
    ConfigParameter<bool> _with_damping;
    ConfigParameter<double> _damping_gamma;
    ConfigParameter<XPBDResidualPolicy> _residual_policy;
    ConfigParameter<double> _mass_to_damping_multiplier;
    ConfigParameter<double> _g_scaling;
};

#endif // __XPBD_MESH_OBJECT_CONFIG_HPP