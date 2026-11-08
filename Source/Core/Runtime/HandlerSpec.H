/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_HANDLERSPEC_H_
#define WARPX_RUNTIME_HANDLERSPEC_H_

#include "Runtime/KeyMatch.H"
#include "Runtime/PluginManifest.H"
#include "Runtime/StagePoint.H"
#include "Runtime/StageRegistry.H"

#include <AMReX_REAL.H>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace warpx::runtime
{

class DataBus;
class HostServices;
class RuntimeData;
struct RuntimeConfigSnapshot;
struct RuntimeContext;

/** Per-handler data contract. Declarations only; the bus owns no storage. */
struct HandlerSpec
{
    std::string name;
    StagePoint point;
    std::vector<std::string> reads_fields;
    std::vector<std::string> writes_fields;
    std::vector<std::string> reads_species;
    std::vector<std::string> writes_species;
    int priority = 0;
    // Scalars are declared after `priority` so existing positional aggregate
    // initialisations keep their meaning.
    std::vector<std::string> reads_scalars;
    std::vector<std::string> writes_scalars;
};

/** Execution context passed to a handler (A3.3a skeleton). */
struct StageContext
{
    const RuntimeContext* context = nullptr;
    DataBus* bus = nullptr;
    HostServices* host = nullptr;
    const RuntimeData* data = nullptr;
    const RuntimeConfigSnapshot* config = nullptr;
    int step = 0;
    amrex::Real time = 0.0;
    amrex::Real dt = 0.0;
};

class IHandler
{
public:
    virtual ~IHandler () = default;
    [[nodiscard]] virtual HandlerSpec spec () const = 0;
    virtual void apply (StageContext&) = 0;

    /** Optional callback after a successful AMR regrid. */
    virtual void on_regrid (StageContext& /*ctx*/) {}
};

/**
 * \brief Minimal handler registry (A3.3a).
 *
 * A3.3a only stores handlers and orders them by StagePoint/priority. Data
 * dependency topological sorting is added in A3.3c.
 */
class PluginHost
{
public:
    void add (std::shared_ptr<IHandler> handler)
    {
        const HandlerSpec spec = handler->spec();
        validate(spec);
        for (const auto& existing : m_specs) {
            if (!(existing.point == spec.point)) { continue; }
            for (const auto& w : spec.writes_fields) {
                for (const auto& ew : existing.writes_fields) {
                    if (w == ew) {
                        throw std::runtime_error(
                            "PluginHost: handlers '" + existing.name + "' and '" +
                            spec.name + "' both write field '" + w +
                            "' at the same StagePoint");
                    }
                }
            }
            for (const auto& w : spec.writes_scalars) {
                for (const auto& ew : existing.writes_scalars) {
                    if (w == ew) {
                        throw std::runtime_error(
                            "PluginHost: handlers '" + existing.name + "' and '" +
                            spec.name + "' both write scalar '" + w +
                            "' at the same StagePoint");
                    }
                }
            }
        }
        m_handlers.push_back(std::move(handler));
        m_specs.push_back(spec);
        m_handler_specs.push_back(spec);
    }

    void declare (HandlerSpec spec)
    {
        validate(spec);
        m_specs.push_back(std::move(spec));
    }

    // ---------------------------------------------------------------- S4 --
    /** \brief Register a plugin (manifest + lifecycle). Names must be unique. */
    void add_plugin (std::shared_ptr<IPluginLifecycle> plugin)
    {
        PluginManifest manifest = plugin->manifest();
        if (manifest.name.empty()) {
            throw std::runtime_error("PluginHost: plugin without a manifest name");
        }
        for (const auto& existing : m_plugins) {
            if (existing.plugin->manifest().name == manifest.name) {
                throw std::runtime_error(
                    "PluginHost: plugin '" + manifest.name + "' registered twice");
            }
        }
        m_plugins.push_back({std::move(manifest), std::move(plugin)});
    }

    [[nodiscard]] std::size_t plugin_count () const { return m_plugins.size(); }

    [[nodiscard]] std::vector<PluginManifest> manifests () const
    {
        std::vector<PluginManifest> out;
        out.reserve(m_plugins.size());
        for (const auto& entry : m_plugins) { out.push_back(entry.manifest); }
        return out;
    }

    /**
     * \brief Refuse to start when a manifest requirement is unmet.
     *
     * A requirement is satisfied by another registered plugin's manifest name,
     * by a plugin role ("kernel", "diagnostics", ...), or by the reserved
     * names "core" and "runtime".
     */
    void validate_plugins () const
    {
        std::vector<std::string> provided{"core", "runtime"};
        for (const auto& entry : m_plugins) {
            provided.push_back(entry.manifest.name);
            provided.push_back(role_name(entry.manifest.role));
        }
        for (const auto& entry : m_plugins) {
            for (const auto& requirement : entry.manifest.required_plugins) {
                bool satisfied = false;
                for (const auto& have : provided) {
                    if (have == requirement) { satisfied = true; break; }
                }
                if (!satisfied) {
                    throw std::runtime_error(
                        "PluginHost: plugin '" + entry.manifest.name +
                        "' requires \"" + requirement +
                        "\", which is neither registered nor a known role");
                }
            }
        }
    }

    /**
     * \brief Refuse to start when an active run feature is not declared.
     *
     * Only plugins that declared at least one capability are checked: an empty
     * list means "no claim", so existing plugins keep working.
     */
    void validate_capabilities (const std::vector<std::string>& active_features) const
    {
        for (const auto& entry : m_plugins) {
            if (entry.manifest.capabilities.empty()) { continue; }
            for (const auto& feature : active_features) {
                bool supported = false;
                for (const auto& capability : entry.manifest.capabilities) {
                    if (capability == feature) { supported = true; break; }
                }
                if (!supported) {
                    std::string declared;
                    for (const auto& capability : entry.manifest.capabilities) {
                        if (!declared.empty()) { declared += ", "; }
                        declared += capability;
                    }
                    throw std::runtime_error(
                        "PluginHost: plugin '" + entry.manifest.name +
                        "' does not support the active run feature '" + feature +
                        "' (declared capabilities: " + declared +
                        "). Disable the feature or use a plugin that supports it.");
                }
            }
        }
    }

    void run_schema (SchemaContext& ctx) const
    {
        for (const auto& entry : m_plugins) { entry.plugin->on_schema(ctx); }
    }
    void run_init (StageContext& ctx) const
    {
        for (const auto& entry : m_plugins) { entry.plugin->on_init(ctx); }
    }
    void run_level_created (StageContext& ctx, int lev) const
    {
        for (const auto& entry : m_plugins) { entry.plugin->on_level_created(ctx, lev); }
    }
    void run_checkpoint (StageContext& ctx) const
    {
        for (const auto& entry : m_plugins) { entry.plugin->on_checkpoint(ctx); }
    }
    void run_restart (StageContext& ctx) const
    {
        for (const auto& entry : m_plugins) { entry.plugin->on_restart(ctx); }
    }
    void run_finalize (StageContext& ctx) const
    {
        for (const auto& entry : m_plugins) { entry.plugin->on_finalize(ctx); }
    }

    [[nodiscard]] std::size_t size () const { return m_handlers.size(); }

    [[nodiscard]] const std::vector<HandlerSpec>& specs () const { return m_specs; }

    [[nodiscard]] std::vector<std::size_t>
    order_indices (const std::vector<HandlerSpec>& specs) const
    {
        std::vector<std::size_t> order(specs.size());
        for (std::size_t i = 0; i < order.size(); ++i) { order[i] = i; }
        std::stable_sort(order.begin(), order.end(), [&specs](std::size_t a, std::size_t b) {
            if (!(specs[a].point == specs[b].point)) { return specs[a].point < specs[b].point; }
            return specs[a].priority < specs[b].priority;
        });

        auto intersects = [](const std::vector<std::string>& a,
                             const std::vector<std::string>& b) {
            return key_intersects(a, b);
        };

        std::vector<std::size_t> sorted;
        sorted.reserve(order.size());
        std::size_t begin = 0;
        while (begin < order.size()) {
            std::size_t end = begin + 1;
            while (end < order.size() &&
                   specs[order[end]].point == specs[order[begin]].point) { ++end; }

            const std::size_t n = end - begin;
            std::vector<int> indegree(n, 0);
            std::vector<std::vector<bool>> edge(n, std::vector<bool>(n, false));
            for (std::size_t i = 0; i < n; ++i) {
                const auto& si = specs[order[begin + i]];
                for (std::size_t j = 0; j < n; ++j) {
                    if (i == j) { continue; }
                    const auto& sj = specs[order[begin + j]];
                    if (intersects(si.writes_fields, sj.reads_fields) ||
                        intersects(si.writes_species, sj.reads_species) ||
                        intersects(si.writes_scalars, sj.reads_scalars)) {
                        edge[i][j] = true;
                        ++indegree[j];
                    }
                }
            }
            std::vector<bool> done(n, false);
            for (std::size_t k = 0; k < n; ++k) {
                std::size_t pick = n;
                for (std::size_t i = 0; i < n; ++i) {
                    if (!done[i] && indegree[i] == 0) { pick = i; break; }
                }
                if (pick == n) {
                    throw std::runtime_error(
                        "PluginHost: dependency cycle at StagePoint (" +
                        StageRegistry::instance().name_of(specs[order[begin]].point.stage) + "," +
                        std::to_string(specs[order[begin]].point.sub) + ")");
                }
                done[pick] = true;
                sorted.push_back(order[begin + pick]);
                for (std::size_t j = 0; j < n; ++j) {
                    if (edge[pick][j]) { --indegree[j]; }
                }
            }
            begin = end;
        }
        return sorted;
    }

    [[nodiscard]] std::vector<HandlerSpec> ordered_specs () const
    {
        std::vector<HandlerSpec> out;
        if (!m_handler_specs.empty()) {
            out.reserve(m_handler_specs.size());
            for (const auto idx : order_indices(m_handler_specs)) {
                out.push_back(m_handler_specs[idx]);
            }
            return out;
        }
        out = m_specs;
        std::stable_sort(out.begin(), out.end(), [](const HandlerSpec& a, const HandlerSpec& b) {
            if (!(a.point == b.point)) { return a.point < b.point; }
            return a.priority < b.priority;
        });
        return out;
    }

    void notify_regrid (StageContext& ctx) const
    {
        for (auto* handler : ordered()) { handler->on_regrid(ctx); }
    }

    [[nodiscard]] std::vector<IHandler*> ordered () const
    {
        std::vector<IHandler*> out;
        out.reserve(m_handlers.size());
        for (const auto idx : order_indices(m_handler_specs)) {
            out.push_back(m_handlers[idx].get());
        }
        return out;
    }

    [[nodiscard]] std::vector<IHandler*> at (int stage) const
    {
        std::vector<IHandler*> out;
        for (auto* h : ordered()) {
            if (h->spec().point.stage == stage) { out.push_back(h); }
        }
        return out;
    }

private:
    static void validate (const HandlerSpec& spec)
    {
        if (!StageRegistry::instance().contains(spec.point.stage)) {
            throw std::runtime_error(
                "PluginHost: handler '" + spec.name + "' is declared at stage " +
                StageRegistry::instance().name_of(spec.point.stage) +
                ", which is not a registered stage. Register it with "
                "warpx::runtime::register_stage() before adding the handler.");
        }
    }

    struct PluginEntry
    {
        PluginManifest manifest;
        std::shared_ptr<IPluginLifecycle> plugin;
    };

    std::vector<PluginEntry> m_plugins;
    std::vector<std::shared_ptr<IHandler>> m_handlers;
    std::vector<HandlerSpec> m_specs;
    std::vector<HandlerSpec> m_handler_specs;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_HANDLERSPEC_H_
