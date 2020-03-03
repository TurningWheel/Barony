/**
	A simple interface for reading and writing objects to files, has support for both json and binary.
	Basic types, enums and vectors are supported by default, other class and struct types need to
	implement the "void serialize(FileInterface * file)" function.
	The interface is symmetric, meaning that there is only a single function for both saving and loading.

	class ExampleClass {
	private:
		Uint32 MyNumber;
		String MyString;
	public:
		void serialize(FileInterface * file) {
			file->property("MyNumber", MyNumber);
			file->property("MyString", MyString);
		}
	};
*/

#pragma once

#include <functional>
#include <assert.h>

enum class EFileFormat {
	Json,
	Binary
};

class FileInterface {
public:
	virtual ~FileInterface() {}
	
	// @return true if this interface is reading data from a file, false if it is writing
	virtual bool isReading() const = 0;

	// Signals the beginning of an object in the file
	virtual void beginObject() = 0;
	// Signals the end of an object in the file
	virtual void endObject() = 0;

	// Signals the beginning of an array in the file
	// @param size number of items in the array
	virtual void beginArray(Uint32 & size) = 0;
	// Signals the end of an array in the file
	virtual void endArray() = 0;

	// Serializes the name of a property
	// @param name name of the property 
	virtual void propertyName(const char * name) = 0;

	// @param v the value to serialize
	virtual void value(Uint32& v) = 0;
	// @param v the value to serialize
	virtual void value(Sint32& v) = 0;
	// @param v the value to serialize
	virtual void value(float& v) = 0;
	// @param v the value to serialize
	virtual void value(double& v) = 0;
	// @param v the value to serialize
	virtual void value(bool& v) = 0;
	// @param v the value to serialize
	// @param maxLength maximum length of the string allowed, 0 is no limit
	virtual void value(std::string& v, Uint32 maxLength = 0) = 0;

	// Serialize a vector with a max length
	// @param v the value to serialize
	// @param maxLength maximum number of items, 0 is no limit
	template<typename T, typename... Args>
	void value(std::vector<T>& v, Uint32 maxLength = 0, Args ... args) {
		Uint32 size = (Uint32)v.size();
		beginArray(size);
		assert(maxLength == 0 || size <= maxLength);
		v.resize(size);
		for (Uint32 index = 0; index < size; ++index) {
			value(v[index], args...);
		}
		endArray();
	}

	// Serialize a pointer by dereferencing it
	// @param v the pointer to dereference and serialize
	template<typename T>
	void value (T*& v) {
		if (isReading()) {
			v = new T();
		}
		value(*v);
	}

	// Serializes an enum value as its underlying type to the file
	// @param t the enum value to serialize
	template<typename T>
	typename std::enable_if<std::is_enum<T>::value, void>::type
	value(T& v) {
		typename std::underlying_type<T>::type temp = v;
		value(temp);
		v = (T)temp;
	}

	// Serializes a class or struct to the file using it's ::serialize(FileInterface*) function
	// @param v the object to serialize
	template<typename T>
	typename std::enable_if<std::is_class<T>::value, void>::type
	value(T& v) {
		beginObject();
		v.serialize(this);
		endObject();
	}
	
	// Serializes a fixed-size native array
	// @param v array to serialize
	template<typename T, Uint32 Size, typename... Args>
	void value(T (&v)[Size], Args ... args) {
		Uint32 size = Size;
		beginArray(size);
		assert(size == Size);
		for (Uint32 index = 0; index < size; ++index) {
			value(v[index], args...);
		}
		endArray();
	}

	// Helper function to serialize a property name and value at the same time 
	// @param name name of the property
	// @param v value to serialize
	// @param args additional args to pass into the value() function
	template<typename T, typename... Args>
	void property(const char * name, T& v, Args ... args) {
		propertyName(name);
		value(v, args...);
	}

};

class FileHelper {
public:
	// Write an object's data to a file
	// @param filename the name of the file to write
	// @param v the object to write
	template<typename T>
	static bool writeObject(const char * filename, EFileFormat format, T & v) {
		using std::placeholders::_1;
		SerializationFunc serialize = std::bind(&T::serialize, &v, _1);
		return writeObjectInternal(filename, format, serialize);
	}

	// Read an object's data from a file
	// @param filename the name of the file to read
	// @param v the object to populate with data
	template<typename T>
	static bool readObject(const char * filename, T & v) {
		using std::placeholders::_1;
		SerializationFunc serialize = std::bind(&T::serialize, &v, _1);
		return readObjectInternal(filename, serialize);
	}

	typedef std::function<void(FileInterface*)> SerializationFunc;

private:

	static bool writeObjectInternal(const char * filename, EFileFormat format, const SerializationFunc& serialize);
	static bool readObjectInternal(const char * filename, const SerializationFunc& serialize);
};
