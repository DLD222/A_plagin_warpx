/* Copyright 2026 The WarpX Community
 *
 * Authors: Remi Lehe
 * License: BSD-3-Clause-LBNL
 */
#include "Python/pyWarpX.H"

#include <Runtime/IImplicitSolver.H>

#include <ablastr/fields/MultiFabRegister.H>

// The implicit solver is owned by the kernel and destroyed with it, so the Python object must
// never delete it: py::nodelete keeps the returned pointer non-owning.
//
// The binding deliberately names only the core contract.  The solver-specific operations it
// exposes (mass-matrix deposition and application) are declared on IImplicitSolver, so this
// file needs no header from the field-solver plugin - a plugin must not have to include
// another plugin to publish its own API.


void init_ImplicitSolver (py::module& m)
{
    py::class_<warpx::runtime::IImplicitSolver>(m, "IImplicitSolver")
        .def("finish_mass_matrices_deposit",
            &warpx::runtime::IImplicitSolver::FinishMassMatricesDeposition,
            R"pbdoc(Fill the second half of the symmetric diagonal mass matrices)pbdoc")
        .def("apply_mass_matrices",
            &warpx::runtime::IImplicitSolver::ApplyMassMatrices,
            py::arg("out"), py::arg("in"),
            py::arg("in_ref").none(true) = nullptr,
            py::arg("baseline").none(true) = nullptr,
            py::arg("scale") = 1.0,
            py::arg("zero_out_first") = false,
            R"pbdoc(Apply the mass matrices to a vector field)pbdoc")
    ;
}
