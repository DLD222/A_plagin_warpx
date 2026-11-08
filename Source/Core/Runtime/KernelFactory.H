/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_KERNELFACTORY_H_
#define WARPX_RUNTIME_KERNELFACTORY_H_

#include <memory>

namespace warpx::runtime
{

class IPicKernel;

/**
 * \brief Entry points a PIC kernel plugin provides to the Core runtime.
 *
 * Core never references a kernel implementation directly (A7 KernelFactory).
 * A plugin fills this table once, at static-initialization time; the runtime
 * then creates the kernel through it.  When no plugin registered, the runtime
 * reports that no kernel is available instead of failing to link.
 */
struct PicKernelPlugin
{
    std::unique_ptr<IPicKernel> (*create) (int argc, char* argv[]) = nullptr;
    void (*initialize_libraries) (int argc, char* argv[]) = nullptr;
    void (*finalize_libraries) () = nullptr;
};

/** \brief Register the PIC kernel implementation (idempotent; last one wins). */
void register_pic_kernel_plugin (const PicKernelPlugin& plugin);

//! True once a kernel plugin has registered.
[[nodiscard]] bool has_pic_kernel_plugin ();

/**
 * \brief Create the simulation kernel through the registered plugin.
 *
 * Throws std::runtime_error with an actionable message when no kernel plugin
 * is linked, which is the expected outcome for a Core-only build.
 */
[[nodiscard]] std::unique_ptr<IPicKernel> create_pic_kernel (int argc, char* argv[]);

//! Library init/finalize hooks; no-ops when the plugin does not provide them.
void initialize_runtime_libraries (int argc, char* argv[]);
void finalize_runtime_libraries ();

//! Drop the registration again (tests only; not part of the runtime lifecycle).
void clear_pic_kernel_plugin ();

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_KERNELFACTORY_H_
