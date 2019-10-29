#ifndef _ICP_PACKETIO_DPDK_PORT_FILTER_H_
#define _ICP_PACKETIO_DPDK_PORT_FILTER_H_

#include <optional>
#include <variant>
#include <vector>
#include <unordered_map>

#include "core/icp_log.h"
#include "net/net_types.h"
#include "packetio/generic_interface.h"

struct rte_flow;

namespace icp::packetio::dpdk::port {

/**
 * Filter states
 */
struct filter_state_ok {};        /*<< Filter is enabled and working */
struct filter_state_overflow {};  /*<< Port is promiscuous due to filter overflow */
struct filter_state_disabled {};  /*<< Port is promiscuous due to administrative event */
struct filter_state_error {};     /*<< Filter returned an error; no further modifications are possible */

using filter_state = std::variant<filter_state_ok,
                                  filter_state_overflow,
                                  filter_state_disabled,
                                  filter_state_error>;

/**
 * Filter events
 */
struct filter_event_add {
    net::mac_address mac;
};

struct filter_event_del {
    net::mac_address mac;
};

struct filter_event_disable {};
struct filter_event_enable {};

using filter_event = std::variant<filter_event_add,
                                  filter_event_del,
                                  filter_event_disable,
                                  filter_event_enable>;

template <typename Derived, typename StateVariant, typename EventVariant>
class filter_state_machine
{
    StateVariant m_state;

public:
    void handle_event(const EventVariant& event)
    {
        Derived& child = static_cast<Derived&>(*this);
        auto next_state = std::visit(
            [&](const auto& action, const auto& state) {
                return (child.on_event(action, state));
            }, event, m_state);

        if (next_state) {
            ICP_LOG(ICP_LOG_TRACE, "Port Filter %u: %s --> %s\n",
                    reinterpret_cast<Derived&>(*this).port_id(),
                    to_string(m_state).data(),
                    to_string(*next_state).data());
            m_state = *std::move(next_state);
        }
    }
};

class mac_filter : public filter_state_machine<mac_filter, filter_state, filter_event>
{
    uint16_t m_port;
    std::vector<net::mac_address> m_filtered;
    std::vector<net::mac_address> m_overflowed;

public:
    mac_filter(uint16_t port_id);

    uint16_t port_id() const;

    std::optional<filter_state> on_event(const filter_event_add& add, const filter_state_ok&);
    std::optional<filter_state> on_event(const filter_event_del& del, const filter_state_ok&);
    std::optional<filter_state> on_event(const filter_event_disable&, const filter_state_ok&);

    std::optional<filter_state> on_event(const filter_event_add& add, const filter_state_overflow&);
    std::optional<filter_state> on_event(const filter_event_del& del, const filter_state_overflow&);

    std::optional<filter_state> on_event(const filter_event_add& add, const filter_state_disabled&);
    std::optional<filter_state> on_event(const filter_event_del& del, const filter_state_disabled&);
    std::optional<filter_state> on_event(const filter_event_enable&, const filter_state_disabled&);

    /* Default action for any unhandled cases */
    template <typename Event, typename State>
    std::optional<filter_state> on_event(const Event&, const State&)
    {
        return (std::nullopt);
    }
};

class filter
{
public:
    filter(uint16_t port_id);

    uint16_t port_id() const;

    void add_mac_address(const net::mac_address& mac);
    void del_mac_address(const net::mac_address& mac);

    void enable();
    void disable();

    using filter_strategy = std::variant<mac_filter>;

private:
    filter_strategy m_filter;
};

const char * to_string(const filter_state&);

}

#endif /* _ICP_PACKETIO_DPDK_PORT_FILTER_H_ */
