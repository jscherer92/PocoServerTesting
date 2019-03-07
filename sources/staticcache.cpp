#include "staticcache.h"
#include "Poco/AccessExpireCache.h"
#include "Poco/FileStream.h"
#include "Poco/Timestamp.h"

Poco::SharedPtr<Poco::FileStreamBuf> StaticCache::getData(std::string uri)
{
    auto data = cache->get(uri);
    if( data.isNull() ) {
        return nullptr;
    } else {
        return data;
    }
};

void StaticCache::setData(std::string uri, std::unique_ptr<Poco::FileStreamBuf> data) {
    cache->add(uri, *data);
};

void StaticCache::clearCache() {
    cache->clear();
}