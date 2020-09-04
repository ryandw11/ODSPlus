/*

	This is just the test class for ODSPlus.

	Please see ods.h for the source code.

	Only include ods.h inside of your project.

*/

#include "ods.h";

using namespace ODS;


int main(void) {
	/*
	double d = 25.6;
	std::cout << sizeof(reinterpret_cast<char*>(&d)) << std::endl;
	ODS::BinaryOutputStream bos = ODS::BinaryOutputStream("example.ods");
	bos.writeShort(20);
	bos.writeInt(420);
	bos.writeDouble(25.64);
	bos.writeFloat(25.64);
	bos.writeString("This is a test.png");
	long l = 2030;
	bos.writeLong(l);
	bos.close();

	std::string fws = "This is a test.png";

	ODS::BinaryInputStream bis = ODS::BinaryInputStream("example.ods");
	std::cout << bis.readShort() << std::endl;
	std::cout << bis.readInt() << std::endl;
	std::cout << bis.readDouble() << std::endl;
	std::cout << bis.readFloat() << std::endl;
	std::cout << bis.readString(18) + "\0" << std::endl;
	std::cout << bis.readLong() << std::endl;
	bis.close();
	*/

	ODS::ObjectDataStructure ods = ODS::ObjectDataStructure("example.ods", CompressionType::NONE);
	ByteTag bt = ByteTag("yeet", 44);
	std::vector<ITag*> tags = std::vector< ITag*>();
	tags.push_back(new ByteTag("yeet", 44));
	tags.push_back(new DoubleTag("yeetwef", 90.564));
	tags.push_back(new FloatTag("mefloat", 90.888888));
	tags.push_back( new IntTag("meInt", 420));

	VectorTag* testVec = new VectorTag("Test", std::vector<std::shared_ptr <ITag>>());
	testVec->addTag((std::shared_ptr <IntTag>) new IntTag("Test", 20));
	testVec->addTag((std::shared_ptr <IntTag>) new IntTag("Test", 30));
	testVec->addTag((std::shared_ptr <IntTag>) new IntTag("Test", 60));

	tags.push_back( testVec);

	ObjectTag objTag = ObjectTag("Test");
	objTag.addTag(new LongTag("tst", 2890));
	IntTag* intTag = new IntTag("woah", 25);
	
	tags.push_back( &objTag);

	ods.save(tags);

	return 0;
}