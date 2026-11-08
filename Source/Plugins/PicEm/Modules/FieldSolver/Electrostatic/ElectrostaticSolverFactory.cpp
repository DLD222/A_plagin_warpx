/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Electrostatic solver plugin entry point: the kernel asks the core, the core
 * forwards to this factory (S6).
 */
#include "FieldSolver/Electrostatic/EffectivePotentialES.H"
#include "FieldSolver/Electrostatic/LabFrameExplicitES.H"
#include "FieldSolver/Electrostatic/RelativisticExplicitES.H"

#include "Runtime/IElectrostaticSolver.H"
#include "Runtime/ServiceFactory.H"
#include "Utils/WarpXAlgorithmSelection.H"

#include <memory>

extern "C" void warpx_electrostatic_anchor (void) {}

namespace warpx::runtime
{
namespace
{
std::unique_ptr<IElectrostaticSolver>
make_electrostatic_solver (int algorithm, int max_level, const RuntimeContext* context)
{
    auto const algo = static_cast<ElectrostaticSolverAlgo>(algorithm);
    if (algo == ElectrostaticSolverAlgo::LabFrame ||
        algo == ElectrostaticSolverAlgo::LabFrameElectroMagnetostatic) {
        return std::make_unique<LabFrameExplicitES>(max_level, context);
    }
    if (algo == ElectrostaticSolverAlgo::LabFrameEffectivePotential) {
        return std::make_unique<EffectivePotentialES>(max_level, context);
    }
    return std::make_unique<RelativisticExplicitES>(max_level, context);
}

const bool electrostatic_factory_registered = [] {
    register_electrostatic_solver_factory(&make_electrostatic_solver);
    return true;
}();
} // namespace
} // namespace warpx::runtime
