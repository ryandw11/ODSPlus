/*
=============================[ODSPlus]=============================
ODSPlus is the C++ port of Object Data Structure (The java version).
The entirty of the code is inside of the header file to make your life easier.
For now please check out the JavaDoc for the API until the formal C++ version is written.

The code is currently formatted similar to how you would find it in the Java version.

Missing Features:
 - File Compression Reading/Writing.

Author: Ryandw11
License: MIT (see LICENSE file).

=============================[ODSPlus]=============================
*/

#pragma once

#ifndef ODS_HEADER
#define ODS_HEADER

#include <iostream>;
#include <fstream>;
#include <vector>;
#include <algorithm>;
#include <any>;

#include "zlib.h";

// The ODS Namespace
namespace ODS {

	/*
	=========================================
	ODS Utility classes, and data types.
	=========================================
	*/
	// The diffrent types of compression
	enum class CompressionType {
		NONE,
		GZIP,
		ZLIB
	};

	/**
	Important note:
	ODS bytes are signed.
	*/
	typedef char byte;


	// The standard ODS exception.
	// This is thrown when an error occurs or you attempt
	// to trigger an unimplemented method (from ITag or the Tag template).
	class ODSException : public std::exception {
	public:
		ODSException(const char* message) throw();
		virtual const char* what() const throw();
	private:
		const char* msg;
	};

	inline ODSException::ODSException(const char* message) throw()
	{
		msg = message;
	}

	inline const char* ODSException::what() const throw()
	{
		return msg;
	}


	// A utility template method to swap the endianess of a datatype.
	template <typename T>
	T swap_endian(T u)
	{
		static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

		union
		{
			T u;
			char u8[sizeof(T)];
		} source, dest;

		source.u = u;

		for (size_t k = 0; k < sizeof(T); k++)
			dest.u8[k] = source.u8[sizeof(T) - k - 1];

		return dest.u;
	}

	/**
	====================================

		Input / Output streams

	====================================
	*/
	// BinaryOutputStream is a custom stream that allows you to write primative data types
	// in the big endian format.
	// There are two types of BinaryOutputStream: The memory only version, and the file version.
	// The memory only version will not write to a file. Do enable memory only just construct the class
	// with no parameters.
	//
	// The file mode does NOT open the file until the close() method is called. This is to allow for the automatic
	// compression of the file. (TODO: It might be a good idea to stream files when there is no compression)
	class BinaryOutputStream {
	public:
		BinaryOutputStream(std::string file_name, CompressionType type);
		BinaryOutputStream(std::string file_name);
		BinaryOutputStream();
		~BinaryOutputStream();

		void writeByte(byte b);
		void writeByte(const byte* b, int size);

		void writeShort(short s);
		void writeInt(int i);
		void writeLong(__int64 l);
		void writeDouble(double d);
		void writeFloat(float f);

		// these methods exist to be redundant.
		void writeInt16(__int16 i);
		void writeInt32(__int32 i);

		void writeString(std::string string);

		void close();
		// Get the array of bytes (This works in both memory and file mode.)
		byte* getArray();
		int length();

	private:
		std::vector<byte> bytes;
		std::string name;
		CompressionType compressionType;
	};

	BinaryOutputStream::BinaryOutputStream(std::string file_name, CompressionType type)
	{
		name = file_name;
		compressionType = type;
		bytes = std::vector<byte>();
	}

	BinaryOutputStream::BinaryOutputStream(std::string file_name)
	{
		name = file_name;
		compressionType = CompressionType::NONE;
		bytes = std::vector<byte>();
	}

	BinaryOutputStream::BinaryOutputStream()
	{
		compressionType = CompressionType::NONE;
		bytes = std::vector<byte>();
	}

	BinaryOutputStream::~BinaryOutputStream() {}

	void BinaryOutputStream::writeByte(byte b)
	{
		bytes.push_back(b);
	}

	void BinaryOutputStream::writeByte(const byte* b, int size)
	{
		bytes.insert(bytes.end(), b, b + size);
	}

	// Inline since vs wants it to be.
	inline void BinaryOutputStream::writeShort(short s)
	{
		bytes.push_back(s >> 8);
		bytes.push_back(s);
	}

	inline void BinaryOutputStream::writeInt(int i)
	{
		bytes.push_back(i >> 24);
		bytes.push_back(i >> 16);
		bytes.push_back(i >> 8);
		bytes.push_back(i);
	}

	inline void BinaryOutputStream::writeLong(__int64 l)
	{
		__int64 lon = swap_endian<__int64>(l);
		byte* b = reinterpret_cast<byte*>(&lon);
		writeByte(b, 8);
	}

	inline void BinaryOutputStream::writeDouble(double d)
	{
		double bigD = swap_endian<double>(d);
		byte* b = reinterpret_cast<byte*>(&bigD);
		writeByte(b, 8);
	}

	inline void BinaryOutputStream::writeFloat(float f)
	{
		float bigF = swap_endian<float>(f);
		byte* b = reinterpret_cast<byte*>(&bigF);
		writeByte(b, 4);
	}

	inline void BinaryOutputStream::writeInt16(__int16 i)
	{
		__int16 lon = swap_endian<__int16>(i);
		byte* b = reinterpret_cast<byte*>(&lon);
		writeByte(b, 2);
	}

	inline void BinaryOutputStream::writeInt32(__int32 i)
	{
		__int32 lon = swap_endian<__int32>(i);
		byte* b = reinterpret_cast<byte*>(&lon);
		writeByte(b, 4);
	}

	inline void BinaryOutputStream::writeString(std::string string)
	{
		writeByte(string.c_str(), string.length());
	}

	// Write the data from memory into the file.
	// (There is no technical reason to do this when in memory mode; however, it is still
	// good practice to do so.)
	inline void BinaryOutputStream::close()
	{
		if (name.empty()) {
			return;
		}
		if (compressionType == CompressionType::NONE) {
			std::ofstream stream(name, std::ios::out | std::ios::binary | std::ios::ate);
			std::ostream_iterator<byte> output_iterator(stream);
			std::copy(bytes.begin(), bytes.end(), output_iterator);
			stream.close();
		}
		else if (compressionType == CompressionType::ZLIB) {
			// TODO
		}
	}

	inline byte* BinaryOutputStream::getArray()
	{
		return &bytes[0];
	}

	inline int BinaryOutputStream::length()
	{
		return bytes.size();
	}

	/*
		This is the input stream for binary files.
	*/
	// This class handles the reading of binary data from files and memory.
	// Similar to the BinaryOutputStream it translates big endian primative to
	// little endian automatically.
	//
	// To create the BinaryInputStream in memory only mode please construct the class
	// with with your array of bytes. (You can decompress data. [TODO])
	//
	// Technical Note: The entire file is loaded into memory at the beging to deal with compression.
	// (TODO: Maybe stream the file if there is not compression.)
	class BinaryInputStream {
	public:
		BinaryInputStream(std::string file_name, CompressionType type = CompressionType::NONE);
		BinaryInputStream(byte data[], CompressionType type = CompressionType::NONE);
		BinaryInputStream(byte* data, CompressionType type = CompressionType::NONE);
		~BinaryInputStream();

		byte readByte();
		void readBytes(byte* b, int size);
		void readBytes(byte b[]);
		template<size_t size> void readBytes(std::array<byte, size> s);

		short readShort();
		int readInt();
		__int64 readLong();
		double readDouble();
		float readFloat();

		__int16 readInt16();
		__int32 readInt32();

		std::string readString(int size);

		void close();

	private:
		byte* bytes;
		std::string name;
		CompressionType compressionType;
		long currentIndex;
		long fileSize;
	};

	inline BinaryInputStream::BinaryInputStream(std::string file_name, CompressionType type)
	{
		name = file_name;
		compressionType = type;
		currentIndex = 0;
		if (compressionType == CompressionType::NONE) {
			std::ifstream stream(name, std::ios::in | std::ios::binary | std::ios::ate);
			if (stream.is_open()) {
				stream.seekg(0, stream.end);
				fileSize = stream.tellg();
				stream.seekg(0, stream.beg);
				bytes = new byte[fileSize];
				stream.read(bytes, fileSize);
			}
			else {
				throw ODS::ODSException("File stream not open! Does that file exist?");
			}
			stream.close();
		}
	}

	inline BinaryInputStream::BinaryInputStream(byte data[], CompressionType type = CompressionType::NONE)
	{
		// TODO DECOMPRESS DATA
		name = "";
		compressionType = type;
		currentIndex = 0;
		this->bytes = data;
	}

	inline BinaryInputStream::BinaryInputStream(byte* data, CompressionType type)
	{
		// TODO DECOMPRESS DATA
		name = "";
		compressionType = type;
		currentIndex = 0;
		this->bytes = data;
	}

	inline BinaryInputStream::~BinaryInputStream()
	{
		//delete[] bytes;
	}

	inline byte BinaryInputStream::readByte()
	{
		return bytes[currentIndex++];
	}

	inline void BinaryInputStream::readBytes(byte* b, int size)
	{
		for (int i = 0; i < size; i++) {
			b[i] = bytes[currentIndex];
			currentIndex++;
		}
	}

	inline void BinaryInputStream::readBytes(byte b[])
	{
		for (int i = 0; i < sizeof(b); i++) {
			b[i] = bytes[currentIndex];
			currentIndex++;
		}
	}

	template<size_t size>
	inline void ODS::BinaryInputStream::readBytes(std::array<byte, size> s)
	{
		memcpy(s, bytes + currentIndex, size);
		currentIndex += size;
	}

	inline short BinaryInputStream::readShort()
	{
		byte* tempData = new byte[2];
		readBytes(tempData, 2);
		return swap_endian((short)((tempData[1] << 8) | tempData[0]));
	}

	inline int BinaryInputStream::readInt()
	{
		byte* tempData = new byte[4];
		readBytes(tempData, 4);
		return swap_endian((int)(tempData[3] << 24) | (tempData[2] << 16) | (tempData[1] << 8) | (tempData[0]));
	}

	inline __int64 BinaryInputStream::readLong()
	{
		byte* tempData = new byte[8];
		readBytes(tempData, 8);
		__int64* d = reinterpret_cast<__int64*>(tempData);
		return swap_endian(*d);
	}

	inline double BinaryInputStream::readDouble()
	{
		byte* tempData = new byte[8];
		readBytes(tempData, 8);
		double* d = reinterpret_cast<double*>(tempData);
		return swap_endian(*d);
	}

	inline float BinaryInputStream::readFloat()
	{
		byte* tempData = new byte[4];
		readBytes(tempData, 4);
		float* d = reinterpret_cast<float*>(tempData);
		return swap_endian(*d);
	}

	inline __int16 BinaryInputStream::readInt16()
	{
		byte* tempData = new byte[2];
		readBytes(tempData, 2);
		__int16* d = reinterpret_cast<__int16*>(tempData);
		return swap_endian(*d);
	}

	inline __int32 BinaryInputStream::readInt32()
	{
		byte* tempData = new byte[4];
		readBytes(tempData, 2);
		__int32* d = reinterpret_cast<__int32*>(tempData);
		return swap_endian(*d);
	}

	inline std::string BinaryInputStream::readString(int size)
	{
		byte* tempData = new byte[size + 1];
		readBytes(tempData, size);
		tempData[size] = '\0';
		return std::string(tempData);
	}

	inline void BinaryInputStream::close()
	{
		delete[] bytes;
	}

	/**
	====================================
		ODS TAGS
		These are the tags of ODS.
	====================================
	*/
	// An ITag is used so that way you can have a vector of tags without knowing the primative type.
	// Example: std::vector<ITag*> vec();
	//
	// DO NOT construct this class as it is an abstract class. This class is only to be used to refrence and unknown tag type
	// or to be extended.
	//
	// Calling any of these methods directly will result in an ODSException.
	class ITag {
	public:
		virtual std::string getName() { throw ODSException("INVALID OPERATION"); };
		virtual void setName(std::string name) { throw ODSException("INVALID OPERATION"); };
		virtual void writeData(BinaryOutputStream& bos) { throw ODSException("INVALID OPERATION"); };
		virtual byte getID() { throw ODSException("INVALID OPERATION"); };
	};

	// This is the abstract template class for the tag. Please do not construct this
	// template class on its own.
	//
	// class T is the primative type the tag is based on. For Example: The StringTag is based upon
	// the std::string class. 
	//
	// Calling any of these methods directly will result in an ODSException.
	template <class T> class Tag : public ITag{
	public:
		virtual T getValue() { throw ODSException("INVALID OPERATION"); };
		virtual void setValue(T t){ throw ODSException("INVALID OPERATION"); };
		virtual std::string getName() { throw ODSException("INVALID OPERATION"); };
		virtual void setName(std::string name) { throw ODSException("INVALID OPERATION"); };

		virtual void writeData(BinaryOutputStream& bos) { throw ODSException("INVALID OPERATION"); };
		virtual Tag<T> createFromData(byte value[], int length) { throw ODSException("INVALID OPERATION"); };

		virtual byte getID() { throw ODSException("INVALID OPERATION"); };
	};

	/*
	
		Byte Tag
	
	*/
	class ByteTag : public Tag<byte> {
	private:
		std::string name;
		byte value;

	public:
		ByteTag(std::string name, byte value);
		~ByteTag();

		void setValue(byte b);
		byte getValue();
		void setName(std::string);
		std::string getName();

		void writeData(BinaryOutputStream& bos);
		Tag<byte> createFromData(byte value[], int length);
		byte getID();
	};

	inline ByteTag::ByteTag(std::string name, byte value)
	{
		this->name = name;
		this->value = value;
	}

	inline ByteTag::~ByteTag()
	{
	}

	inline void ByteTag::setValue(byte b)
	{
		this->value = b;
	}

	inline byte ByteTag::getValue()
	{
		return value;
	}

	inline void ByteTag::setName(std::string)
	{
		this->name = name;
	}

	inline std::string ByteTag::getName()
	{
		return name;
	}

	inline void ByteTag::writeData(BinaryOutputStream& bos)
	{
		bos.writeByte(getID());
		// Memory only stream
		BinaryOutputStream tempBOS = BinaryOutputStream();
		tempBOS.writeShort(name.length());
		tempBOS.writeString(name);
		tempBOS.writeByte(value);

		bos.writeInt(tempBOS.length());
		bos.writeByte(tempBOS.getArray(), tempBOS.length());
		tempBOS.close();
	}

	
	inline Tag<byte> ByteTag::createFromData(byte value[], int length)
	{
		this->value = value[0];
		return *this;
	}
	

	inline byte ByteTag::getID()
	{
		return 8;
	}

	/******************************

		Char Tag
		(Doesn't have much purpose in c++, exists only to be compatible with other ODS versions.)

	*******************************
	*/
	class CharTag : public Tag<char> {
	private:
		std::string name;
		char value;

	public:
		CharTag(std::string name, byte value);
		~CharTag();

		void setValue(char b);
		char getValue();
		void setName(std::string);
		std::string getName();

		void writeData(BinaryOutputStream& bos);
		Tag<char> createFromData(byte value[], int length);
		byte getID();
	};

	inline CharTag::CharTag(std::string name, char value)
	{
		this->name = name;
		this->value = value;
	}

	inline CharTag::~CharTag()
	{
	}

	inline void CharTag::setValue(char b)
	{
		this->value = b;
	}

	inline byte CharTag::getValue()
	{
		return value;
	}

	inline void CharTag::setName(std::string)
	{
		this->name = name;
	}

	inline std::string CharTag::getName()
	{
		return name;
	}

	inline void CharTag::writeData(BinaryOutputStream& bos)
	{
		bos.writeByte(getID());
		// Memory only stream
		BinaryOutputStream tempBOS = BinaryOutputStream();
		tempBOS.writeShort(name.length());
		tempBOS.writeString(name);
		tempBOS.writeByte(value);

		bos.writeInt(tempBOS.length());
		bos.writeByte(tempBOS.getArray(), tempBOS.length());
		tempBOS.close();
	}


	inline Tag<char> CharTag::createFromData(byte value[], int length)
	{
		this->value = value[0];
		return *this;
	}


	inline byte CharTag::getID()
	{
		return 7;
	}

	/******************************

		Double Tag

	*******************************
	*/
	class DoubleTag : public Tag<double> {
	private:
		std::string name;
		double value;

	public:
		DoubleTag(std::string name, double value);
		~DoubleTag();

		void setValue(double b);
		double getValue();
		void setName(std::string);
		std::string getName();

		void writeData(BinaryOutputStream& bos);
		Tag<double> createFromData(byte value[], int length);
		byte getID();
	};

	inline DoubleTag::DoubleTag(std::string name, double value)
	{
		this->name = name;
		this->value = value;
	}

	inline DoubleTag::~DoubleTag()
	{
	}

	inline void DoubleTag::setValue(double b)
	{
		this->value = b;
	}

	inline double DoubleTag::getValue()
	{
		return value;
	}

	inline void DoubleTag::setName(std::string)
	{
		this->name = name;
	}

	inline std::string DoubleTag::getName()
	{
		return name;
	}

	inline void DoubleTag::writeData(BinaryOutputStream& bos)
	{
		bos.writeByte(getID());
		// Memory only stream
		BinaryOutputStream tempBOS = BinaryOutputStream();
		tempBOS.writeShort(name.length());
		tempBOS.writeString(name);
		tempBOS.writeDouble(value);

		bos.writeInt(tempBOS.length());
		bos.writeByte(tempBOS.getArray(), tempBOS.length());
		tempBOS.close();
	}


	inline Tag<double> DoubleTag::createFromData(byte value[], int length)
	{
		this->value = *reinterpret_cast<double*>(*value);
		return *this;
	}


	inline byte DoubleTag::getID()
	{
		return 4;
	}

	/******************************

		Float Tag

	*******************************
	*/
	class FloatTag : public Tag<float> {
	private:
		std::string name;
		float value;

	public:
		FloatTag(std::string name, float value);
		~FloatTag();

		void setValue(float b);
		float getValue();
		void setName(std::string);
		std::string getName();

		void writeData(BinaryOutputStream& bos);
		Tag<float> createFromData(byte value[], int length);
		byte getID();
	};

	inline FloatTag::FloatTag(std::string name, float value)
	{
		this->name = name;
		this->value = value;
	}

	inline FloatTag::~FloatTag()
	{
	}

	inline void FloatTag::setValue(float b)
	{
		this->value = b;
	}

	inline float FloatTag::getValue()
	{
		return value;
	}

	inline void FloatTag::setName(std::string)
	{
		this->name = name;
	}

	inline std::string FloatTag::getName()
	{
		return name;
	}

	inline void FloatTag::writeData(BinaryOutputStream& bos)
	{
		bos.writeByte(getID());
		// Memory only stream
		BinaryOutputStream tempBOS = BinaryOutputStream();
		tempBOS.writeShort(name.length());
		tempBOS.writeString(name);
		tempBOS.writeFloat(value);

		bos.writeInt(tempBOS.length());
		bos.writeByte(tempBOS.getArray(), tempBOS.length());
		tempBOS.close();
	}


	inline Tag<float> FloatTag::createFromData(byte value[], int length)
	{
		this->value = *reinterpret_cast<float*>(*value);
		return *this;
	}


	inline byte FloatTag::getID()
	{
		return 3;
	}

	/******************************

		Int Tag

	*******************************
	*/
	class IntTag : public Tag<int> {
	private:
		std::string name;
		int value;

	public:
		IntTag(std::string name, int value);
		~IntTag();

		void setValue(int b);
		int getValue();
		void setName(std::string);
		std::string getName();

		void writeData(BinaryOutputStream& bos);
		Tag<int> createFromData(byte value[], int length);
		byte getID();
	};

	inline IntTag::IntTag(std::string name, int value)
	{
		this->name = name;
		this->value = value;
	}

	inline IntTag::~IntTag()
	{
	}

	inline void IntTag::setValue(int b)
	{
		this->value = b;
	}

	inline int IntTag::getValue()
	{
		return value;
	}

	inline void IntTag::setName(std::string)
	{
		this->name = name;
	}

	inline std::string IntTag::getName()
	{
		return name;
	}

	inline void IntTag::writeData(BinaryOutputStream& bos)
	{
		bos.writeByte(getID());
		// Memory only stream
		BinaryOutputStream tempBOS = BinaryOutputStream();
		tempBOS.writeShort(name.length());
		tempBOS.writeString(name);
		tempBOS.writeInt(value);

		bos.writeInt(tempBOS.length());
		bos.writeByte(tempBOS.getArray(), tempBOS.length());
		tempBOS.close();
	}


	inline Tag<int> IntTag::createFromData(byte value[], int length)
	{
		this->value = *reinterpret_cast<int*>(*value);
		return *this;
	}


	inline byte IntTag::getID()
	{
		return 2;
	}

	/******************************

		Invalid Tag

	*******************************
	*/
	class InvalidTag : public Tag<byte*> {
	private:
		std::string name;
		byte* value;

	public:
		InvalidTag(std::string name, byte* value);
		~InvalidTag();

		void setValue(byte* b);
		byte* getValue();
		void setName(std::string);
		std::string getName();

		void writeData(BinaryOutputStream& bos);
		Tag<byte*> createFromData(byte value[], int length);
		byte getID();
	};

	inline InvalidTag::InvalidTag(std::string name, byte* value)
	{
		this->name = name;
		this->value = value;
	}

	inline InvalidTag::~InvalidTag()
	{
	}

	inline void InvalidTag::setValue(byte* b)
	{
		this->value = b;
	}

	inline byte* InvalidTag::getValue()
	{
		return value;
	}

	inline void InvalidTag::setName(std::string)
	{
		this->name = name;
	}

	inline std::string InvalidTag::getName()
	{
		return name;
	}

	inline void InvalidTag::writeData(BinaryOutputStream& bos)
	{
		throw ODSException("Error: Cannot write an Invalid Tag!");
	}


	inline Tag<byte*> InvalidTag::createFromData(byte value[], int length)
	{
		this->value = reinterpret_cast<byte*>(value);
		return *this;
	}


	inline byte InvalidTag::getID()
	{
		return 0;
	}

	/******************************

		Vector Tag
		(Replaces this ListTag of the C# and Java versions).

	*******************************
	*/
	class VectorTag : public Tag<std::vector<std::shared_ptr <ITag>>> {
	private:
		std::string name;
		std::vector<std::shared_ptr <ITag>> value;

	public:
		VectorTag(std::string name, std::vector<std::shared_ptr <ITag>> value);
		~VectorTag();

		void setValue(std::vector<std::shared_ptr <ITag>> b);
		std::vector<std::shared_ptr <ITag>> getValue();
		void setName(std::string);
		std::string getName();

		void addTag(std::shared_ptr <ITag> tag);
		void removeTag(std::shared_ptr <ITag> tag);
		std::shared_ptr <ITag> getTag(int i);
		void removeAllTags();
		int indexOf(std::shared_ptr <ITag> tag);

		void writeData(BinaryOutputStream& bos);
		Tag<std::vector<std::shared_ptr <ITag>>> createFromData(byte value[], int length);
		byte getID();

		VectorTag& operator+=(ITag* tag) {
			value.push_back((std::shared_ptr <ITag>) tag);
			return *this;
		}
	};

	inline VectorTag::VectorTag(std::string name, std::vector<std::shared_ptr <ITag>> value)
	{
		this->name = name;
		this->value = value;
	}

	inline VectorTag::~VectorTag()
	{
		value.clear();
	}

	inline void VectorTag::setValue(std::vector<std::shared_ptr <ITag>> b)
	{
		this->value = b;
	}

	inline std::vector<std::shared_ptr <ITag>> VectorTag::getValue()
	{
		return value;
	}

	inline void VectorTag::setName(std::string)
	{
		this->name = name;
	}

	inline std::string VectorTag::getName()
	{
		return name;
	}

	inline void VectorTag::addTag(std::shared_ptr <ITag> tag)
	{
		value.push_back(tag);
	}

	inline void VectorTag::removeTag(std::shared_ptr <ITag> tag)
	{
		int i = 0;
		for (std::shared_ptr <ITag> t : value) {
			if (t == tag)
				break;
			i++;
		}
		value.erase(value.begin() + i);
	}

	inline std::shared_ptr <ITag> VectorTag::getTag(int i)
	{
		return value[i];
	}

	inline void VectorTag::removeAllTags()
	{
		value.clear();
	}

	inline int VectorTag::indexOf(std::shared_ptr <ITag> tag)
	{
		int i = 0;
		for (std::shared_ptr <ITag> t : value) {
			if (t == tag)
				break;
			i++;
		}
		return i;
	}

	inline void VectorTag::writeData(BinaryOutputStream& bos)
	{
		bos.writeByte(getID());
		// Memory only stream
		BinaryOutputStream tempBOS = BinaryOutputStream();
		tempBOS.writeShort(name.length());
		tempBOS.writeString(name);
		
		for (std::shared_ptr <ITag> tag : this->value) {
			tag->setName("");
			tag->writeData(tempBOS);
		}

		bos.writeInt(tempBOS.length());
		bos.writeByte(tempBOS.getArray(), tempBOS.length());
		tempBOS.close();
	}


	inline Tag<std::vector<std::shared_ptr <ITag>>> VectorTag::createFromData(byte value[], int length)
	{
		// TODO Implement this when I make ObjectDataStructure.getListData(value, length);
		throw ODSException("NOT IMPLEMENTED");
	}


	inline byte VectorTag::getID()
	{
		return 9;
	}

	/******************************

		Long Tag

	*******************************
	*/
	class LongTag : public Tag<long> {
	private:
		std::string name;
		long value;

	public:
		LongTag(std::string name, long value);
		~LongTag();

		void setValue(long b);
		long getValue();
		void setName(std::string);
		std::string getName();

		void writeData(BinaryOutputStream& bos);
		Tag<long> createFromData(byte value[], int length);
		byte getID();
	};

	inline LongTag::LongTag(std::string name, long value)
	{
		this->name = name;
		this->value = value;
	}

	inline LongTag::~LongTag()
	{
	}

	inline void LongTag::setValue(long b)
	{
		this->value = b;
	}

	inline long LongTag::getValue()
	{
		return value;
	}

	inline void LongTag::setName(std::string)
	{
		this->name = name;
	}

	inline std::string LongTag::getName()
	{
		return name;
	}

	inline void LongTag::writeData(BinaryOutputStream& bos)
	{
		bos.writeByte(getID());
		// Memory only stream
		BinaryOutputStream tempBOS = BinaryOutputStream();
		tempBOS.writeShort(name.length());
		tempBOS.writeString(name);
		tempBOS.writeLong(value);

		bos.writeInt(tempBOS.length());
		bos.writeByte(tempBOS.getArray(), tempBOS.length());
		tempBOS.close();
	}


	inline Tag<long> LongTag::createFromData(byte value[], int length)
	{
		this->value = reinterpret_cast<long>(value);
		return *this;
	}


	inline byte LongTag::getID()
	{
		return 6;
	}

	/******************************

		Object Tag

	*******************************
	*/
	class ObjectTag : public Tag<std::vector<ITag*>> {
	private:
		std::string name;
		std::vector<ITag*> value;

	public:
		ObjectTag(std::string name, std::vector<ITag*> value);
		ObjectTag(std::string name);
		~ObjectTag();

		void setValue(std::vector<ITag*> b);
		std::vector<ITag*> getValue();
		void setName(std::string);
		std::string getName();

		void addTag(ITag* tag);
		void removeTag(ITag* tag);
		ITag* getTag(std::string name);
		void removeAllTags();

		void writeData(BinaryOutputStream& bos);
		Tag<std::vector<ITag*>> createFromData(byte value[], int length);
		byte getID();
	};

	inline ObjectTag::ObjectTag(std::string name, std::vector<ITag*> value)
	{
		this->name = name;
		this->value = value;
	}

	inline ObjectTag::ObjectTag(std::string name) {
		this->name = name;
		this->value = std::vector<ITag*>();
	}

	inline ObjectTag::~ObjectTag()
	{
		value.clear();
	}

	inline void ObjectTag::setValue(std::vector<ITag*> b)
	{
		this->value = b;
	}

	inline std::vector<ITag*> ObjectTag::getValue()
	{
		return value;
	}

	inline void ObjectTag::setName(std::string)
	{
		this->name = name;
	}

	inline std::string ObjectTag::getName()
	{
		return name;
	}

	inline void ObjectTag::addTag(ITag* tag)
	{
		value.push_back(std::move(tag));
	}

	inline void ObjectTag::removeTag(ITag* tag)
	{
		int i = 0;
		for (ITag* t : value) {
			if (t == tag)
				break;
			i++;
		}
		value.erase(value.begin() + i);
	}

	inline ITag* ObjectTag::getTag(std::string name)
	{
		return NULL;
	}

	inline void ObjectTag::removeAllTags()
	{
		value.clear();
	}

	inline void ObjectTag::writeData(BinaryOutputStream& bos)
	{
		bos.writeByte(getID());
		// Memory only stream
		BinaryOutputStream tempBOS = BinaryOutputStream();
		tempBOS.writeShort(name.length());
		tempBOS.writeString(name);

		for (ITag* tag : this->value) {
			tag->setName("");
			tag->writeData(tempBOS);
		}

		bos.writeInt(tempBOS.length());
		bos.writeByte(tempBOS.getArray(), tempBOS.length());
		tempBOS.close();
	}


	inline Tag<std::vector<ITag*>> ObjectTag::createFromData(byte value[], int length)
	{
		// TODO Implement this when I make ObjectDataStructure.getListData(value, length);
		throw ODSException("NOT IMPLEMENTED");
	}


	inline byte ObjectTag::getID()
	{
		return 11;
	}


	/*
	===========================================
	
	Object Data Structure Class

	===========================================
	*/

	class ObjectDataStructure {
	private:
		std::string file_name;
		CompressionType compression;

	public:
		ObjectDataStructure(std::string file_name);
		ObjectDataStructure(std::string file_name, CompressionType compression);
		~ObjectDataStructure();
		
		void save(std::vector< std::shared_ptr<ITag>> tags);
		void save(std::vector<ITag*> tags);
	};

	inline ObjectDataStructure::ObjectDataStructure(std::string file_name)
	{
		this->file_name = file_name;
		this->compression = CompressionType::NONE;
	}

	inline ObjectDataStructure::ObjectDataStructure(std::string file_name, CompressionType compression)
	{
		this->file_name = file_name;
		this->compression = compression;
	}

	inline ObjectDataStructure::~ObjectDataStructure()
	{
	}

	inline void ObjectDataStructure::save(std::vector<std::shared_ptr<ITag>> tags)
	{
		BinaryOutputStream bos = BinaryOutputStream(file_name, compression);
		for (std::shared_ptr <ITag> &tag : tags) {
			tag->writeData(bos);
		}
		bos.close();
	}

	inline void ObjectDataStructure::save(std::vector<ITag*> tags)
	{
		BinaryOutputStream bos = BinaryOutputStream(file_name, compression);
		for (ITag* tag : tags) {
			tag->writeData(bos);
		}
		bos.close();
	}


}

#endif // !ODS_HEADER
