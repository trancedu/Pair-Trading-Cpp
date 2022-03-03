#include <fstream>
#include <sstream>
#include <vector>
#include "Util.h"
#include <iostream>
#include <regex>
#include <string>
using namespace std;

vector<string> split(string text, char delim) {
	string line;
	vector<string> vec;
	stringstream ss(text);
	while (std::getline(ss, line, delim)) {
		vec.push_back(line);
	}
	return vec;
}

map<string, string> ProcessConfigData(string config_file)
{
	map<string, string> config_map;
	ifstream fin;
	fin.open(config_file, ios::in);
	string line, name, value;
	while (!fin.eof())
	{
		getline(fin, line);
		stringstream sin(line);
		getline(sin, name, ':');
		getline(sin, value, '\r');
//        cout << value << endl;
//        cout << value.back() << endl;
        
		config_map.insert(pair<string, string>(name, value));
	}
	return config_map;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

//read file to get pairs of stocks
map<string, string> GetPairs(string pair_file)
{
	map<string, string>pairmap;
	ifstream mfin;
	mfin.open(pair_file, ios::in);
	string line,stock1, stock2;
	while (!mfin.eof())
	{
		getline(mfin, line);
		stringstream sin(line);
		getline(sin, stock1, ',');
		getline(sin, stock2, '\r');
		pairmap.insert(pair<string, string>(stock1, stock2));

	}
	return pairmap;
}

bool ValidateDouble(const std::string& str) 
{
	const basic_regex<char> dbl(R"(^\d+\.?\d?$)");
	return regex_match(str, dbl);
}

double ReadDouble(const std::string& prompt) 
{
	std::string input;
	bool match = false;

	while (!match)
	{
		std::getline(std::cin, input);
		match = ValidateDouble(input);
		if (!match) std::cout << prompt << endl;
	}
	return std::stod(input);
}
