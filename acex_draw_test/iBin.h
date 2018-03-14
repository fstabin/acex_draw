#pragma once


#include <Windows.h>


//----
class Bin
{
public:
	Bin()
		: pBin_(nullptr), binSize_(0)
	{}
	Bin(void* p, size_t s)
	{
		Copy(p, s);
	}
	Bin(LPCWSTR filename)
	{
		Read(filename);
	}
	Bin(const Bin& bin)
	{
		Copy(bin.pBin_, bin.binSize_);
	}

	~Bin()
	{
		Destroy();
	}

	bool Read(LPCWSTR filename)
	{
		Destroy();

		HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		DWORD s = GetFileSize(hFile, nullptr);
		if (s == 0)
		{
			CloseHandle(hFile);
			return false;
		}

		pBin_ = malloc(s);
		DWORD readedSize;
		if (!ReadFile(hFile, pBin_, s, &readedSize, nullptr))
		{
			CloseHandle(hFile);
			return false;
		}

		binSize_ = s;

		CloseHandle(hFile);
		return true;
	}

	bool Copy(void* p, size_t s)
	{
		if ((p == nullptr) || (s == 0))
		{
			return false;
		}

		Destroy();

		pBin_ = malloc(s);
		memcpy(pBin_, p, s);
		binSize_ = s;

		return true;
	}

	void Destroy()
	{
		if (pBin_)
		{
			free(pBin_);
			pBin_ = nullptr;
			binSize_ = 0;
		}
	}

	void* GetBin() { return pBin_; }
	const void* GetBin() const { return pBin_; }
	size_t GetBinSize() const { return binSize_; }


private:
	void*	pBin_;
	size_t	binSize_;
};	// class BinFile
