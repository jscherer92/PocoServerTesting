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

#include <exception>
#include <sstream>
#include <iostream>

class RootHandler: public Poco::Net::HTTPRequestHandler
{
public:
    RootHandler(std::string staticHostLocation) {
        staticHost = staticHostLocation;
    }
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
    {
        Poco::URI uri(request.getURI());
        std::cout << uri.getPath() << std::endl;
        Poco::Path path(staticHost,  (std::string("/").compare(uri.getPath()) == 0) ? std::string("/index.html") : uri.getPath());
        Poco::File content(path);
        
        if( content.exists() && content.isFile() && content.canRead() ) {
            response.setChunkedTransferEncoding(true);
            // this needs to be handled better
            response.setContentType("text/html");
            std::ostream& ostr = response.send();
            Poco::FileInputStream fis(path.toString());
            ostr << fis.rdbuf();
        } else {
            response.setChunkedTransferEncoding(true);
            response.setContentType("text/html");
            response.setStatus("404");
            std::ostream& ostr = response.send();
            ostr << "404: File Not Found!";
        }
    }
private:
    std::string staticHost;
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
    MyRequestHandlerFactory() : staticLocation("static"), baseDir("static")
    {
    }

    MyRequestHandlerFactory(std::string staticLoc)
    {
        staticLocation = staticLoc;
        Poco::File staticDirectory(staticLoc);
    }

    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request)
    {
        if(request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0)
            return new WebSocketHandler();
        else {
            return new RootHandler(staticLocation);
        }
            
    }
private:
    Poco::File baseDir;
    std::string staticLocation;
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
    object->get("port").convert(port);
    object->get("max_queued").convert(maxQueued);
    object->get("max_threads").convert(maxThreads);
    std::string staticLocation = object->get("static_files").toString();

    Poco::Net::HTTPServerParams* params = new Poco::Net::HTTPServerParams();
    params->setMaxQueued(maxQueued);
    params->setMaxThreads(maxThreads);

    Poco::Net::ServerSocket svs(port);

    Poco::Net::HTTPServer srv(new MyRequestHandlerFactory(staticLocation), svs, params);

    srv.start();

    for(;;) {}

    srv.stop();

    return 0;
}