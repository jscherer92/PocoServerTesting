#include "configuration.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/Timestamp.h"
#include "Poco/JSON/Parser.h"
#include "Poco/Dynamic/Var.h"

#include <sstream>
#include <iostream>

using namespace std;

namespace TinyserveConfiguration {
    void populateConfiguration(string configFile) 
    {
        Poco::File config(configFile);
        if( config.exists() && config.canRead() && config.isFile() ) {
            Poco::FileInputStream fis(configFile);
            stringstream ssr;
            ssr << fis.rdbuf();
            string config = ssr.str();
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var result = parser.parse(config);
            Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();

            // now lets read these contents into the various variables that we have ready for it
            object->get("port").convert(port);
            object->get("maximum_queued").convert(maxQueued);
            object->get("maximum_threads").convert(maxThreads);

            Poco::JSON::Object::Ptr hostInfo = object->getObject("host");
            host.location = hostInfo->get("static_folder").toString();
            host.defaultRoot = hostInfo->get("default_root").toString();

            Poco::JSON::Object::Ptr cacheInfo = object->getObject("cache");
            cacheInfo->get("size").convert(cache.size);
            cacheInfo->get("time").convert(cache.time);

            Poco::JSON::Object::Ptr sslInfo = object->getObject("ssl");
            sslInfo->get("file").convert(ssl.key);
        }
    }
}