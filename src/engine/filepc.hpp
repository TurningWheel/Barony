#pragma once

/*-------------------------------------------------------------------------------

	BARONY
	File: filepc.hpp
	Desc: defines the PC-specific derived class for FileBase.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../files.hpp"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>

//Don't create a FileBase or derivative class (such as this one) directly, use FileIO::open to get one...
class FilePC : public FileBase
{
	friend class FileIO;
public:
	size_t write(const void* src, size_t size, size_t count) override
	{
		if (0U == FileBase::write(src, size, count)) {
			return 0U;
		}
        const size_t writeSize = size * count;
        (void)data.insert(data.begin() + pos, (const uint8_t*)src, (const uint8_t*)src + writeSize);
        pos += writeSize;
		return writeSize / size;
	}

	size_t read(void* buffer, size_t size, size_t count) override
	{
		if (0U == FileBase::read(buffer, size, count)) {
			return 0U;
		}
		size_t readSize = 0U;
		size_t end = std::min(this->size(), pos + size * count);
		uint8_t* buf = (uint8_t*)buffer;
		for (size_t c = pos; c < end; ++c) {
			*buf = data[c]; ++buf;
			++readSize;
		}
		pos += readSize;
		return readSize / size;
	}

	size_t size() override
	{
		return data.size();
	}

	bool eof() override
	{
		return pos >= size();
	}

	int seek(ptrdiff_t offset, SeekMode mode) override
	{
		switch (mode) {
		case SeekMode::SET: pos = offset; break;
		case SeekMode::ADD: pos += offset; break;
		case SeekMode::SETEND: pos = size() + offset; break;
		}
		if (eof()) {
			return -1;
		} else {
			return 0;
		}
	}

	long int tell() override
	{
		return (long int)pos;
	}

private:
	FilePC(FILE* fp, FileMode mode, const char* path) :
		FileBase(mode, path),
		fp(fp)
	{
	    assert(fp);
	    if (mode == FileMode::READ) {
		    (void)fseek(fp, 0, SEEK_END);
		    size_t end = ftell(fp);
		    (void)fseek(fp, 0, SEEK_SET);
		    data.resize(end);
		    size_t c = 0;
		    for (; c < end;) {
		        size_t result = fread(data.data(), sizeof(uint8_t), end - c, fp);
		        if (!result) {
		            // failed to read, try to read just a chunk
		            constexpr size_t chunk_size = 1024;
		            size_t chunk = std::min(end - c, chunk_size);
		            printlog("[FILES] failed to read %llu bytes from '%s', trying %llu bytes instead", end - c, path, chunk);
		            result = fread(data.data(), sizeof(uint8_t), chunk, fp);
		            assert(result);
		        }
		        c += result;
		    }
	        assert(c == end);
		}
	}

	~FilePC()
	{
	}

	void close() override
	{
	    assert(fp);
	    if (mode == FileMode::WRITE) {
	        size_t c = 0u;
	        size_t end = size();
		    for (; c < end;) {
		        size_t result = fwrite(data.data(), sizeof(uint8_t), end - c, fp);
		        if (!result) {
		            // failed to write, try to write just a chunk
		            constexpr size_t chunk_size = 1024;
		            size_t chunk = std::min(end - c, chunk_size);
		            printlog("[FILES] failed to write %llu bytes to '%s', trying %llu bytes instead", end - c, path.c_str(), chunk);
		            result = fwrite(data.data(), sizeof(uint8_t), chunk, fp);
		            assert(result);
		        }
		        c += result;
		    }
	        assert(c == end);
	    }
		int result = fclose(fp);
		assert(result == 0);
	}

	FILE* fp = nullptr;
	std::vector<uint8_t> data;
	size_t pos = 0u;
};
