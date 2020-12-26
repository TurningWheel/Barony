#pragma once

/*-------------------------------------------------------------------------------

	BARONY
	File: filepc.hpp
	Desc: defines the PC-specific derived class for FileBase.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

class FileBase;

//Don't create a FileBase or derivative class (such as this one) directly, use FileIO::open to get one...
class FilePC : public FileBase
{
	friend class FileIO;
public:
	size_t write(const void* src, size_t size, size_t count) override
	{
		if (0U == FileBase::write(src, size, count))
		{
			return 0U;
		}

		return fwrite(src, size, count, fp);
	}

	size_t read(void* buffer, size_t size, size_t count) override
	{
		if (0U == FileBase::read(buffer, size, count))
		{
			return 0U;
		}

		return fread(buffer, size, count, fp);
	}

	size_t size()
	{
		size_t offset = ftell(fp);
		(void)fseek(fp, 0, SEEK_END);
		size_t input_file_size = ftell(fp);
		(void)fseek(fp, offset, SEEK_SET);
		return input_file_size;
	}

	bool eof()
	{
		return feof(fp) != 0;
	}

	char* gets(char* buf, int size) override
	{
		if (nullptr == FileBase::gets(buf, size))
		{
			return nullptr;
		}

		return fgets(buf, size, fp);
	}

	int seek(ptrdiff_t offset, SeekMode mode)
	{
		switch (mode)
		{
			case SeekMode::SET: return fseek(fp, offset, SEEK_SET);
			case SeekMode::ADD: return fseek(fp, offset, SEEK_CUR);
			case SeekMode::SETEND: return fseek(fp, offset, SEEK_END);
		}

		return -1; //Idk, it says "return non-zero on error"
	}

	long int tell()
	{
		return (long int)ftell(fp);
	}

	FILE *handle()
	{
		return fp;
	}

private:
	FilePC(FILE* fp, FileMode mode, const char* path) :
		FileBase(mode, path),
		fp(fp)
	{
	}

	~FilePC()
	{
	}

	void close()
	{
		fclose(fp);
	}

	FILE* fp = nullptr;
};
