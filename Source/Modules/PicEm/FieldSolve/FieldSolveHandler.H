/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_MODULES_PICEM_FIELDSOLVE_FIELDSOLVEHANDLER_H_
#define WARPX_MODULES_PICEM_FIELDSOLVE_FIELDSOLVEHANDLER_H_

#include "Runtime/HandlerSpec.H"

#include <string>

namespace warpx::runtime
{

/** \brief Field solve module, named after the selected algorithm. */
class FieldSolveHandler final : public IHandler
{
public:
    explicit FieldSolveHandler (std::string name) : m_name(std::move(name)) {}

    [[nodiscard]] HandlerSpec spec () const override;
    void apply (StageContext& ctx) override;

private:
    std::string m_name;
};

} // namespace warpx::runtime

#endif
