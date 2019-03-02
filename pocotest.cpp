#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Buffer.h"

#include <iostream>

class RootHandler: public Poco::Net::HTTPRequestHandler
{
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
    {
        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        std::ostream& ostr = response.send();

        ostr << "<html><head><title>HTTP Server powered by POCO C++ Libraries</title>";
        ostr << "<script>";
        ostr << "function WebSocketTest()";
		ostr << "{";
		ostr << "  if (\"WebSocket\" in window)";
		ostr << "  {";
		ostr << "    var ws = new WebSocket(\"ws://" << request.serverAddress().toString() << "/ws\");";
		ostr << "    ws.onopen = function()";
		ostr << "      {";
		ostr << "        ws.send(\"Hello, world!\");";
		ostr << "      };";
		ostr << "    ws.onmessage = function(evt)";
		ostr << "      { ";
		ostr << "        var msg = evt.data;";
		ostr << "        alert(\"Message received: \" + msg);";
		ostr << "        ws.close();";
		ostr << "      };";
		ostr << "    ws.onclose = function()";
		ostr << "      { ";
		ostr << "        alert(\"WebSocket closed.\");";
		ostr << "      };";
		ostr << "  }";
		ostr << "  else";
		ostr << "  {";
		ostr << "     alert(\"This browser does not support WebSockets.\");";
		ostr << "  }";
		ostr << "}";
        ostr << "</script>";
        ostr << "</head>";
        ostr << "<body>";
        ostr << "<h1>POCO server up and running!</h1>";
        ostr << "</body></html>";
    }
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
    MyRequestHandlerFactory()
    {
    }

    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request)
    {
        if(request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0)
            return new WebSocketHandler();
        else
            return new RootHandler();
    }
};

int main(int argc, char** argv)
{
    Poco::UInt16 port = 9999;
    Poco::Net::HTTPServerParams* params = new Poco::Net::HTTPServerParams();
    params->setMaxQueued(100);
    params->setMaxThreads(16);

    Poco::Net::ServerSocket svs(port);

    Poco::Net::HTTPServer srv(new MyRequestHandlerFactory(), svs, params);

    srv.start();

    for(;;) {}

    srv.stop();

    return 0;
}