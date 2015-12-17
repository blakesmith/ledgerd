#ifndef LEDGERD_PAXOS_PERSISTENT_LOG_H_
#define LEDGERD_PAXOS_PERSISTENT_LOG_H_

namespace ledgerd {
namespace paxos {

enum LogStatus {
    LOG_OK,
    LOG_ERR
};

template <typename T>
class PersistentLog {
public:
    virtual LogStatus Write(uint64_t sequence, const T* final_value) = 0;
    virtual std::unique_ptr<T> Get(uint64_t sequence) = 0;
    virtual uint64_t HighestSequence() = 0;
};

}
}

#endif
