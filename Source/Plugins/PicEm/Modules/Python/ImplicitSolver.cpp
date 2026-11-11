/* Copyright 2026 The WarpX Community
 *
 * Authors: Remi Lehe
 * License: BSD-3-Clause-LBNL
 */
#include "Python/pyWarpX.H"

#include <FieldSolver/Implicit/ImplicitSolver.H>

#include <ablastr/fields/MultiFabRegister.H>

#include <memory>


void init_ImplicitSolver (py::module& m)
{
    // The implicit solver is owned by WarpX (WarpX::m_implicit_solver) and is
    // destroyed with it in WarpX::Finalize, so the Python object must never
    // delete it: py::nodelete makes the returned pointer non-owning.
    // The kernel hands out the solver through its core interface
    // (IImplicitSolver*). Register that interface as the pybind11 base class, or
    // pybind11 cannot convert the pointer: it falls back to the concrete derived
    // type and fails with "Unregistered type : ThetaImplicitEM".
    // The solver-specific calls live on ImplicitSolver, not on the interface, so they
    // are exposed on the base binding through a downcast: the kernel returns an
    // IImplicitSolver*, and pybind11 hands Python that object.
    py::class_<warpx::runtime::IImplicitSolver>(m, "IImplicitSolver")
        .def("finish_mass_matrices_deposit",
            [](warpx::runtime::IImplicitSolver& s) {
                dynamic_cast<ImplicitSolver&>(s).FinishMassMatricesDeposition();
            },
            R"pbdoc(Fill the second half of the symmetric diagonal mass matrices)pbdoc")
        .def("apply_mass_matrices",
            [](warpx::runtime::IImplicitSolver& s,
                ablastr::fields::MultiLevelVectorField& out,
                const ablastr::fields::MultiLevelVectorField& in,
                const ablastr::fields::MultiLevelVectorField* in_ref,
                const ablastr::fields::MultiLevelVectorField* baseline,
                amrex::Real scale, bool zero_out_first) {
                dynamic_cast<ImplicitSolver&>(s).ApplyMassMatrices(out, in, in_ref, baseline,
                                                                   scale, zero_out_first);
            },
            py::arg("out"), py::arg("in"),
            py::arg("in_ref").none(true) = nullptr,
            py::arg("baseline").none(true) = nullptr,
            py::arg("scale") = 1.0,
            py::arg("zero_out_first") = false,
            R"pbdoc(Apply the mass matrices to a vector field)pbdoc")
    ;

    py::class_<ImplicitSolver, warpx::runtime::IImplicitSolver, std::unique_ptr<ImplicitSolver, py::nodelete>>(m, "ImplicitSolver")
        .def("finish_mass_matrices_deposit", &ImplicitSolver::FinishMassMatricesDeposition,
            R"pbdoc(Fill the second half of the symmetric diagonal mass matrices)pbdoc"
        )
        .def("apply_mass_matrices", &ImplicitSolver::ApplyMassMatrices,
            py::arg("out"), py::arg("in"),
            py::arg("in_ref").none(true) = nullptr,
            py::arg("baseline").none(true) = nullptr,
            py::arg("scale") = 1.0,
            py::arg("zero_out_first") = false,
            R"pbdoc(Apply the mass matrices to a vector field)pbdoc"
        )
    ;
}
