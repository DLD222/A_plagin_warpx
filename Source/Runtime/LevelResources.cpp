/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/LevelResources.H"

#include <algorithm>

#include "AcceleratorLattice/AcceleratorLattice.H"
#include "EmbeddedBoundary/WarpXFaceInfoBox.H"

namespace warpx::runtime
{

LevelResources::LevelResources (int nlevs)
    : m_levels(static_cast<std::size_t>(std::max(nlevs, 0)))
{
}

void
LevelResources::resize (int nlevs)
{
    m_levels.resize(static_cast<std::size_t>(std::max(nlevs, 0)));
}

void
LevelResources::clear (int lev)
{
    if (lev < 0 || lev >= size()) { return; }
    m_levels[static_cast<std::size_t>(lev)] = LevelStorage{};
}

} // namespace warpx::runtime
