#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Buffer.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/LineEndingConverter.h"
#include "Poco/JSON/Parser.h"
#include "Poco/Dynamic/Var.h"
#include "Poco/URI.h"
#include "Poco/Path.h"
#include "Poco/ExpireLRUCache.h"
#include "Poco/Timestamp.h"
#include "Poco/Net/SecureServerSocket.h"
#include "Poco/Net/Context.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Application.h"
#include "Poco/Exception.h"
#include <exception>
#include <sstream>
#include <iostream>

struct CacheSetup {
    long size;
    Poco::Timestamp::TimeDiff time;
};

class RootHandler: public Poco::Net::HTTPRequestHandler
{
public:
    RootHandler(std::string staticHostLocation) : defaultRoot("index.html") 
    {
        staticHost = staticHostLocation;
    }
    RootHandler(std::string staticHostLocation, std::string root)
    {
        staticHost = staticHostLocation;
        defaultRoot = root;
    }
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
    {
        Poco::URI uri(request.getURI());
        std::cout << uri.getPath() << std::endl;
        std::cout << request.getMethod() << std::endl;
        Poco::Path path(staticHost,  (std::string("/").compare(uri.getPath()) == 0) ? std::string("/" + defaultRoot) : uri.getPath());
        std::cout << path.getExtension() << std::endl;
        Poco::File content(path);
        
        if( content.exists() && content.isFile() && content.canRead() ) {
            response.setChunkedTransferEncoding(true);
            response.setContentType(getContentType(path.getExtension()));
            std::ostream& ostr = response.send();
            Poco::FileInputStream fis(path.toString());
            ostr << fis.rdbuf();
        } else {
            response.setChunkedTransferEncoding(true);
            response.setContentType(getContentType(path.getExtension()));
            response.setStatus("404");
            std::ostream& ostr = response.send();
            ostr << "404: File Not Found!";
        }
    }
private:
    //Todo: this should be a map lookup so it is an O(1) time
    std::string getContentType(std::string fileExtension) {
        if(fileExtension.compare("html") == 0) return std::string("text/html");
        if(fileExtension.compare("css") == 0) return std::string("text/css");
        if(fileExtension.compare("js") == 0) return std::string("text/javacsript");
        if(fileExtension.compare("gif") == 0) return std::string("image/gif");
        if(fileExtension.compare("ico") == 0) return std::string("image/vnd.microsoft.icon");
        if(fileExtension.compare("jpeg") == 0 || fileExtension.compare("jpg") == 0) return std::string("image/jpeg");
        if(fileExtension.compare("json") == 0) return std::string("application/json");
        if(fileExtension.compare("svg") == 0) return std::string("image/svg+xml");
        return std::string("text/plain");
    };
    std::string staticHost;
    std::string defaultRoot;
};

class WebSocketHandler: public Poco::Net::HTTPRequestHandler
{
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
    {
        Poco::Net::WebSocket ws(request, response);
        int num;
        int flags;
        char buffer[1024];
        do
        {
            num = ws.receiveFrame(buffer, sizeof(buffer), flags);
            std::cout << Poco::format("Frame received (length %d, flags=0x%x).", num, unsigned(flags));
            ws.sendFrame(buffer, num, flags);
        } while (num > 0 && (flags & Poco::Net::WebSocket::FRAME_OP_BITMASK) != Poco::Net::WebSocket::FRAME_OP_CLOSE);
    }
};

class MyRequestHandlerFactory: public Poco::Net::HTTPRequestHandlerFactory
{
public:
    MyRequestHandlerFactory() : staticLocation("static"), baseDir("static"), defaultRoot("index.html")
    {
    }

    MyRequestHandlerFactory(std::string staticLoc)
    {
        staticLocation = staticLoc;
        Poco::File staticDirectory(staticLoc);
    }

    MyRequestHandlerFactory(std::string staticLoc, std::string root)
    {
        staticLocation = staticLoc;
        defaultRoot = root;
        Poco::File staticDirectory(staticLoc);
    }

    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request)
    {
        std::cout << "request came in!" << std::endl;
        if(request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0)
            return new WebSocketHandler();
        else {
            return new RootHandler(staticLocation, defaultRoot);
        }
            
    }
private:
    Poco::File baseDir;
    std::string staticLocation;
    std::string defaultRoot;
};

Poco::JSON::Object::Ptr readServerConfiguration(std::string filename) {
    Poco::File config(filename);
    if( config.exists() && config.canRead() && config.isFile() )
    {
        Poco::FileInputStream fis(filename);
        std::stringstream ssr;
        ssr << fis.rdbuf();
        std::string config = ssr.str();
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(config);
        Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();
        return object;
    }
    throw std::runtime_error("Configuration Not Found!");
};

int main(int argc, char** argv)
{
    Poco::JSON::Object::Ptr object = readServerConfiguration("config/startup.json");
    Poco::UInt16 port;
    int maxQueued;
    int maxThreads;
    Poco::Timestamp::TimeDiff maxCacheTime;
    long maxCacheLength;
    object->get("port").convert(port);
    object->get("max_queued").convert(maxQueued);
    object->get("max_threads").convert(maxThreads);
    object->get("cache_time_length").convert(maxCacheTime);
    object->get("cache_max_entries").convert(maxCacheLength);
    std::string staticLocation = object->get("static_files").toString();
    std::string defaultRoot = object->get("default_root").toString();

    Poco::Net::HTTPServerParams* params = new Poco::Net::HTTPServerParams();
    params->setMaxQueued(maxQueued);
    params->setMaxThreads(maxThreads);

    CacheSetup cSetup;
    cSetup.size = maxCacheLength;
    cSetup.time = maxCacheTime;
    
    Poco::Net::initializeSSL();
    Poco::Net::SocketAddress socketAddr("127.0.0.1", 8080);
    Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::SERVER_USE, "cert.pem");
    std::cout << context.isNull() << std::endl;

    //Todo: implement as ServerApplication so we can use the SSLManager
    try {
    Poco::Net::SecureServerSocket svs(port);
    } catch(Poco::Exception e) {
        std::cout << e.name() << e.message() << std::endl;
        //std::cout << svs.available() << std::endl;
    }
    

    //Poco::Net::HTTPServer srv(new MyRequestHandlerFactory(staticLocation, defaultRoot), svs, params);

    //srv.start();

    for(;;){};

    //srv.stop();
    Poco::Net::uninitializeSSL();

    return 0;
}