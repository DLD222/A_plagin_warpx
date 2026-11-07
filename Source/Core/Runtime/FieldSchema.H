/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_FIELDSCHEMA_H_
#define WARPX_RUNTIME_FIELDSCHEMA_H_

#include "Runtime/FieldRegistry.H"

namespace warpx::runtime
{
//! A2.4 name kept as an alias for the A4.1 FieldRegistry.
using FieldSchema = FieldRegistry;
} // namespace warpx::runtime

#endif // WARPX_RUNTIME_FIELDSCHEMA_H_
