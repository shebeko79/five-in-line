#ifndef bin_index_utilsH
#define bin_index_utilsH
#include <stdio.h>
#include <vector>
#include <string>
#include <filesystem>
#include <set>


namespace Gomoku
{
typedef std::vector<unsigned char> data_t;
typedef std::vector<data_t> datas_t;

struct file_hldr
{
	FILE* f;
	file_hldr(FILE* _f) {f=_f;}
	~file_hldr(){if(f)fclose(f);}
};

void load_file(const std::string& file_name,data_t& res);
void save_file(const std::string& file_name,const data_t& res);

void load_file(const std::filesystem::path& file_name,data_t& res);
void save_file(const std::filesystem::path& file_name,const data_t& res);

void hex2bin(const std::string& str,data_t& bin);
void bin2hex(const data_t& bin,std::string& str);

template<typename T>
inline void pack_raw(const T& val,data_t& bin)
{
    const unsigned char* p=reinterpret_cast<const unsigned char*>(&val);
	bin.insert(bin.end(),p,p+sizeof(T));
}

template<typename T>
inline void unpack_raw(const data_t& bin,T& val,size_t& from,const char* throw_message)
{
	if(from+sizeof(T)>=bin.size())
		throw std::runtime_error(throw_message);
	
    val=*(const T*)(&bin[from]);
	from+=sizeof(T);
}

template<typename T, typename Pr, typename Alloc>
void pack(const std::set<T,Pr,Alloc>& container, data_t& bin)
{
	size_t cnt=container.size();
	const unsigned char* pcnt=reinterpret_cast<const unsigned char*>(&cnt);
	bin.insert(bin.end(),pcnt,pcnt+sizeof(cnt));

	for(const auto& v : container)
		v.pack(bin);
}

template<typename T, typename Pr, typename Alloc>
void unpack(std::set<T,Pr,Alloc>& container, const data_t& bin, size_t& from)
{
	container.clear();

	if(from+sizeof(size_t)>bin.size())
		throw std::runtime_error(std::string(__func__)+": unpack size failed");

	size_t cnt=*reinterpret_cast<const size_t*>(&bin[from]);
	from+=sizeof(size_t);

	for (size_t i = 0; i < cnt; i++)
	{
		T val;
		val.unpack(bin,from);
		container.insert(val);
	}
}

}//namespace

#endif
