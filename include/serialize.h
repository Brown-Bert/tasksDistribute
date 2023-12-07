#ifndef SERIALIZE_H_
#define SERIALIZE_H_
#include <iostream>
#include <vector>
#include <cstring>
#include <fstream>
#include <chrono>
using namespace std;
class DataStream {
public:
  enum DataType
  {
    BOOL = 0,
    CHAR,
    INT32,
    INT64,
    FLOAT,
    DOUBLE,
    STRING,
    VECTOR,
    LIST,
    MAP,
    SET,
    BPLUSNODE
  };
  DataStream();
  ~DataStream();
  void show() const;
  bool writeIntoFile();
  bool readFromFile(ifstream &ifs, int nums);
  void write(const char* data, int len);
  void write(bool value);
  void write(char value);
  void write(int32_t value);
  void write(int64_t value);
  void write(float value);
  void write(double value);
  void write(const char* value);
  void write(const string& value);
  // void write(BPlusNode node);
  // void write(Student student);
  DataStream& operator << (bool value);
  DataStream& operator << (char value);
  DataStream& operator << (int32_t value);
  DataStream& operator << (int64_t value);
  DataStream& operator << (float value);
  DataStream& operator << (double value);
  DataStream& operator << (const char* value);
  DataStream& operator << (const string & value);
  // DataStream& operator << (BPlusNode &node_ptr);
  // DataStream& operator << (Student &student);

  bool read(bool &value);
  bool read(char &value);
  bool read(int32_t &value);
  bool read(int64_t &value);
  bool read(float &value);
  bool read(double &value);
  bool read(string& value);
  // bool read(BPlusNode& node);
  // bool read(Student& student);
  DataStream& operator >> (bool &value);
  DataStream& operator >> (char &value);
  DataStream& operator >> (int32_t &value);
  DataStream& operator >> (int64_t &value);
  DataStream& operator >> (float &value);
  DataStream& operator >> (double &value);
  DataStream& operator >> (string& value);
  // DataStream& operator >> (BPlusNode& node);
  // DataStream& operator >> (Student& student);

  void clear();
private:
  vector<char> m_buf;
  int position;
  int num; // 一次性要写入的节点个数
  vector<int> node_num; // 每一个元素代表当前节点所占字节数
  int current_num = 0; // 记录当前有多少个节点
private:
  void reserve(int len);
};
#endif