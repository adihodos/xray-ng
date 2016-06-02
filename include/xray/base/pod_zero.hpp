#ifndef xray_base_pod_zero_hpp__
#define xray_base_pod_zero_hpp__

#include <cstring>
#include <type_traits>

#include "xray/xray.hpp"

namespace xray {
namespace base {

/// \brief      Automatically set a POD type to 0 when creating an object
///             of this type.
template<typename pod_type>
struct pod_zero : public pod_type {
    
    static_assert(std::is_pod<pod_type>::value, "Only POD types allowed!");
    
    pod_zero() noexcept {
        memset(this, 0, sizeof(*this));
    }
};

}   // namespace base 
}   // namespace xray

#endif /* !defined xray_base_pod_zero_hpp__ */