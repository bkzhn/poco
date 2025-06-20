//
// ParallelSocketAcceptor.h
//
// Library: Net
// Package: Reactor
// Module:  ParallelSocketAcceptor
//
// Definition of the ParallelSocketAcceptor class.
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#ifndef Net_ParallelSocketAcceptor_INCLUDED
#define Net_ParallelSocketAcceptor_INCLUDED


#include "Poco/Net/ParallelSocketReactor.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Environment.h"
#include "Poco/SharedPtr.h"
#include <vector>


namespace Poco {
namespace Net {


template <class ServiceHandler, class SR>
class ParallelSocketAcceptor
	/// This class implements the Acceptor part of the Acceptor-Connector design pattern.
	/// Only the difference from single-threaded version is documented here, For full
	/// description see Poco::Net::SocketAcceptor documentation.
	///
	/// This is a multi-threaded version of SocketAcceptor, it differs from the
	/// single-threaded version in number of reactors (defaulting to number of processors)
	/// that can be specified at construction time and is rotated in a round-robin fashion
	/// by event handler. See ParallelSocketAcceptor::onAccept and
	/// ParallelSocketAcceptor::createServiceHandler documentation and implementation for
	/// details.
{
public:
	using ParallelReactor = Poco::Net::ParallelSocketReactor<SR>;
	using Observer = Poco::NObserver<ParallelSocketAcceptor, ReadableNotification>;

	explicit ParallelSocketAcceptor(ServerSocket& socket,
		unsigned threads = Poco::Environment::processorCount(),
		const std::string& threadName = ""):
		_threadName(threadName),
		_socket(socket),
		_pReactor(nullptr),
		_threads(threads),
		_next(0)
		/// Creates a ParallelSocketAcceptor using the given ServerSocket,
		/// sets number of threads and populates the reactors vector.
	{
		init();
	}

	ParallelSocketAcceptor(ServerSocket& socket,
		SocketReactor& reactor,
		unsigned threads = Poco::Environment::processorCount(), const std::string& threadName = ""):
		_threadName(threadName),
		_socket(socket),
		_pReactor(&reactor),
		_threads(threads),
		_next(0)
		/// Creates a ParallelSocketAcceptor using the given ServerSocket, sets the
		/// number of threads, populates the reactors vector and registers itself
		/// with the given SocketReactor.
	{
		init();
		_pReactor->addEventHandler(_socket, Observer(*this, &ParallelSocketAcceptor::onAccept));
	}

	virtual ~ParallelSocketAcceptor()
		/// Destroys the ParallelSocketAcceptor.
	{
		try
		{
			if (_pReactor)
			{
				_pReactor->removeEventHandler(_socket, Observer(*this, &ParallelSocketAcceptor::onAccept));
			}
		}
		catch (...)
		{
			poco_unexpected();
		}
	}

	ParallelSocketAcceptor() = delete;
	ParallelSocketAcceptor(const ParallelSocketAcceptor&) = delete;
	ParallelSocketAcceptor& operator = (const ParallelSocketAcceptor&) = delete;

	void setReactor(SocketReactor& reactor)
		/// Sets the reactor for this acceptor.
	{
		registerAcceptor(reactor);
	}

	virtual void registerAcceptor(SocketReactor& reactor)
		/// Registers the ParallelSocketAcceptor with a SocketReactor.
		///
		/// A subclass can override this function to e.g.
		/// register an event handler for timeout event.
		///
		/// The overriding method must either call the base class
		/// implementation or register the accept handler on its own.
	{
		_pReactor = &reactor;
		if (!_pReactor->hasEventHandler(_socket, Observer(*this, &ParallelSocketAcceptor::onAccept)))
		{
			_pReactor->addEventHandler(_socket, Observer(*this, &ParallelSocketAcceptor::onAccept));
		}
	}

	virtual void unregisterAcceptor()
		/// Unregisters the ParallelSocketAcceptor.
		///
		/// A subclass can override this function to e.g.
		/// unregister its event handler for a timeout event.
		///
		/// The overriding method must either call the base class
		/// implementation or unregister the accept handler on its own.
	{
		if (_pReactor)
		{
			_pReactor->removeEventHandler(_socket, Observer(*this, &ParallelSocketAcceptor::onAccept));
		}
	}

	void onAccept(const AutoPtr<ReadableNotification>& pNotification)
		/// Accepts connection and creates event handler.
	{
		StreamSocket sock = _socket.acceptConnection();
		_pReactor->wakeUp();
		createServiceHandler(sock);
	}

protected:
	using ReactorVec = std::vector<typename ParallelReactor::Ptr>;

	virtual ServiceHandler* createServiceHandler(StreamSocket& socket)
		/// Create and initialize a new ServiceHandler instance.
		/// If socket is already registered with a reactor, the new
		/// ServiceHandler instance is given that reactor; otherwise,
		/// the next reactor is used. Reactors are rotated in round-robin
		/// fashion.
		///
		/// Subclasses can override this method.
	{
		SocketReactor* pReactor = reactor(socket);
		if (!pReactor)
		{
			std::size_t next = _next++;
			if (_next == _reactors.size()) _next = 0;
			pReactor = _reactors[next];
		}
		pReactor->wakeUp();
		return new ServiceHandler(socket, *pReactor);
	}

	SocketReactor* reactor(const Socket& socket)
		/// Returns reactor where this socket is already registered
		/// for polling, if found; otherwise returns null pointer.
	{
		typename ReactorVec::iterator it = _reactors.begin();
		typename ReactorVec::iterator end = _reactors.end();
		for (; it != end; ++it)
		{
			if ((*it)->has(socket)) return it->get();
		}
		return nullptr;
	}

	SocketReactor* reactor()
		/// Returns a pointer to the SocketReactor where
		/// this SocketAcceptor is registered.
		///
		/// The pointer may be null.
	{
		return _pReactor;
	}

	Socket& socket()
		/// Returns a reference to the SocketAcceptor's socket.
	{
		return _socket;
	}

	void init()
		/// Populates the reactors vector.
	{
		poco_assert (_threads > 0);

		for (unsigned i = 0; i < _threads; ++i)
			_reactors.push_back(new ParallelReactor(_threadName + "#" + std::to_string(i)));
	}

	ReactorVec& reactors()
		/// Returns reference to vector of reactors.
	{
		return _reactors;
	}

	SocketReactor* reactor(std::size_t idx)
		/// Returns reference to the reactor at position idx.
	{
		return _reactors.at(idx).get();
	}

	std::size_t next()
		/// Returns the next reactor index.
	{
		return _next;
	}

private:

	std::string _threadName;
		/// Name prefix of sub SocketReactor threads
	ServerSocket   _socket;
	SocketReactor* _pReactor;
	unsigned       _threads;
	ReactorVec     _reactors;
	std::size_t    _next;
};


} } // namespace Poco::Net


#endif // Net_ParallelSocketAcceptor_INCLUDED
