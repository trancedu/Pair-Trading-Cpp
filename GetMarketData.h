//
//  GetMarketData.hpp
//  PairTrading
//
//  Created by 杜学渊 on 2/19/22.
//  Modified by Nicole on 03/11/2022
//


#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <thread>

#include "json/json.h"
#include "curl/curl.h"
#include <sqlite3.h>

#include "Trade.h"
#include "Util.h"


using namespace std;

int PullMarketData(const std::string& url_request, std::string& read_buffer);

int PopulateDailyTrades(const std::string& read_buffer,
                    Stock& stock);
void PullMarketDataMultiThread(const vector<std::string> & url_requests, vector<std::string> & read_buffers, int Num_threads);
int PullMarketDataMultiURL(vector<std::string>::const_iterator url_requests, vector<std::string>::iterator read_buffers, int size);
