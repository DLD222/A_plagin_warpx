/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Out-of-tree example module written in plain C against the v3 plugin ABI.
 */
#ifndef WARPX_TEST_EXAMPLE_PLUGIN_H_
#define WARPX_TEST_EXAMPLE_PLUGIN_H_

#include "Runtime/PluginCApi.H"

#ifdef __cplusplus
extern "C" {
#endif

/* Descriptor of the example plugin; register it in-process or return it from
 * warpx_plugin_entry for dynamic loading. */
const WarpXPluginDesc* warpx_example_plugin_desc (void);

/* Number of times the handler's apply() ran. */
int warpx_example_plugin_apply_count (void);

#ifdef __cplusplus
}
#endif

#endif /* WARPX_TEST_EXAMPLE_PLUGIN_H_ */
