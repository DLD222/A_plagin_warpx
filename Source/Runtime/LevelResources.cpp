/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/LevelResources.H"

namespace warpx::runtime
{

LevelResources::LevelResources (int nlevs)
    : m_size(nlevs)
{
}

void
LevelResources::resize (int nlevs)
{
    m_size = nlevs;
}

} // namespace warpx::runtime
