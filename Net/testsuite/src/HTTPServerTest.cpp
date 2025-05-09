//
// HTTPServerTest.cpp
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "HTTPServerTest.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/AbstractHTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerRequestImpl.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerSession.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/StreamCopier.h"
#include "Poco/Path.h"
#include "Poco/FileStream.h"
#include "Poco/File.h"
#include <sstream>


using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::AbstractHTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::ServerSocket;
using Poco::StreamCopier;
using Poco::Path;
using Poco::File;
using Poco::FileOutputStream;


namespace
{
	static const int sendFileSize = 64000;

	class EchoBodyRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			if (request.getChunkedTransferEncoding())
				response.setChunkedTransferEncoding(true);
			else if (request.getContentLength() != HTTPMessage::UNKNOWN_CONTENT_LENGTH)
				response.setContentLength(request.getContentLength());

			response.setContentType(request.getContentType());

			std::istream& istr = request.stream();
			std::ostream& ostr = response.send();
			StreamCopier::copyStream(istr, ostr);
		}
	};

	class EchoHeaderRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			std::ostringstream osstr;
			request.write(osstr);
			int n = (int) osstr.str().length();
			response.setContentLength(n);
			std::ostream& ostr = response.send();
			if (request.getMethod() != HTTPRequest::HTTP_HEAD)
				request.write(ostr);
		}
	};

	class RedirectRequestHandler: public AbstractHTTPRequestHandler
	{
	public:
		void run()
		{
			response().redirect("http://www.appinf.com/");
		}
	};

	class AuthRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			response.requireAuthentication("/auth");
			response.send();
		}
	};

	class BufferRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			std::string data("xxxxxxxxxx");
			response.sendBuffer(data.data(), data.length());
		}
	};
	
	class FileRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			std::string payload(sendFileSize, 'x');
			Poco::Path testFilePath = Poco::Path::temp().append("test.http.server.sendfile.txt");
			const std::string fileName = testFilePath.toString();
			{
				File f(fileName);
				if (f.exists())
				{
					f.remove();
				}
			}
			FileOutputStream fout(fileName);
			fout << payload;
			fout.close();
			response.sendFile(fileName, "text/plain");
		}
	};
	
	class TrailerRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			std::string data("xxxxxxxxxx");

			response.setChunkedTransferEncoding(true);
			Poco::Net::MessageHeader& trailer = static_cast<Poco::Net::HTTPServerRequestImpl&>(request).session().responseTrailer();
			trailer.set("Trailer-1", "Value 1");
			trailer.set("Trailer-2", "Value 2");
			response.send() << data;
		}
	};

	class RequestHandlerFactory: public HTTPRequestHandlerFactory
	{
	public:
		HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
		{
			if (request.getURI() == "/echoBody")
				return new EchoBodyRequestHandler;
			else if (request.getURI() == "/echoHeader")
				return new EchoHeaderRequestHandler;
			else if (request.getURI() == "/redirect")
				return new RedirectRequestHandler;
			else if (request.getURI() == "/auth")
				return new AuthRequestHandler;
			else if (request.getURI() == "/buffer")
				return new BufferRequestHandler;
			else if (request.getURI() == "/trailer")
				return new TrailerRequestHandler;
			else if (request.getURI() == "/file")
				return new FileRequestHandler;
			else
				return nullptr;
		}
	};
}


HTTPServerTest::HTTPServerTest(const std::string& name): CppUnit::TestCase(name)
{
}


HTTPServerTest::~HTTPServerTest()
{
}


void HTTPServerTest::testIdentityRequest()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody");
	request.setContentLength((int) body.length());
	request.setContentType("text/plain");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getContentLength() == body.size());
	assertTrue (response.getContentType() == "text/plain");
	assertTrue (rbody == body);
}


void HTTPServerTest::testPutIdentityRequest()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("PUT", "/echoBody");
	request.setContentLength((int) body.length());
	request.setContentType("text/plain");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getContentLength() == body.size());
	assertTrue (response.getContentType() == "text/plain");
	assertTrue (rbody == body);
}


void HTTPServerTest::testChunkedRequest()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody");
	request.setContentType("text/plain");
	request.setChunkedTransferEncoding(true);
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
	assertTrue (response.getContentType() == "text/plain");
	assertTrue (response.getChunkedTransferEncoding());
	assertTrue (rbody == body);
}


void HTTPServerTest::testIdentityRequestKeepAlive()
{
	HTTPServer srv(new RequestHandlerFactory, 8008);
	srv.start();

	HTTPClientSession cs("127.0.0.1", srv.socket().address().port());
	cs.setKeepAlive(true);
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody", HTTPMessage::HTTP_1_1);
	request.setContentLength((int) body.length());
	request.setContentType("text/plain");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getContentLength() == body.size());
	assertTrue (response.getContentType() == "text/plain");
	assertTrue (response.getKeepAlive());
	assertTrue (rbody == body);

	body.assign(1000, 'y');
	request.setContentLength((int) body.length());
	request.setKeepAlive(false);
	cs.sendRequest(request) << body;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getContentLength() == body.size());
	assertTrue (response.getContentType() == "text/plain");
	assertTrue (!response.getKeepAlive());
	assertTrue (rbody == body);}


void HTTPServerTest::testChunkedRequestKeepAlive()
{
	HTTPServer srv(new RequestHandlerFactory, 8009);
	srv.start();

	HTTPClientSession cs("127.0.0.1", srv.socket().address().port());
	cs.setKeepAlive(true);
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody", HTTPMessage::HTTP_1_1);
	request.setContentType("text/plain");
	request.setChunkedTransferEncoding(true);
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
	assertTrue (response.getContentType() == "text/plain");
	assertTrue (response.getChunkedTransferEncoding());
	assertTrue (rbody == body);

	body.assign(1000, 'y');
	request.setKeepAlive(false);
	cs.sendRequest(request) << body;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
	assertTrue (response.getContentType() == "text/plain");
	assertTrue (response.getChunkedTransferEncoding());
	assertTrue (!response.getKeepAlive());
	assertTrue (rbody == body);
}


void HTTPServerTest::testMaxKeepAlive()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(true);
	pParams->setMaxKeepAliveRequests(4);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	cs.setKeepAlive(true);
	HTTPRequest request("POST", "/echoBody", HTTPMessage::HTTP_1_1);
	request.setContentType("text/plain");
	request.setChunkedTransferEncoding(true);
	std::string body(5000, 'x');
	for (int i = 0; i < 3; ++i)
	{
		cs.sendRequest(request) << body;
		HTTPResponse response;
		std::string rbody;
		cs.receiveResponse(response) >> rbody;
		assertTrue (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
		assertTrue (response.getContentType() == "text/plain");
		assertTrue (response.getChunkedTransferEncoding());
		assertTrue (response.getKeepAlive());
		assertTrue (rbody == body);
	}

	{
		cs.sendRequest(request) << body;
		HTTPResponse response;
		std::string rbody;
		cs.receiveResponse(response) >> rbody;
		assertTrue (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
		assertTrue (response.getContentType() == "text/plain");
		assertTrue (response.getChunkedTransferEncoding());
		assertTrue (!response.getKeepAlive());
		assertTrue (rbody == body);
	}

	{
		cs.setKeepAlive(false);
		cs.sendRequest(request) << body;
		HTTPResponse response;
		std::string rbody;
		cs.receiveResponse(response) >> rbody;
		assertTrue (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
		assertTrue (response.getContentType() == "text/plain");
		assertTrue (response.getChunkedTransferEncoding());
		assertTrue (!response.getKeepAlive());
		assertTrue (rbody == body);
	}
}


void HTTPServerTest::testKeepAliveTimeout()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(true);
	pParams->setMaxKeepAliveRequests(4);
	pParams->setKeepAliveTimeout(Poco::Timespan(3, 0));
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	cs.setKeepAlive(true);
	cs.setKeepAliveTimeout(Poco::Timespan(2, 0));
	HTTPRequest request("POST", "/echoBody", HTTPMessage::HTTP_1_1);
	request.setContentType("text/plain");
	request.setChunkedTransferEncoding(true);
	std::string body(5000, 'x');
	for (int i = 0; i < 3; ++i)
	{
		cs.sendRequest(request) << body;
		HTTPResponse response;
		std::string rbody;
		cs.receiveResponse(response) >> rbody;
		assertTrue (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
		assertTrue (response.getContentType() == "text/plain");
		assertTrue (response.getChunkedTransferEncoding());
		assertTrue (response.getKeepAlive());
		assertTrue (rbody == body);
	}

	Poco::Thread::sleep(4000);

	{
		cs.sendRequest(request) << body;
		HTTPResponse response;
		std::string rbody;
		cs.receiveResponse(response) >> rbody;
		assertTrue (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
		assertTrue (response.getContentType() == "text/plain");
		assertTrue (response.getChunkedTransferEncoding());
		assertTrue (response.getKeepAlive());
		assertTrue (rbody == body);
	}
}


void HTTPServerTest::test100Continue()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody");
	request.setContentLength((int) body.length());
	request.setContentType("text/plain");
	request.set("Expect", "100-Continue");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getContentLength() == body.size());
	assertTrue (response.getContentType() == "text/plain");
	assertTrue (rbody == body);
}


void HTTPServerTest::testRedirect()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	HTTPRequest request("GET", "/redirect");
	cs.sendRequest(request);
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getStatus() == HTTPResponse::HTTP_FOUND);
	assertTrue (response.get("Location") == "http://www.appinf.com/");
	assertTrue (rbody.empty());
}


void HTTPServerTest::testAuth()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	HTTPRequest request("GET", "/auth");
	cs.sendRequest(request);
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getStatus() == HTTPResponse::HTTP_UNAUTHORIZED);
	assertTrue (response.get("WWW-Authenticate") == "Basic realm=\"/auth\"");
	assertTrue (rbody.empty());
}


void HTTPServerTest::testNotImpl()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	HTTPRequest request("GET", "/notImpl");
	cs.sendRequest(request);
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getStatus() == HTTPResponse::HTTP_NOT_IMPLEMENTED);
	assertTrue (rbody.empty());
}


void HTTPServerTest::testBuffer()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	HTTPRequest request("GET", "/buffer");
	cs.sendRequest(request);
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getStatus() == HTTPResponse::HTTP_OK);
	assertTrue (rbody == "xxxxxxxxxx");
}

void HTTPServerTest::testFile()
{
	std::string payload(sendFileSize, 'x');
	
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("127.0.0.1", svs.address().port());
	HTTPRequest request("GET", "/file");
	cs.sendRequest(request);
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getStatus() == HTTPResponse::HTTP_OK);
	assertTrue (rbody == payload);
}


void HTTPServerTest::testChunkedTrailer()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();

	HTTPClientSession cs("127.0.0.1", svs.address().port());
	HTTPRequest request("GET", "/trailer");
	cs.sendRequest(request);
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assertTrue (response.getStatus() == HTTPResponse::HTTP_OK);
	assertTrue (rbody == "xxxxxxxxxx");
	assertTrue (!cs.responseTrailer().empty());
	assertTrue (cs.responseTrailer().get("Trailer-1") == "Value 1");
	assertTrue (cs.responseTrailer().get("Trailer-2") == "Value 2");
}


void HTTPServerTest::setUp()
{
}


void HTTPServerTest::tearDown()
{
}


CppUnit::Test* HTTPServerTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("HTTPServerTest");

	CppUnit_addTest(pSuite, HTTPServerTest, testIdentityRequest);
	CppUnit_addTest(pSuite, HTTPServerTest, testPutIdentityRequest);
	CppUnit_addTest(pSuite, HTTPServerTest, testChunkedRequest);
	CppUnit_addTest(pSuite, HTTPServerTest, testIdentityRequestKeepAlive);
	CppUnit_addTest(pSuite, HTTPServerTest, testChunkedRequestKeepAlive);
	CppUnit_addTest(pSuite, HTTPServerTest, testMaxKeepAlive);
	CppUnit_addTest(pSuite, HTTPServerTest, testKeepAliveTimeout);
	CppUnit_addTest(pSuite, HTTPServerTest, test100Continue);
	CppUnit_addTest(pSuite, HTTPServerTest, testRedirect);
	CppUnit_addTest(pSuite, HTTPServerTest, testAuth);
	CppUnit_addTest(pSuite, HTTPServerTest, testNotImpl);
	CppUnit_addTest(pSuite, HTTPServerTest, testBuffer);
	CppUnit_addTest(pSuite, HTTPServerTest, testFile);
	CppUnit_addTest(pSuite, HTTPServerTest, testChunkedTrailer);

	return pSuite;
}
