#include "main.hpp"
#include "files.hpp"
#include "json.hpp"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

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

		jfw.beginObject();
		serialize(&jfw);
		jfw.endObject();

		jfw.save(file);
		return true;
	}

	virtual bool isReading() const override { return false; }

	virtual void beginObject() override {
		writer.StartObject();
	}
	virtual void endObject() override {
		writer.EndObject();
	}

	virtual void beginArray(Uint32 & size) override {
		writer.StartArray();
	}
	virtual void endArray() override {
		writer.EndArray();
	}

	virtual void propertyName(const char * fieldName) override {
		writer.Key(fieldName);
	}

	virtual void value(Uint32& value) override {
		writer.Uint(value);
	}
	virtual void value(Sint32& value) override {
		writer.Int(value);
	}
	virtual void value(float& value) override {
		writer.Double(value);
	}
	virtual void value(double& value) override {
		writer.Double(value);
	}
	virtual void value(bool& value) override {
		writer.Bool(value);
	}
	virtual void value(std::string& value, Uint32 maxLength) override {
		assert(maxLength == 0 || value.size() <= maxLength);
		writer.String(value.c_str());
	}

private:

	void save(File* file) {
		buffer.Flush();
		file->puts(buffer.GetString());
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

		jfr.beginObject();
		serialize(&jfr);
		jfr.endObject();

		return true;
	}

	virtual bool isReading() const override { return true; }

	virtual void beginObject() override {
		auto cv = GetCurrentValue();
		assert(cv->IsObject());
		DocIterator di;
		di.it = cv;
		di.index = -1;
		stack.push_back(di);
	}

	virtual void endObject() override {
		stack.pop_back();
	}

	virtual void beginArray(Uint32 & size) override {
		auto cv = GetCurrentValue();
		assert(cv->IsArray());
		DocIterator di;
		di.it = cv;
		di.index = 0;
		stack.push_back(di);
		size = di.it->GetArray().Size();
	}

	virtual void endArray() override {
		stack.pop_back();
	}
	virtual void propertyName(const char * fieldName) override {
		propName = fieldName;
	}
	virtual void value(Uint32& value) override {
		auto cv = GetCurrentValue();
		assert(cv->IsUint());
		value = cv->GetUint();
	}
	virtual void value(Sint32& value) override {
		auto cv = GetCurrentValue();
		assert(cv->IsInt());
		value = cv->GetInt();
	}
	virtual void value(float& value) override {
		auto cv = GetCurrentValue();
		assert(cv->IsFloat());
		value = cv->GetFloat();
	}
	virtual void value(double& value) override {
		auto cv = GetCurrentValue();
		assert(cv->IsDouble());
		value = cv->GetDouble();
	}
	virtual void value(bool& value) override {
		auto cv = GetCurrentValue();
		assert(cv->IsBool());
		value = cv->GetBool();
	}
	virtual void value(std::string& value, Uint32 maxLength) override {
		auto cv = GetCurrentValue();
		assert(cv->IsString());
		assert(maxLength == 0 || cv->GetStringLength() <= maxLength);
		value = cv->GetString();
	}

protected:

	rapidjson::Value::ConstValueIterator GetCurrentValue() {
		if (stack.empty()) {
			assert(propName == nullptr);
			return &doc;
		}

		DocIterator& di = stack.back();
		if (di.it->IsArray()) {
			assert(di.index >= 0);
			return &di.it->GetArray()[di.index++];
		}

		assert(propName != nullptr);
		rapidjson::Value::ConstValueIterator result = &(*di.it)[propName];
		propName = nullptr;
		return result;
	}

	bool readAllFileData(File * fp) {
		if (fp->seek(0, FileBase::SeekMode::SETEND)) {
			printlog("JsonFileReader: failed to seek end (%d)", errno);
			return false;
		}

		long size = fp->tell();
		if (fp->seek(0, FileBase::SeekMode::SET)) {
			printlog("JsonFileReader: failed to seek beg (%d)", errno);
			return false;
		}

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

		bfw.beginObject();
		serialize(&bfw);
		bfw.endObject();

		return true;
	}

	virtual bool isReading() const override { return false; }

	virtual void beginObject() override {
	}

	virtual void endObject() override {
	}

	virtual void beginArray(Uint32 & size) override {
		fp->write(&size, sizeof(size), 1);
	}

	virtual void endArray() override {
	}

	virtual void propertyName(const char * name) override {
	}

	virtual void value(Uint32& v) override {
		fp->write(&v, sizeof(v), 1);
	}
	virtual void value(Sint32& v) override {
		fp->write(&v, sizeof(v), 1);
	}
	virtual void value(float& v) override {
		fp->write(&v, sizeof(v), 1);
	}
	virtual void value(double& v) override {
		fp->write(&v, sizeof(v), 1);
	}
	virtual void value(bool& v) override {
		fp->write(&v, sizeof(v), 1);
	}
	virtual void value(std::string& v, Uint32 maxLength) override {
		assert(maxLength == 0 || v.size() <= maxLength);
		writeStringInternal(v);
	}

private:

	void writeHeader() {
		fp->write(&BinaryFormatTag, sizeof(BinaryFormatTag), 1);
	}

	void writeStringInternal(const std::string& v) {
		Uint32 len = (Uint32)v.size();
		fp->write(&len, sizeof(len), 1);
		if (len) {
			fp->write(v.c_str(), sizeof(char), len);
		}
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
		serialize(&bfr);
		bfr.endObject();

		return true;
	}

	virtual bool isReading() const override { return true; }

	virtual void beginObject() override {
	}

	virtual void endObject() override {
	}

	virtual void beginArray(Uint32 & size) override {
		size_t read = fp->read(&size, sizeof(size), 1);
		assert(read == 1);
	}

	virtual void endArray() override {
	}

	virtual void propertyName(const char * name) override {
	}

	virtual void value(Uint32& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		assert(read == 1);
	}
	virtual void value(Sint32& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		assert(read == 1);
	}
	virtual void value(float& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		assert(read == 1);
	}
	virtual void value(double& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		assert(read == 1);
	}
	virtual void value(bool& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		assert(read == 1);
	}
	virtual void value(std::string& v, Uint32 maxLength) override {
		readStringInternal(v);
		assert(maxLength == 0 || v.size() <= maxLength);
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

	void readStringInternal(std::string & v) {
		Uint32 len;
		size_t read = fp->read(&len, sizeof(len), 1);
		assert(read == 1);

		if (len) {
			v.reserve(len);
			read = fp->read(&v[0u], sizeof(char), len);
			assert(read == len);
		}
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
