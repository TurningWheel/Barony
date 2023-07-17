#include "main.hpp"
#include "files.hpp"
#include "json.hpp"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

#include <cassert>

const Uint32 BinaryFormatTag = *"spff";

class JsonFileWriter : public FileInterface {
public:

	JsonFileWriter()
	: buffer()
	, writer(buffer)
	{
	}

	static bool writeObject(File* file, const FileHelper::SerializationFunc& serialize) {
		JsonFileWriter jfw;

        bool result = false;
		if (jfw.beginObject()) {
		    result = serialize(&jfw);
		    jfw.endObject();
		}
		jfw.save(file);
		return result;
	}

	virtual bool isReading() const override { return false; }

	virtual bool beginObject() override {
		return writer.StartObject();
	}
	virtual void endObject() override {
		writer.EndObject();
	}

	virtual bool beginArray(Uint32 & size) override {
		return writer.StartArray();
	}
	virtual void endArray() override {
		writer.EndArray();
	}

	virtual void propertyName(const char * fieldName) override {
		writer.Key(fieldName);
	}

	virtual bool value(Uint32& value) override {
		return writer.Uint(value);
	}
	virtual bool value(Sint32& value) override {
		return writer.Int(value);
	}
	virtual bool value(float& value) override {
		return writer.Double(value);
	}
	virtual bool value(double& value) override {
		return writer.Double(value);
	}
	virtual bool value(bool& value) override {
		return writer.Bool(value);
	}
	virtual bool value(std::string& value) override {
		return writer.String(value.c_str());
	}

private:

	void save(File* file) {
		buffer.Flush();
		file->puts(buffer.GetString());
#ifdef NINTENDO
		file->putc('\0');
#endif
	}

	File * fp;
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer;
};

class JsonFileReader : public FileInterface {
public:

	static bool readObject(File * fp, const FileHelper::SerializationFunc & serialize) {
		JsonFileReader jfr;

		if (!jfr.readAllFileData(fp)) {
			return false;
		}

		if (jfr.beginObject()) {
		    bool result = serialize(&jfr);
		    jfr.endObject();
		    return result;
		} else {
		    return false;
		}
	}

	virtual bool isReading() const override { return true; }

	virtual bool beginObject() override {
		auto cv = GetCurrentValue();
		if (cv && cv->IsObject()) {
		    DocIterator di;
		    di.it = cv;
		    di.index = -1;
		    stack.push_back(di);
		    return true;
		} else {
		    return false;
		}
	}

	virtual void endObject() override {
	    if (!stack.empty()) {
		    stack.pop_back();
		}
	}

	virtual bool beginArray(Uint32 & size) override {
		auto cv = GetCurrentValue();
		if (cv && cv->IsArray()) {
		    DocIterator di;
		    di.it = cv;
		    di.index = 0;
		    stack.push_back(di);
		    size = di.it->GetArray().Size();
		    return true;
		} else {
		    return false;
		}
	}

	virtual void endArray() override {
	    if (!stack.empty()) {
		    stack.pop_back();
		}
	}
	virtual void propertyName(const char * fieldName) override {
		propName = fieldName;
	}
	virtual bool value(Uint32& value) override {
		auto cv = GetCurrentValue();
		if (cv && cv->IsUint()) {
		    value = cv->GetUint();
		    return true;
		} else {
		    return false;
		}
	}
	virtual bool value(Sint32& value) override {
		auto cv = GetCurrentValue();
		if (cv && cv->IsInt()) {
		    value = cv->GetInt();
		    return true;
		} else {
		    return false;
		}
	}
	virtual bool value(float& value) override {
		auto cv = GetCurrentValue();
		if (cv && cv->IsFloat()) {
		    value = cv->GetFloat();
		    return true;
		} else {
		    return false;
		}
	}
	virtual bool value(double& value) override {
		auto cv = GetCurrentValue();
		if (cv && cv->IsDouble()) {
		    value = cv->GetDouble();
		    return true;
		} else {
		    return false;
		}
	}
	virtual bool value(bool& value) override {
		auto cv = GetCurrentValue();
		if (cv && cv->IsBool()) {
		    value = cv->GetBool();
		    return true;
		} else {
		    return false;
		}
	}
	virtual bool value(std::string& value) override {
		auto cv = GetCurrentValue();
		if (cv && cv->IsString()) {
			value = cv->GetString();
			return true;
		} else {
		    return false;
		}
	}

protected:

	rapidjson::Value::ConstValueIterator GetCurrentValue() {
		if (stack.empty()) {
		    if (propName == nullptr) {
			    return &doc;
		    } else {
		        return nullptr;
		    }
		}

		DocIterator& di = stack.back();
		if (di.it->IsArray()) {
			if (di.index >= 0) {
			    return &di.it->GetArray()[di.index++];
			} else {
			    return nullptr;
			}
		}

		if (propName != nullptr) {
		    rapidjson::Value::ConstValueIterator result;
		    if ((*di.it).HasMember(propName)) {
		        result = &(*di.it)[propName];
		    } else {
		        result = nullptr;
		    }
		    propName = nullptr;
		    return result;
		} else {
		    return nullptr;
		}
	}

	bool readAllFileData(File * fp) {
		long size = fp->size();

		// reserve an extra byte for the null terminator
		char * data = (char *)calloc(sizeof(char), size + 1);
		assert(data);

		size_t bytesRead = fp->read(data, sizeof(char), size);
		if (bytesRead != size) {
			printlog("JsonFileReader: failed to read data (%d)", errno);
			free(data);
			return false;
		}

		// null terminate
		data[size] = 0;

		rapidjson::ParseResult result = doc.Parse(data);

		free(data);

		if (!result) {
			printlog("JsonFileReader: parse error: %s (%d)", rapidjson::GetParseError_En(result.Code()), result.Offset());
			return false;
		}

		return true;
	}

	struct DocIterator {
		rapidjson::Value::ConstValueIterator it;
		Uint32 index;
	};

	rapidjson::Document doc;
	const char * propName = nullptr;
	std::vector<DocIterator> stack;
};

class BinaryFileWriter : public FileInterface {
public:

	BinaryFileWriter(File * file)
	: fp(file)
	{
	}

	~BinaryFileWriter() {
	}

	static bool writeObject(File * fp, const FileHelper::SerializationFunc & serialize) {
		BinaryFileWriter bfw(fp);

		bfw.writeHeader();

		if (bfw.beginObject()) {
		    bool result = serialize(&bfw);
		    bfw.endObject();
		    return result;
		} else {
		    return false;
		}
	}

	virtual bool isReading() const override { return false; }

	virtual bool beginObject() override {
	    return true;
	}

	virtual void endObject() override {
	}

	virtual bool beginArray(Uint32 & size) override {
		return fp->write(&size, sizeof(size), 1) == 1;
	}

	virtual void endArray() override {
	}

	virtual void propertyName(const char * name) override {
	}

	virtual bool value(Uint32& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(Sint32& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(float& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(double& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(bool& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(std::string& v) override {
		return writeStringInternal(v);
	}

private:

	void writeHeader() {
		(void)fp->write(&BinaryFormatTag, sizeof(BinaryFormatTag), 1);
	}

	bool writeStringInternal(const std::string& v) {
		Uint32 len = (Uint32)v.size();
		bool result = true;
		result = fp->write(&len, sizeof(len), 1) == 1 ? result : false;
		if (len) {
			result = fp->write(v.c_str(), sizeof(char), len) == len ?
			    result : false;
		}
		return result;
	}

	File* fp = nullptr;
};

class BinaryFileReader : public FileInterface {
public:

	BinaryFileReader(File * file)
		: fp(file)
	{
	}

	static bool readObject(File * fp, const FileHelper::SerializationFunc & serialize) {
		BinaryFileReader bfr(fp);

		if (!bfr.readHeader()) {
			return false;
		}

		bfr.beginObject();
		bool result = serialize(&bfr);
		bfr.endObject();

		return result;
	}

	virtual bool isReading() const override { return true; }

	virtual bool beginObject() override {
	    return true;
	}

	virtual void endObject() override {
	}

	virtual bool beginArray(Uint32 & size) override {
		return fp->read(&size, sizeof(size), 1) == 1;
	}

	virtual void endArray() override {
	}

	virtual void propertyName(const char * name) override {
	}

	virtual bool value(Uint32& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(Sint32& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(float& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(double& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(bool& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(std::string& v) override {
		bool result = readStringInternal(v);
		return result;
	}

private:

	bool readHeader() {
		Uint32 fileFormatTag;
		size_t read = fp->read(&fileFormatTag, sizeof(fileFormatTag), 1);
		if (read != 1) {
			printlog("BinaryFileReader: failed to read format tag (%d)", errno);
			return false;
		}

		if (fileFormatTag != BinaryFormatTag) {
			printlog("BinaryFileReader: file format tag mismatch (expected %x, got %x)", BinaryFormatTag, fileFormatTag);
			return false;
		}

		return true;
	}

	bool readStringInternal(std::string & v) {
		Uint32 len;
		bool result = true;
		size_t read = fp->read(&len, sizeof(len), 1);
		result = read == 1 ? result : false;

		if (len) {
			v.reserve(len);
			read = fp->read(&v[0u], sizeof(char), len);
		    result = read == len ? result : false;
		}

		return result;
	}

	File* fp;
};

static EFileFormat GetFileFormat(File * file) {
	Uint32 fileFormatTag = 0;
	file->read(&fileFormatTag, sizeof(fileFormatTag), 1);
	file->seek(0, FileBase::SeekMode::SET);

	if (fileFormatTag == BinaryFormatTag) {
		return EFileFormat::Binary;
	}
	else {
		return EFileFormat::Json;
	}
}

//TODO: NX PORT: Update for the Switch?
bool FileHelper::writeObjectInternal(const char * filename, EFileFormat format, const SerializationFunc& serialize) {
	File * file = FileIO::open(filename, "wb");
#ifndef NDEBUG
	printlog("Opening file '%s' for write", filename);
#endif
	if (!file) {
		printlog("Unable to open file '%s' for write (%d)", filename, errno);
		return false;
	}

	bool success = false;
	if (format == EFileFormat::Binary) {
		success = BinaryFileWriter::writeObject(file, serialize);
	}
	else if (format == EFileFormat::Json) {
		success = JsonFileWriter::writeObject(file, serialize);
	}
	else {
		assert(false);
	}

	FileIO::close(file);

	return success;
}

bool FileHelper::readObjectInternal(const char * filename, const SerializationFunc& serialize) {
	File * file = FileIO::open(filename, "rb");
#ifndef NDEBUG
	printlog("Opening file '%s' for read", filename);
#endif
	if (!file) {
		printlog("Unable to open file '%s' for read (%d)", filename, errno);
		return false;
	}

	EFileFormat format = GetFileFormat(file);

	bool success = false;
	if (format == EFileFormat::Binary) {
		success = BinaryFileReader::readObject(file, serialize);
	}
	else if(format == EFileFormat::Json) {
		success = JsonFileReader::readObject(file, serialize);
	}
	else {
		assert(false);
	}

	FileIO::close(file);

	return success;
}
