# npstream
A new stream class based on the namedpipe provided by WinAPI.Class provided encapsulation and realized interfaces to call winAPI.

demo:

int main()
{
	
	DWORD OMode = PIPE_ACCESS_DUPLEX;	
	DWORD PMode = PIPE_TYPE_BYTE;
	LPCSTR name = "\\\\.\\pipe\\test";
	bool succeed;
	npstream<int> test(name, PMode, OMode, succeed);
	if (succeed == false)
		system("pause");
	int a = 0, b = -1, c = -1;
	cout << "Input data:";
	cin >> a;
	cin >> b;
	cin >> c;
	//can't use like this:
	//test << a << b << c << flush;
	test << a;
	test << b;
	test << c << flush;
	cout << "Data sent" << endl << "a:" << a << " b:" << b << " c:" << c << endl;
	system("pause");
	

	bool connected = false;
	LPCSTR name = "\\\\.\\pipe\\test";
	npstream<int> t(name, connected);
	system("pause");
	int a = -1, b = -1, c = -1;
	t >> a;
	t >> b;
	t >> c;
	cout << "Received data" << endl << "a:" << a << endl << "b:" << b << endl << "c:" << c << endl;
	cout << endl;
	system("pause");
	
}
