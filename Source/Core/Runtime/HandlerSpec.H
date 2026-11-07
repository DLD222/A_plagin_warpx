/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_HANDLERSPEC_H_
#define WARPX_RUNTIME_HANDLERSPEC_H_

#include "Runtime/StagePoint.H"

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
};

/** Execution context passed to a handler (A3.3a skeleton). */
struct StageContext
{
    const RuntimeContext* context = nullptr;
    DataBus* bus = nullptr;
    HostServices* host = nullptr;
    const RuntimeData* data = nullptr;
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
        }
        m_handlers.push_back(std::move(handler));
        m_specs.push_back(spec);
        m_handler_specs.push_back(spec);
    }

    void declare (HandlerSpec spec)
    {
        m_specs.push_back(std::move(spec));
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

        auto field_matches = [](const std::string& a, const std::string& b) {
            if (a == b) { return true; }
            // A spec may declare an aggregate name ("Efield_fp") while the
            // registry stores components ("Efield_fp_x").
            if (b.size() > a.size() && b.compare(0, a.size(), a) == 0 && b[a.size()] == '_') { return true; }
            if (a.size() > b.size() && a.compare(0, b.size(), b) == 0 && a[b.size()] == '_') { return true; }
            return false;
        };
        auto intersects = [&field_matches](const std::vector<std::string>& a,
                                           const std::vector<std::string>& b) {
            for (const auto& x : a) {
                for (const auto& y : b) { if (field_matches(x, y)) { return true; } }
            }
            return false;
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
                        intersects(si.writes_species, sj.reads_species)) {
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
                        std::to_string(static_cast<int>(specs[order[begin]].point.stage)) + "," +
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

    [[nodiscard]] std::vector<IHandler*> at (Stage stage) const
    {
        std::vector<IHandler*> out;
        for (auto* h : ordered()) {
            if (h->spec().point.stage == stage) { out.push_back(h); }
        }
        return out;
    }

private:
    std::vector<std::shared_ptr<IHandler>> m_handlers;
    std::vector<HandlerSpec> m_specs;
    std::vector<HandlerSpec> m_handler_specs;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_HANDLERSPEC_H_
