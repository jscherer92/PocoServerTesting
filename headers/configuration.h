#include "Poco/Timestamp.h"

namespace TinyserveConfiguration {
    static const struct Cache {
        long size;
        Poco::Timestamp::TimeDiff time;
    };
    static Cache cache;

    static const struct SSL {
        std::string key;
    };
    static SSL ssl;

    static const Poco::UInt16 port;

    static const int maxQueued;
    
    static const int maxThreads;

    static const struct Host {
        std::string location;
        std::string defaultRoot;
    };
    static Host host;

    // will take in a file and try to populate all of the namespace members
    void populateConfiguration(std::string configFileLocation);
}