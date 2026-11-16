#ifndef WARPX_RUNTIME_EBGEOMETRY_H_
#define WARPX_RUNTIME_EBGEOMETRY_H_

#include "Runtime/IEBGeometry.H"
#include "Runtime/RuntimeData.H"

namespace warpx::runtime {

/** Default implementation: forwards to the core's own EB factory. No plugin involved. */
class EBGeometry final : public IEBGeometry
{
public:
    explicit EBGeometry (RuntimeData const& data, int finest_level)
        : m_data(data), m_finest_level(finest_level) {}

    [[nodiscard]] bool hasEB (int lev) const override
    {
        return lev >= 0 && lev <= m_finest_level;
    }

    [[nodiscard]] amrex::EBFArrayBoxFactory const& factory (int lev) const override
    {
        return m_data.fieldEBFactory(lev);
    }

private:
    RuntimeData const& m_data;
    int m_finest_level;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_EBGEOMETRY_H_
