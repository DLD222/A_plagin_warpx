#ifndef WARPX_RUNTIME_IPML_H_
#define WARPX_RUNTIME_IPML_H_
#include <ablastr/fields/MultiFabRegister.H>
#include <string>
#include <vector>
namespace warpx::runtime {
class IPML {
public:
    virtual ~IPML () = default;
    virtual bool doPML () const = 0;
    virtual void checkpointPML (int, const std::string&, ablastr::fields::MultiFabRegister&) = 0;
    virtual std::vector<bool> pmlDirections () const = 0;
};
}
#endif
