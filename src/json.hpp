/**
	A simple interface for reading and writing objects to files, has support for both json and binary.
	Basic types, enums and vectors are supported by default, other class and struct types need to
	implement the "bool serialize(FileInterface * file)" function.
	The interface is symmetric, meaning that there is only a single function for both saving and loading.

	class ExampleClass {
	private:
		Uint32 MyNumber;
		String MyString;
	public:
		bool serialize(FileInterface * file) {
			file->property("MyNumber", MyNumber);
			file->property("MyString", MyString);
			return true; // you may return false if any of the property() functions failed
		}
	};
*/

#pragma once

#include <functional>
#include <string>

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
	virtual bool beginObject() = 0;
	// Signals the end of an object in the file
	virtual void endObject() = 0;

	// Signals the beginning of an array in the file
	// @param size number of items in the array
	virtual bool beginArray(Uint32 & size) = 0;
	// Signals the end of an array in the file
	virtual void endArray() = 0;

	// Serializes the name of a property
	// @param name name of the property 
	virtual void propertyName(const char * name) = 0;

	// @param v the value to serialize
	virtual bool value(Uint32& v) = 0;
	// @param v the value to serialize
	virtual bool value(Sint32& v) = 0;
	// @param v the value to serialize
	virtual bool value(float& v) = 0;
	// @param v the value to serialize
	virtual bool value(double& v) = 0;
	// @param v the value to serialize
	virtual bool value(bool& v) = 0;
	// @param v the value to serialize
	virtual bool value(std::string& v) = 0;

	// Serialize a vector with a max length
	// @param v the value to serialize
	// @param maxLength maximum number of items, 0 is no limit
	template<typename T, typename... Args>
	bool value(std::vector<T>& v, Uint32 maxLength = 0, Args ... args) {
		Uint32 size = (Uint32)v.size();
		if (beginArray(size) && (maxLength == 0 || size <= maxLength)) {
		    v.resize(size);
		    bool result = true;
		    for (Uint32 index = 0; index < size; ++index) {
			    result = value(v[index], args...) ? result : false;
		    }
		    endArray();
		    return result;
		} else {
		    return false;
		}
	}

	// Serialize a pair
	// @param v the pair to serialize
	template<typename T1, typename T2>
	bool value(std::pair<T1, T2>& v) {
	    bool result = false;
	    if (beginObject()) {
	        result = true;
	        result = property("first", v.first) ? result : false;
	        result = property("second", v.second) ? result : false;
	        endObject();
	    }
	    return result;
	}

	// Serialize a pointer by dereferencing it
	// @param v the pointer to dereference and serialize
	template<typename T>
	bool value (T*& v) {
		if (isReading()) {
			v = new T();
		}
		return value(*v);
	}

	// Serializes an enum value as its underlying type to the file
	// @param t the enum value to serialize
	template<typename T>
	typename std::enable_if<std::is_enum<T>::value, bool>::type
	value(T& v) {
		typename std::underlying_type<T>::type temp = v;
		return value(temp);
		v = (T)temp;
	}

	// Serializes a class or struct to the file using it's ::serialize(FileInterface*) function
	// @param v the object to serialize
	template<typename T>
	typename std::enable_if<std::is_class<T>::value, bool>::type
	value(T& v) {
	    bool result = false;
		if (beginObject()) {
		    result = v.serialize(this);
		    endObject();
		}
		return result;
	}
	
	// Serializes a fixed-size native array
	// @param v array to serialize
	template<typename T, Uint32 Size, typename... Args>
	bool value(T (&v)[Size], Args ... args) {
		Uint32 size = Size;
		if (beginArray(size) && size == Size) {
		    bool result = true;
		    for (Uint32 index = 0; index < size; ++index) {
			    result = value(v[index], args...) ? result : false;
		    }
		    endArray();
		    return result;
		} else {
		    return false;
		}
	}

	// Helper function to serialize a property name and value at the same time 
	// @param name name of the property
	// @param v value to serialize
	// @param args additional args to pass into the value() function
	template<typename T, typename... Args>
	bool property(const char * name, T& v, Args ... args) {
		propertyName(name);
		return value(v, args...);
	}

    // As above, but if check is false, the property won't be read.
    // this allows version checking with an expression, eg:
    // propertyVersion("myInt", version >= 2, i);
    template<typename T, typename... Args>
    bool propertyVersion(const char* name, bool check, T& v, Args ... args) {
        if (!isReading() || check) {
            propertyName(name);
            return value(v, args...);
        } else {
            return true;
        }
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

	typedef std::function<bool(FileInterface*)> SerializationFunc;

private:

	static bool writeObjectInternal(const char * filename, EFileFormat format, const SerializationFunc& serialize);
	static bool readObjectInternal(const char * filename, const SerializationFunc& serialize);
};
