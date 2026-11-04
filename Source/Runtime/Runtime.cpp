/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/Runtime.H"

#include "Initialization/WarpXInit.H"
#include "WarpX.H"

#include <stdexcept>

namespace warpx::runtime
{

Runtime::Runtime (int argc, char* argv[])
{
    warpx::initialization::initialize_external_libraries(argc, argv);
}

Runtime::~Runtime ()
{
    finalize();
    warpx::initialization::finalize_external_libraries();
}

void
Runtime::initialize ()
{
    if (m_finalized) {
        throw std::runtime_error("Runtime::initialize() called after finalize()");
    }
    WarpX::GetInstance().InitData();
    m_initialized = true;
}

void
Runtime::evolve (int numsteps)
{
    if (!m_initialized || m_finalized) {
        throw std::runtime_error("Runtime::evolve() called before initialize() or after finalize()");
    }
    WarpX::GetInstance().Evolve(numsteps);
}

void
Runtime::finalize ()
{
    if (m_finalized) { return; }
    WarpX::Finalize();
    m_finalized = true;
}

bool
Runtime::verbose () const
{
    return !m_finalized && WarpX::GetInstance().Verbose();
}

} // namespace warpx::runtime
