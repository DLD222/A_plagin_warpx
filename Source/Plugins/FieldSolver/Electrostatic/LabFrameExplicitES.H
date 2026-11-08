/* Copyright 2024 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * Authors: Roelof Groenewald, Arianna Formenti, Revathi Jambunathan
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_LABFRAMEEXPLICITES_H_
#define WARPX_LABFRAMEEXPLICITES_H_

#include "ElectrostaticSolver.H"

class LabFrameExplicitES final : public ElectrostaticSolver
{
public:

    explicit LabFrameExplicitES (int nlevs_max, const warpx::runtime::RuntimeContext* ctx = nullptr) : ElectrostaticSolver (nlevs_max, ctx) {
        ReadParameters();
    }

    void InitData () override;

    void ComputeSpaceChargeField (
        ablastr::fields::MultiFabRegister& fields,
        warpx::runtime::ParticleContext const& particles,
        warpx::runtime::IFluidModel* mfl,
        int max_level,
        bool verbose_step) override;

    void computePhiTriDiagonal (
        const ablastr::fields::MultiLevelScalarField& rho,
        const ablastr::fields::MultiLevelScalarField& phi
    );

    void computePhiTriDiagonal_periodic (
        const ablastr::fields::MultiLevelScalarField& rho,
        const ablastr::fields::MultiLevelScalarField& phi
    );

};

#endif  // WARPX_LABFRAMEEXPLICITES_H_
