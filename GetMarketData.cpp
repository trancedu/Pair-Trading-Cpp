//
//  GetMarketData.cpp
//  PairTrading
//
//  Created by 杜学渊 on 2/19/22.
//


#include "GetMarketData.h"


int PullMarketData(const std::string& url_request, std::string& read_buffer)
{

    //global initiliation of curl before calling a function
    curl_global_init(CURL_GLOBAL_ALL);

    //creating session handle
    CURL * handle;

    // Store the result of CURLís webpage retrieval, for simple error checking.
    CURLcode result;

    // notice the lack of major error-checking, for brevity
    handle = curl_easy_init();
    
    if (!handle)
    {
        cout << "curl_easy_init failed" << endl;
        return -1;
    }
    
    curl_easy_setopt(handle, CURLOPT_URL, url_request.c_str());
//    curl_easy_setopt(handle, CURLOPT_URL, url);
    
    //adding a user agent
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:74.0) Gecko/20100101 Firefox/74.0");
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);

    // send all data to this function
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);

    // we pass our 'chunk' struct to the callback function
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &read_buffer);

    //perform a blocking file transfer
    result = curl_easy_perform(handle);

    // check for errors
    if (result != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
        cout << "curl: " << url_request.c_str() << endl;
        return -1;
    }
    
    return 0;
}


int PopulateDailyTrades(const std::string& read_buffer,
                    Stock& stock)
{
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();
    Json::Value root;   // will contains the root value after parsing.
    string errors;

    bool parsingSuccessful = reader->parse(read_buffer.c_str(), read_buffer.c_str() + read_buffer.size(), &root, &errors);
    if (not parsingSuccessful)
    {
        // Report failures and their locations in the document.
        cout << "Failed to parse JSON" << endl << read_buffer << errors << endl;
        return -1;
    }
    else
    {
//        cout << "\nSucess parsing json\n" << root << endl;
        string date;
        float open, high, low, close, adjusted_close;
        long volume;
        for (Json::Value::const_iterator itr = root.begin(); itr != root.end(); itr++)
        {
            date = (*itr)["date"].asString();
            open = (*itr)["open"].asFloat();
            high = (*itr)["high"].asFloat();
            low = (*itr)["low"].asFloat();
            close = (*itr)["close"].asFloat();
            adjusted_close = (*itr)["adjusted_close"].asFloat();
            volume = (*itr)["volume"].asInt64();
            TradeData aTrade(date, open, high, low, close, adjusted_close, volume);
            stock.addTrade(aTrade);
        }
//        cout << stock << endl;
    }
    return 0;
};
