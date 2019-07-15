#ifndef _ICP_PACKETIO_DPDK_EAL_H_
#define _ICP_PACKETIO_DPDK_EAL_H_

#include <any>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "packetio/generic_driver.h"
#include "packetio/generic_port.h"
#include "packetio/memory/dpdk/pool_allocator.h"

namespace icp {
namespace packetio {

namespace port {
class generic_port;
}

namespace dpdk {

class eal
{
public:
    /* named constructors */
    static eal test_environment(std::vector<std::string> args,
                                std::unordered_map<int, std::string> port_ids,
                                unsigned test_portpairs);

    static eal real_environment(std::vector<std::string> args,
                                std::unordered_map<int, std::string> port_ids);

    ~eal();

    /* environment is movable */
    eal& operator=(eal&& other);
    eal(eal&& other);

    /* environment is non-copyable */
    eal(const eal&) = delete;
    eal& operator=(const eal&&) = delete;

    std::vector<std::string> port_ids() const;
    std::optional<port::generic_port> port(std::string_view id) const;
    std::optional<int> port_index(std::string_view id) const;

    tl::expected<std::string, std::string> create_port(std::string_view id, const port::config_data& config);
    tl::expected<void, std::string> delete_port(std::string_view id);

    tl::expected<void, int> attach_port_sink(std::string_view id, pga::generic_sink& sink);
    void detach_port_sink(std::string_view id, pga::generic_sink& sink);

    tl::expected<void, int> attach_port_source(std::string_view id, pga::generic_source& source);
    void detach_port_source(std::string_view id, pga::generic_source& source);

private:
    bool m_initialized;
    std::unique_ptr<pool_allocator> m_allocator;
    std::unordered_map<int, std::string> m_bond_ports;
    std::unordered_map<int, std::string> m_port_ids;

    eal(std::vector<std::string> args,
        std::unordered_map<int, std::string> port_ids,
        unsigned test_portpairs = 0);

    void start() const;
    void stop() const;
};

}
}
}

#endif /* _ICP_PACKETIO_DPDK_EAL_H_ */
