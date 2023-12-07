#include "../include/serialize.h"

using namespace std;


// 构造函数
DataStream::DataStream():position(0), num(100){

}
// 析构函数
DataStream::~DataStream() {

}
// 清除
void DataStream::clear(){
  m_buf.clear();
  position = 0;
}
// 扩容
void DataStream::reserve(int len) {
  int size = m_buf.size();
  int cap = m_buf.capacity();
  if (size + len > cap) {
    while (size + len > cap)
    {
      if (cap == 0) {
        cap = 1;
      }
      else
      {
        cap *= 2;
      }
    }
    m_buf.reserve(cap);
  }
}
bool DataStream::writeIntoFile() {
  //string file_path_name = "./test.out";
  string file_path_name = "./application.properties";
  ofstream ofs;
  ofs.open(file_path_name, ios::app | ios::binary);
  if (!ofs.is_open()) {
    cout << "create failed\n" << endl;
    return false;
  }
  // 把m_buf的数据写入到文件中
  int i;
  for (i = 0; i < m_buf.size(); i++){
    ofs.put(m_buf[i]);
  }
  // cout << "i = " << i << endl;
  // ofs.put(m_buf[pos + j]);
  // int pos = 0; // 记录位置
  // int count = 1;
  // for(vector<int>::iterator it = node_num.begin(); it != node_num.end(); it++){
  //   int j =  0;
  //   for (j; j < (*it); j++){
  //     ofs.put(m_buf[pos + j]);
  //   }
  //   pos += j;
  // }
  // 所有节点写完之后，清空数据
  // node_num.clear();
  m_buf.clear();
  // current_num = 0;
  // 关闭文件
  ofs.close();
  return true;
}

bool DataStream::readFromFile(ifstream &ifs, int nums) { // 读取nums个字符
  // string file_path_name = "./binary.out";
  string buf;
  int i = 0;
  char c;
  while (ifs.get(c))
  {

    buf.append(1, c);
    i++;
    if (i == nums) {
      break;
    }
  }
  write(buf.data(), buf.size());
  /*for_each(V.begin(), V.end(), printVector);
  cout << endl;*/
  return true;
}
// 把数据写入vector中
void DataStream::write(const char* data, int len) {
  double s = *(data);
  reserve(len);
  int size = m_buf.size();
  // cout << "size = " << size << endl;
  m_buf.resize(size + len);
  memcpy(&m_buf[size], data, len);
}
// 对bool类型的数据编码
void DataStream::write(bool value) {
  char type = DataType::BOOL;
  write((char*)&type, sizeof(char));
  write((char*)&value, sizeof(char));
}
// 对char数据类型编码
void DataStream::write(char value) {
  char type = DataType::CHAR;
  write((char*)&type, sizeof(char));
  write((char*)&value, sizeof(char));
}

// 对int32位的类型编码
void DataStream::write(int32_t value) {
  char type = DataType::INT32;
  write((char*)&type, sizeof(char));
  write((char*)&value, sizeof(int32_t));
}
// 对int64位的类型编码
void DataStream::write(int64_t value) {
  char type = DataType::INT64;
  write((char*)&type, sizeof(char));
  write((char*)&value, sizeof(int64_t));
}
// 对float的类型编码
void DataStream::write(float value) {
  char type = DataType::FLOAT;
  write((char*)&type, sizeof(char));
  write((char*)&value, sizeof(float));
}
// 对double的类型编码
void DataStream::write(double value) {
  char type = DataType::DOUBLE;
  write((char*)&type, sizeof(char));
  write((char*)&value, sizeof(double));
}
// 对c风格的字符串编码
void DataStream::write(const char* value) {
  char type = DataType::STRING;
  // write((char*)&type, sizeof(char));
  int len = strlen(value);
  // cout << "cccccc" << endl;
  // write(len);
  write(value, len);
}
// 对c++风格的字符串编码
void DataStream::write(const string& value) {
  char type = DataType::STRING;
  // write((char*)&type, sizeof(char));
  int len = value.size();
  // write(len);
  // cout << "data = " << value.data() << "-" << len << endl;
  write(value.data(), len);
}
// 对自定义的节点类进行序列化编码
// void DataStream::write(BPlusNode node){
//   // char type = DataType::INT32;
//   // write((char*)&type, sizeof(char));
//   write(node.flag);
//   write(node.name);
//   write(node.size);
// }
// void DataStream::write(Student stu){
//   write(stu.name);
//   write(stu.id);
// }
DataStream& DataStream::operator << (bool value) {
  write(value);
  return *this;
}
DataStream& DataStream::operator << (char value) {
  write(value);
  return *this;
}
DataStream& DataStream::operator << (int32_t value) {
  write(value);
  return *this;
}
DataStream& DataStream::operator << (int64_t value) {
  write(value);
  return *this;
}
DataStream& DataStream::operator << (float value) {
  write(value);
  return *this;
}
DataStream& DataStream::operator << (double value) {
  write(value);
  return *this;
}
DataStream& DataStream::operator << (const char* value) {
  write(value);
  return *this;
}
DataStream& DataStream::operator << (const string& value) {
  write(value);
  return *this;
}
// DataStream& DataStream::operator << (BPlusNode &node) {
//   write(node);
//   return *this;
// }
// DataStream& DataStream::operator << (Student &student) {
//   write(student);
//   return *this;
// }

bool DataStream::read(bool &value) {
  if ((DataType)m_buf[position] != DataType::BOOL) {
    return false;
  }
  position++;
  value = (bool)m_buf[position];
  position++;
  return true;
}
bool DataStream::read(char &value) {
  if ((DataType)m_buf[position] != DataType::CHAR) {
    return false;
  }
  position++;
  value = (char)m_buf[position];
  position++;
  return true;
}
bool DataStream::read(int32_t &value) {
  if ((DataType)m_buf[position] != DataType::INT32) {
    return false;
  }
  position++;
  value = *((int32_t *)(&m_buf[position]));
  position += 4;
  return true;
}
bool DataStream::read(int64_t &value) {
  if ((DataType)m_buf[position] != DataType::INT64) {
    return false;
  }
  position++;
  value = *((int64_t *)(&m_buf[position]));
  position += 8;
  return true;
}
bool DataStream::read(float &value) {
  if ((DataType)m_buf[position] != DataType::FLOAT) {
    return false;
  }
  position++;
  value = *((float *)(&m_buf[position]));
  position += 4;
  return true;
}
bool DataStream::read(double &value) {
  if ((DataType)m_buf[position] != DataType::DOUBLE) {
    return false;
  }
  position++;
  value = *((double *)(&m_buf[position]));
  position += 8;
  return true;
}
bool DataStream::read(string& value) {
  if ((DataType)m_buf[position] != DataType::STRING) {
    return false;
  }
  position++;
  int len;
  read(len);
  if (len < 0) {
    return false;
  }
  value.assign((char *)&m_buf[position], len);
  position += len;
  return true;
}
DataStream& DataStream::operator >> (bool& value) {
  read(value);
  return *this;
}
DataStream& DataStream::operator >> (char& value) {
  read(value);
  return *this;
}
DataStream& DataStream::operator >> (int32_t& value) {
  read(value);
  return *this;
}
DataStream& DataStream::operator >> (int64_t& value) {
  read(value);
  return *this;
}
DataStream& DataStream::operator >> (float& value) {
  read(value);
  return *this;
}
DataStream& DataStream::operator >> (double& value) {
  read(value);
  return *this;
}
DataStream& DataStream::operator >> (string& value) {
  read(value);
  return *this;
}
