/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_MODULES_PICEM_PICEMPACKAGE_H_
#define WARPX_MODULES_PICEM_PICEMPACKAGE_H_

#include "Runtime/HandlerSpec.H"

#include <set>
#include <string>

namespace warpx::runtime
{

/** \brief Built-in PIC/EM package: registers one unit module per capability. */
class PicEmPackage
{
public:
    static void register_handlers (PluginHost& host,
                                   const std::string& field_solver_module,
                                   const std::set<std::string>& disabled = {});
    static void register_specs (PluginHost& host,
                                const std::string& field_solver_module = "field_solve.yee");
};

} // namespace warpx::runtime

#endif
