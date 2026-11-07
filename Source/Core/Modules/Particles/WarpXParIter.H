#ifndef WARPX_CORE_PARTICLES_WARPXPARITER_H_
#define WARPX_CORE_PARTICLES_WARPXPARITER_H_
#include "Modules/Particles/ParticleAttributes.H"
#include "Runtime/IParticleContainer.H"
#include <AMReX_ParIter.H>
#include <array>
#include <string>
class WarpXParIter
    : public amrex::ParIterSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator>
{
public:
    using amrex::ParIterSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator>::ParIterSoA;
    WarpXParIter (ContainerType& pc, int level);
    WarpXParIter (ContainerType& pc, int level, amrex::MFItInfo& info);
    [[nodiscard]] const std::array<RealVector, PIdx::nattribs>& GetAttribs () const
    { return GetStructOfArrays().GetRealData(); }
    [[nodiscard]] std::array<RealVector, PIdx::nattribs>& GetAttribs ()
    { return GetStructOfArrays().GetRealData(); }
    [[nodiscard]] const RealVector& GetAttribs (int comp) const
    { return GetStructOfArrays().GetRealData(comp); }
    [[nodiscard]] RealVector& GetAttribs (int comp)
    { return GetStructOfArrays().GetRealData(comp); }
    [[nodiscard]] const IntVector& GetiAttribs (int comp) const
    { return GetStructOfArrays().GetIntData(comp); }
    [[nodiscard]] IntVector& GetiAttribs (int comp)
    { return GetStructOfArrays().GetIntData(comp); }
    [[nodiscard]] const RealVector& GetAttribs (const std::string& name) const
    { return GetStructOfArrays().GetRealData(name); }
    [[nodiscard]] RealVector& GetAttribs (const std::string& name)
    { return GetStructOfArrays().GetRealData(name); }
    [[nodiscard]] const IntVector& GetiAttribs (const std::string& name) const
    { return GetStructOfArrays().GetIntData(name); }
    [[nodiscard]] IntVector& GetiAttribs (const std::string& name)
    { return GetStructOfArrays().GetIntData(name); }
};
#endif
