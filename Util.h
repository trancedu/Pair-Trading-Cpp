#pragma once
#include <map>
#include <string>
using namespace std;


// Process config file for Market Data Retrieval
map<string, string> ProcessConfigData(string config_file);

// writing call back function for storing fetched values in memory
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

map<string, string> GetPairs(string pair_file);

// Some functions for user input of a double
bool ValidateDouble(const std::string& str);
double ReadDouble(const std::string& prompt);



