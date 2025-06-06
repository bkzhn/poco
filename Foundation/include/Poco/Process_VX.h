//
// Process_VX.h
//
// Library: Foundation
// Package: Processes
// Module:  Process
//
// Definition of the ProcessImpl class for VxWorks.
//
// Copyright (c) 2004-20011, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#ifndef Foundation_Process_VX_INCLUDED
#define Foundation_Process_VX_INCLUDED


#include "Poco/Foundation.h"
#include "Poco/RefCountedObject.h"
#include <vector>
#include <map>


#undef PID


namespace Poco {


class Pipe;


class Foundation_API ProcessHandleImpl: public RefCountedObject
{
public:
	ProcessHandleImpl(int pid);
	~ProcessHandleImpl() override;

	int id() const;
	int wait() const;
	int tryWait() const;

private:
	int _pid;
};


class Foundation_API ProcessImpl
{
public:
	using PIDImpl = int;
	using ArgsImpl = std::vector<std::string>;
	using EnvImpl = std::map<std::string, std::string>;

	static PIDImpl idImpl();
	static void timesImpl(long& userTime, long& kernelTime);
	static void timesMicrosecondsImpl(Poco::Int64& userTime, Poco::Int64& kernelTime);
	static ProcessHandleImpl* launchImpl(
		const std::string& command,
		const ArgsImpl& args,
		const std::string& initialDirectory,
		Pipe* inPipe,
		Pipe* outPipe,
		Pipe* errPipe,
		const EnvImpl& env);
	static void killImpl(ProcessHandleImpl& handle);
	static void killImpl(PIDImpl pid);
	static bool isRunningImpl(const ProcessHandleImpl& handle);
	static bool isRunningImpl(PIDImpl pid);
	static void requestTerminationImpl(PIDImpl pid);
};


} // namespace Poco


#endif // Foundation_Process_UNIX_INCLUDED
