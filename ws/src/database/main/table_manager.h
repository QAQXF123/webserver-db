#pragma once
#include<fstream>
#include<unordered_map>
#include<vector>
#include<iostream>
#include <thread>
#include <chrono>

namespace bustub{
class Tuple;
class Table;
class TableManager{
	public:
		TableManager();
		~TableManager() = default;
		Table *GetTable(const std::string &tableName);
		bool Insert(const std::string& tableName, const std::vector<std::string> &values);
		bool GetTuple(const std::string& tableName, uint64_t key, Tuple *tuple);
		bool GetValue(const std::string& tableName, uint64_t key, const std::string &colName, std::string &value);
		void test1();
		void test2();
		//void Flush();
			
		
		
	
	private:
		std::unordered_map<std::string, Table*> getTable_;
		std::vector<std::string> tableInfos;
		std::thread *flushThread;

};

}