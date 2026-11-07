#ifndef WARPX_RUNTIME_IFIELDALGEBRA_H_
#define WARPX_RUNTIME_IFIELDALGEBRA_H_
#include <ablastr/fields/MultiFabRegister.H>
#include <AMReX_MultiFab.H>
#include <array>
namespace warpx::runtime {
class IFieldAlgebra {
public:
    virtual ~IFieldAlgebra () = default;
    virtual void computeDivB (amrex::MultiFab&, int, ablastr::fields::VectorField const&, const std::array<amrex::Real,3>&) = 0;
    virtual void computeDivE (amrex::MultiFab&, int) = 0;
    virtual std::unique_ptr<amrex::MultiFab> computePhi (int) = 0;
    virtual void applyFilterMF (ablastr::fields::MultiLevelVectorField const&, int, int) = 0;
};
}
#endif
