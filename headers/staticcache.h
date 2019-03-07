#include "Poco/AccessExpireCache.h"
#include "Poco/FileStream.h"
#include "Poco/Timestamp.h"

class StaticCache {
public:
    StaticCache(long size, Poco::Timestamp::TimeDiff expire) : expireTime(expire), cacheSize(size)
    {
        cache = new Poco::AccessExpireCache<std::string, Poco::FileStreamBuf>(static_cast<Poco::Int64>(expireTime));
    };
    ~StaticCache()
    {
        delete cache;
    }
    Poco::SharedPtr<Poco::FileStreamBuf> getData(std::string path);
    void setData(std::string path, std::unique_ptr<Poco::FileStreamBuf> data); //that way we get a copy and we can take control of it
    void clearCache();
private:
    Poco::Timestamp::TimeDiff expireTime;
    long cacheSize;
    Poco::AccessExpireCache<std::string, Poco::FileStreamBuf>* cache;
}