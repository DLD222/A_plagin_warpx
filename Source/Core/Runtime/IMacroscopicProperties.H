#ifndef WARPX_RUNTIME_IMACROSCOPICPROPERTIES_H_
#define WARPX_RUNTIME_IMACROSCOPICPROPERTIES_H_
#include "Runtime/MaterialRegistry.H"
namespace warpx::runtime {
/** Macroscopic-medium plugin contract (A7.3.2d). */
class IMacroscopicProperties
{
public:
    virtual ~IMacroscopicProperties () = default;
    [[nodiscard]] virtual const MaterialRegistry& materials () const = 0;
};
}
#endif
