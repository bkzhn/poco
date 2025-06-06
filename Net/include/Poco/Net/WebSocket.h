//
// WebSocket.h
//
// Library: Net
// Package: WebSocket
// Module:  WebSocket
//
// Definition of the WebSocket class.
//
// Copyright (c) 2012, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#ifndef Net_WebSocket_INCLUDED
#define Net_WebSocket_INCLUDED


#include "Poco/Net/Net.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/HTTPCredentials.h"
#include "Poco/Buffer.h"


namespace Poco {
namespace Net {


class WebSocketImpl;
class HTTPServerRequest;
class HTTPServerResponse;
class HTTPClientSession;


class Net_API WebSocket: public StreamSocket
	/// This class implements a WebSocket according
	/// to the WebSocket protocol specification in RFC 6455.
	///
	/// Both client-side and server-side WebSockets
	/// are supported.
	///
	/// Server-side WebSockets are usually created from within
	/// a HTTPRequestHandler.
	///
	/// Client-side WebSockets are created using a HTTPClientSession.
	///
	/// Note that special frames like PING must be handled at
	/// application level. In the case of a PING, a PONG message
	/// must be returned.
	///
	/// Once connected, a WebSocket can be put into non-blocking
	/// mode, by calling setBlocking(false). 
	/// Please refer to the sendFrame() and receiveFrame() documentation
	/// for non-blocking behavior.
{
public:
	enum Mode
	{
		WS_SERVER, /// Server-side WebSocket.
		WS_CLIENT  /// Client-side WebSocket.
	};

	enum FrameFlags
		/// Frame header flags.
	{
		FRAME_FLAG_FIN  = 0x80, /// FIN bit: final fragment of a multi-fragment message.
		FRAME_FLAG_RSV1 = 0x40, /// Reserved for future use. Must be zero.
		FRAME_FLAG_RSV2 = 0x20, /// Reserved for future use. Must be zero.
		FRAME_FLAG_RSV3 = 0x10  /// Reserved for future use. Must be zero.
	};

	enum FrameOpcodes
		/// Frame header opcodes.
	{
		FRAME_OP_CONT    = 0x00, /// Continuation frame.
		FRAME_OP_TEXT    = 0x01, /// Text frame.
		FRAME_OP_BINARY  = 0x02, /// Binary frame.
		FRAME_OP_CLOSE   = 0x08, /// Close connection.
		FRAME_OP_PING    = 0x09, /// Ping frame.
		FRAME_OP_PONG    = 0x0a, /// Pong frame.
		FRAME_OP_BITMASK = 0x0f, /// Bit mask for opcodes.
		FRAME_OP_SETRAW  = 0x100 /// Set raw flags (for use with sendBytes() and FRAME_OP_CONT).
	};

	enum SendFlags
		/// Combined header flags and opcodes for use with sendFrame().
	{
                FRAME_TEXT   = static_cast<int>(FRAME_FLAG_FIN) | static_cast<int>(FRAME_OP_TEXT),
			/// Use this for sending a single text (UTF-8) payload frame.
                FRAME_BINARY = static_cast<int>(FRAME_FLAG_FIN) | static_cast<int>(FRAME_OP_BINARY)
			/// Use this for sending a single binary payload frame.
	};

	enum StatusCodes
		/// StatusCodes for CLOSE frames sent with shutdown().
	{
		WS_NORMAL_CLOSE            = 1000,
		WS_ENDPOINT_GOING_AWAY     = 1001,
		WS_PROTOCOL_ERROR          = 1002,
		WS_PAYLOAD_NOT_ACCEPTABLE  = 1003,
		WS_RESERVED                = 1004,
		WS_RESERVED_NO_STATUS_CODE = 1005,
		WS_RESERVED_ABNORMAL_CLOSE = 1006,
		WS_MALFORMED_PAYLOAD       = 1007,
		WS_POLICY_VIOLATION        = 1008,
		WS_PAYLOAD_TOO_BIG         = 1009,
		WS_EXTENSION_REQUIRED      = 1010,
		WS_UNEXPECTED_CONDITION    = 1011,
		WS_RESERVED_TLS_FAILURE    = 1015
	};

	enum ErrorCodes
		/// These error codes can be obtained from a WebSocketException
		/// to determine the exact cause of the error.
	{
		WS_ERR_NO_HANDSHAKE                   = 1,
			/// No Connection: Upgrade or Upgrade: websocket header in handshake request.
		WS_ERR_HANDSHAKE_NO_VERSION           = 2,
			/// No Sec-WebSocket-Version header in handshake request.
		WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION  = 3,
			/// Unsupported WebSocket version requested by client.
		WS_ERR_HANDSHAKE_NO_KEY               = 4,
			/// No Sec-WebSocket-Key header in handshake request.
		WS_ERR_HANDSHAKE_ACCEPT               = 5,
			/// No Sec-WebSocket-Accept header or wrong value.
		WS_ERR_UNAUTHORIZED                   = 6,
			/// The server rejected the username or password for authentication.
		WS_ERR_PAYLOAD_TOO_BIG                = 10,
			/// Payload too big for supplied buffer.
		WS_ERR_INCOMPLETE_FRAME               = 11
			/// Incomplete frame received.
	};

	WebSocket(HTTPServerRequest& request, HTTPServerResponse& response);
		/// Creates a server-side WebSocket from within a
		/// HTTPRequestHandler.
		///
		/// First verifies that the request is a valid WebSocket upgrade
		/// request. If so, completes the handshake by sending
		/// a proper 101 response.
		///
		/// Throws an exception if the request is not a proper WebSocket
		/// upgrade request.

	WebSocket(HTTPClientSession& cs, HTTPRequest& request, HTTPResponse& response);
		/// Creates a client-side WebSocket, using the given
		/// HTTPClientSession and HTTPRequest for the initial handshake
		/// (HTTP Upgrade request).
		///
		/// Additional HTTP headers for the initial handshake request
		/// (such as Origin or Sec-WebSocket-Protocol) can be given
		/// in the request object.
		///
		/// The result of the handshake can be obtained from the response
		/// object.
		///
		/// The HTTPClientSession session object must no longer be used after setting
		/// up the WebSocket.

	WebSocket(HTTPClientSession& cs, HTTPRequest& request, HTTPResponse& response, HTTPCredentials& credentials);
		/// Creates a client-side WebSocket, using the given
		/// HTTPClientSession and HTTPRequest for the initial handshake
		/// (HTTP Upgrade request).
		///
		/// The given credentials are used for authentication
		/// if requested by the server.
		///
		/// Additional HTTP headers for the initial handshake request
		/// (such as Origin or Sec-WebSocket-Protocol) can be given
		/// in the request object.
		///
		/// The result of the handshake can be obtained from the response
		/// object.
		///
		/// The HTTPClientSession session object must no longer be used after setting
		/// up the WebSocket.

	WebSocket(const Socket& socket);
		/// Creates a WebSocket from another Socket, which must be a WebSocket,
		/// otherwise a Poco::InvalidArgumentException will be thrown.

	WebSocket(const WebSocket& socket);
		/// Creates a WebSocket from another WebSocket.

	virtual ~WebSocket();
		/// Destroys the WebSocket.

	WebSocket& operator = (const Socket& socket);
		/// Assignment operator.
		///
		/// The other socket must be a WebSocket, otherwise a Poco::InvalidArgumentException
		/// will be thrown.

	WebSocket& operator = (const WebSocket& socket);
		/// Assignment operator.

#if POCO_NEW_STATE_ON_MOVE

	WebSocket(Socket&& socket);
		/// Creates the WebSocket with the SocketImpl
		/// from another socket and zeroes the other socket's
		/// SocketImpl.The SocketImpl must be
		/// a WebSocketImpl, otherwise an InvalidArgumentException
		/// will be thrown.

	WebSocket(WebSocket&& socket);
		/// Creates the WebSocket with the SocketImpl
		/// from another socket and zeroes the other socket's
		/// SocketImpl.

	WebSocket& operator = (Socket&& socket);
		/// Assignment move operator.
		///
		/// Releases the socket's SocketImpl and
		/// attaches the SocketImpl from the other socket and
		/// zeroes the other socket's SocketImpl.

	WebSocket& operator = (WebSocket&& socket);
		/// Assignment move operator.
		///
		/// Releases the socket's SocketImpl and
		/// attaches the SocketImpl from the other socket and
		/// zeroes the other socket's SocketImpl.

#endif //POCO_NEW_STATE_ON_MOVE

	int shutdown();
		/// Sends a Close control frame to the server end of
		/// the connection to initiate an orderly shutdown
		/// of the connection.
		///
		/// Returns the number of bytes sent or -1 if the socket
		/// is non-blocking and the frame cannot be sent at this time.

	int shutdown(Poco::UInt16 statusCode, const std::string& statusMessage = "");
		/// Sends a Close control frame to the server end of
		/// the connection to initiate an orderly shutdown
		/// of the connection.
		///
		/// Returns the number of bytes sent or -1 if the socket
		/// is non-blocking and the frame cannot be sent at this time.

	int sendFrame(const void* buffer, int length, int flags = FRAME_TEXT);
		/// Sends the contents of the given buffer through
		/// the socket as a single frame.
		///
		/// Values from the FrameFlags, FrameOpcodes and SendFlags enumerations
		/// can be specified in flags.
		///
		/// Returns the number of bytes sent.
		///
		/// If the WebSocket is non-blocking and the frame could not
		/// be sent in full, returns -1. In this case, the call
		/// to sendFrame() must be repeated with exactly the same
		/// parameters as soon as the socket becomes writable again 
		/// (see select() or poll()). 
		/// The value of length is returned after the complete
		/// frame has been sent.

	int receiveFrame(void* buffer, int length, int& flags);
		/// Receives a frame from the socket and stores it
		/// in buffer. Up to length bytes are received. If
		/// the frame's payload is larger, a WebSocketException
		/// is thrown and the WebSocket connection must be
		/// terminated.
		///
		/// The frame's payload size must not exceed the
		/// maximum payload size set with setMaxPayloadSize().
		/// If it does, a WebSocketException (WS_ERR_PAYLOAD_TOO_BIG)
		/// is thrown and the WebSocket connection must be
		/// terminated.
		///
		/// A WebSocketException will also be thrown if a malformed
		/// or incomplete frame is received.
		///
		/// Returns the number of payload bytes received.
		/// A return value of 0, with flags also 0, means that the peer has
		/// shut down or closed the connection.
		/// A return value of 0, with non-zero flags, indicates an
		/// reception of an empty frame (e.g., in case of a PING).
		///
		/// Throws a TimeoutException if a receive timeout has
		/// been set and nothing is received within that interval.
		/// Throws a NetException (or a subclass) in case of other errors.
		///
		/// The frame flags and opcode (FrameFlags and FrameOpcodes)
		/// is stored in flags.
		///
		/// In case of a non-blocking socket, may return -1, even
		/// if a partial frame has been received. 
		/// In this case, receiveFrame() should be called again as 
		/// soon as more data becomes available (see select() or poll()).
		/// Eventually, receiveFrame() will return the complete frame.
		/// The given buffer will not be modified until the full frame has 
		/// been received.

	int receiveFrame(Poco::Buffer<char>& buffer, int& flags);
		/// Receives a frame from the socket and stores it
		/// after any previous content in buffer.
		/// The buffer will be grown as necessary.
		///
		/// See the note below if you want the received data
		/// to be stored at the beginning of the buffer.
		///
		/// The frame's payload size must not exceed the
		/// maximum payload size set with setMaxPayloadSize().
		/// If it does, a WebSocketException (WS_ERR_PAYLOAD_TOO_BIG)
		/// is thrown and the WebSocket connection must be
		/// terminated.
		///
		/// A WebSocketException will also be thrown if a malformed
		/// or incomplete frame is received.
		///
		/// If this method is used, a reasonable maximum payload size should
		/// be set with setMaxPayloadSize() to prevent a potential
		/// DoS attack (memory exhaustion) by sending a WebSocket frame
		/// header with a huge payload size.
		///
		/// Returns the number of payload bytes received.
		/// A return value of 0, with flags also 0, means that the peer has
		/// shut down or closed the connection.
		/// A return value of 0, with non-zero flags, indicates an
		/// reception of an empty frame (e.g., in case of a PING).
		///
		/// Throws a TimeoutException if a receive timeout has
		/// been set and nothing is received within that interval.
		/// Throws a NetException (or a subclass) in case of other errors.
		///
		/// The frame flags and opcode (FrameFlags and FrameOpcodes)
		/// is stored in flags.
		///
		/// Note: Since the data received from the WebSocket is appended
		/// to the given Poco::Buffer, the buffer passed to this method
		/// should be created with a size of 0, or resize(0) should be
		/// called on the buffer beforehand, if the expectation is that
		/// the received data is stored starting at the beginning of the
		/// buffer.
		///
		/// In case of a non-blocking socket, may return -1, even
		/// if a partial frame has been received. 
		/// In this case, receiveFrame() should be called again as 
		/// soon as more data becomes available (see select() or poll()).
		/// Eventually, receiveFrame() will return the complete frame.
		/// The given buffer will not be modified until the full frame has 
		/// been received.

	Mode mode() const;
		/// Returns WS_SERVER if the WebSocket is a server-side
		/// WebSocket, or WS_CLIENT otherwise.

	void setMaxPayloadSize(int maxPayloadSize);
		/// Sets the maximum payload size for receiveFrame().
		///
		/// The default is std::numeric_limits<int>::max().

	int getMaxPayloadSize() const;
		/// Returns the maximum payload size for receiveFrame().
		///
		/// The default is std::numeric_limits<int>::max().

	static const std::string WEBSOCKET_VERSION;
		/// The WebSocket protocol version supported (13).

protected:
	static WebSocketImpl* accept(HTTPServerRequest& request, HTTPServerResponse& response);
	static WebSocketImpl* connect(HTTPClientSession& cs, HTTPRequest& request, HTTPResponse& response, HTTPCredentials& credentials);
	static WebSocketImpl* completeHandshake(HTTPClientSession& cs, HTTPResponse& response, const std::string& key);
	static std::string computeAccept(const std::string& key);
	static std::string createKey();

private:
	WebSocket();

	static const std::string WEBSOCKET_GUID;
	static HTTPCredentials _defaultCreds;
};


} } // namespace Poco::Net


#endif // Net_WebSocket_INCLUDED
