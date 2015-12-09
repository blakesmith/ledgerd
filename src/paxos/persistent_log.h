#ifndef LEDGERD_PAXOS_PERSISTENT_LOG_H_
#define LEDGERD_PAXOS_PERSISTENT_LOG_H_

namespace ledgerd {
namespace paxos {

enum LogStatus {
    LOG_OK
};

template <typename T>
class PersistentLog {
public:
    virtual LogStatus Write(const Instance<T>* instance) = 0;
};

}
}

#endif
