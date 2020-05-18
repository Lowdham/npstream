#ifndef NPSTREAM_H_
#define NPSTREAM_H_

#include<Windows.h>
#include<iostream>
#include<fstream>
#include<streambuf>

using namespace std;
typedef unsigned int uint;


class pipebuf:public filebuf
{
public:
	pipebuf(HANDLE &_hFile, uint _bufferSize = 16, int _maxBackNum = 4)
	{
		bufferSize = _bufferSize;
		buffer = new char[_bufferSize];
		hFile = _hFile;
		memset(buffer, 0, bufferSize - 1);
		setp(buffer, buffer + bufferSize - 1);
		pubsetbuf(buffer, bufferSize);
		maxBackNum = _maxBackNum;
	}
	virtual ~pipebuf()
	{
		sync();
		delete buffer;
	}
protected:
	char* buffer;
	HANDLE hFile;
	//optional
	int maxBackNum;
	uint bufferSize;

	int flushBuffer()
	{
		int num = pptr() - pbase();
		DWORD wnNum = 0;
		DWORD* pwnNum = &wnNum;
		if (WriteFile(hFile, buffer, num, pwnNum, NULL) == 0)
		{
#ifdef DEBUG_NPSTREAM
			cout << "Writing failed" << endl;
			cout << "Error code:" << GetLastError() << endl;
#endif // DEBUG
			return EOF;
		}
		else {
			if (wnNum != num)//output¡£
			{
				return EOF;
			}
			pbump(-num);//Clears the current write pointer,then point to the start of the buffer
#ifdef DEBUG_NPSTREAM
			cout << endl << "Writing succeed" << endl;
#endif // DEBUG
			return num;
		}
	}

	virtual int_type overflow(int_type c) override
	{
		if (c != EOF) {
			*pptr() = c;
			pbump(1);
		}
		if (flushBuffer() == EOF)
		{
			return EOF;
		}
		return c;
	}

	virtual int_type underflow()
	{
		if (gptr() < egptr())
		{
			return *gptr();
		}
		int numputback;
		numputback = gptr() - eback();
		if (numputback > maxBackNum)
			numputback = maxBackNum;
		memcpy(buffer + maxBackNum - numputback, gptr() - numputback, numputback);
		//int num;
		DWORD wnNum = 0, pkNum = 0;
		DWORD* pwnNum = &wnNum, * ppkNum = &pkNum;
		DWORD peekBufSize = bufferSize - maxBackNum;
		DWORD peekSize = bufferSize - maxBackNum;
		char* peekBuf = new char[peekSize];
		PeekNamedPipe(hFile, peekBuf, peekSize, NULL, ppkNum, NULL);
		if (pkNum != 0)
		{
			if (ReadFile(hFile, buffer + maxBackNum, bufferSize - maxBackNum, pwnNum, NULL) == 0)
			{
#ifdef DEBUG_NPSTREAM
				cout << "Reading failed" << endl;
				cout << "Error code:" << GetLastError() << endl;
#endif // DEBUG
				return EOF;
			}

		}
		delete[]peekBuf;
#ifdef DEBUG_NPSTREAM
		cout << "Reading succeed" << endl;
#endif // DEBUG
		setg(buffer + (maxBackNum - numputback), buffer + maxBackNum, buffer + maxBackNum + wnNum);
		return *gptr();
	}

	virtual int sync() override
	{
		if (flushBuffer() == EOF)
			return -1;
		return 0;
	}
};

class pstream
{
public:
	pstream(HANDLE _toPipe, uint _bufferSize = 16):pipeBuffer(_toPipe, _bufferSize)
	{ 
		toPipe = _toPipe;
		streambuf* pb = &pipeBuffer;
		pipe.set_rdbuf(pb);
	}

	template<typename T>
	ostream& operator<<(const T& X)
	{
		pipe << X << " ";
#ifdef DEBUG_NPSTREAM
		cout << "Override called" << endl;
#endif // DEBUG
		return pipe;
	}

	template<typename T>
	istream& operator>>(T& X)
	{
		pipe >> X;
		return pipe;
	}

	fstream& getPipe() { return pipe; }
protected:
	HANDLE toPipe;
	fstream pipe;
	pipebuf pipeBuffer;

};

template<typename T>
class npstream
{
	enum class type { server, client };
public:
	npstream(LPCSTR _PipeName, DWORD _PipeMode, DWORD _OpenMode, bool& ifSucceed,DWORD _MaxInstances = 10, DWORD _TimeOut = 1000);
	npstream(LPCSTR _PipeName,bool& ifSucceed);
	~npstream();

	ostream& operator<<(const T& X);
	istream& operator>>(T& X);
	DWORD pipeMode() { return PipeMode; }
	DWORD openMode() { return OpenMode; }
	DWORD outBufSize() { return OutPutBufSize; }
	DWORD timeOut() { return TimeOut; }
private:
	char PipeName[30];
	DWORD PipeMode;
	DWORD OpenMode;
	DWORD OutPutBufSize;
	DWORD InPutBufSize;
	DWORD TimeOut;
	DWORD MaxInstances;
	HANDLE hPipe;
	pstream* NamedPipe;
	type npstreamType;
};

//Implement
template<typename T>
npstream<T>::npstream(LPCSTR _PipeName, DWORD _PipeMode, DWORD _OpenMode, bool& ifSucceed, DWORD _MaxInstances, DWORD _TimeOut)
{
#ifdef DEBUG_NPSTREAM
	cout << "Creating npstream object" << endl;
#endif // DEBUG
	strcpy_s(PipeName, _PipeName);
	PipeMode = _PipeMode;
	OpenMode = _OpenMode;
	OutPutBufSize = sizeof(T) * 1000;
	InPutBufSize = sizeof(T) * 1000;
	MaxInstances = _MaxInstances;
	TimeOut = _TimeOut;
	npstreamType = type::server;
	hPipe = INVALID_HANDLE_VALUE;
	ifSucceed = false;

	//
	BYTE sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
	SECURITY_ATTRIBUTES sa;

	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = &sd;

	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, (PACL)0, FALSE);

#ifdef DEBUG_NPSTREAM
	cout << "Trying to create a namedPipe" << endl;
#endif // DEBUG
	hPipe = CreateNamedPipeA(PipeName, OpenMode, PipeMode, MaxInstances, OutPutBufSize, InPutBufSize, TimeOut, &sa);
	if (hPipe != INVALID_HANDLE_VALUE && hPipe != NULL)
	{
#ifdef DEBUG_NPSTREAM
		cout << "NamedPipe created" << endl;
#endif // DEBUG
		NamedPipe = new pstream(hPipe);
#ifdef DEBUG_NPSTREAM
		cout << "Creating completed" << endl;
#endif // DEBUG
		ifSucceed = true;
		cout << "Waiting for client connection" << endl;
		ConnectNamedPipe(hPipe, NULL);
		cout << "Connecting succeed" << endl;
	}
	else
	{
#ifdef DEBUG_NPSTREAM
		cout << "Creating failed" << endl;
#endif // DEBUG
	}
}
template<typename T>
npstream<T>::npstream(LPCSTR _PipeName, bool& ifSucceed)
{
	cout << "Trying to connect a namedPipe" << endl;
	hPipe = CreateFileA(_PipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	npstreamType = type::client;
	ifSucceed = false;
	if (hPipe != INVALID_HANDLE_VALUE && hPipe != NULL)
	{
		ifSucceed = true;
		cout << "NamedPipe connected" << endl;
		NamedPipe = new pstream(hPipe);
	}
	else
		cout << "Connecting failed" << endl;
}
template<typename T>
npstream<T>::~npstream()
{
	delete NamedPipe;

	if (npstreamType == type::server)
	{
		DisconnectNamedPipe(hPipe);
		cout << "Disconnected" << endl;
	}

	if (CloseHandle(hPipe))
	{
#ifdef DEBUG_NPSTREAM
		cout << "~npstream succeed" << endl;
#endif // DEBUG
	}
	else
	{
#ifdef DEBUG_NPSTREAM
		cout << "~npstream failed" << endl;
#endif // DEBUG
	}
}

template<typename T>
ostream& npstream<T>::operator<<(const T& X)
{
	(*NamedPipe) << X;
	return NamedPipe->getPipe();
}

template<typename T>
istream& npstream<T>::operator>>(T& X)
{
	(*NamedPipe) >> X;
	return NamedPipe->getPipe();
}
//

#endif