#ifndef LEDGERD_PAXOS_LISTENER_H_
#define LEDGERD_PAXOS_LISTENER_H_

namespace ledgerd {
namespace paxos {

enum class ListenerStatus {
    OK,
    ERR,
    NONE
};

template <typename T, typename V>
class Listener {
public:
    virtual ListenerStatus Receive(uint64_t sequence, const T* final_value) = 0;
    virtual ListenerStatus Map(const T* event_value, V* mapped_value) = 0;
    virtual uint64_t HighestSequence() = 0;
};

}
}

#endif
