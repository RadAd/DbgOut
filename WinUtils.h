#pragma once

class WinHandle
{
public:
	WinHandle(HANDLE value = NULL) : value_(value) {}

	operator HANDLE() const { return value_; }

	friend bool operator ==(WinHandle l, WinHandle r) { return l.value_ == r.value_; }
	friend bool operator !=(WinHandle l, WinHandle r) { return l.value_ != r.value_; }
	friend bool operator ==(HANDLE l, WinHandle r) { return l == r.value_; }
	friend bool operator !=(HANDLE l, WinHandle r) { return l != r.value_; }
	friend bool operator ==(WinHandle l, HANDLE r) { return l.value_ == r; }
	friend bool operator !=(WinHandle l, HANDLE r) { return l.value_ != r; }

	struct Deleter
	{
		typedef WinHandle pointer;
		void operator()(WinHandle handle) const { CloseHandle(handle); }
	};

private:
	HANDLE value_;
};

typedef std::unique_ptr<WinHandle, WinHandle::Deleter> HandlePtr;
