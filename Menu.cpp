//
//  main.cpp
//  PairTrading
//
//  Created by 杜学渊 on 2/19/22.
//

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sqlite3.h>

#include "Trade.hpp"
#include "Database.h"

#include "json/json.h"
#include "curl/curl.h"

#include "Util.h"
#include "GetMarketData.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
    
    
    string database_name = "PairTrading.db";
    cout << "Opening Database..." << endl;
    sqlite3* db = NULL;
    if (OpenDatabase(database_name.c_str(), db) != 0)      return -1;

    vector<string> symbols;
    symbols.push_back("MSFT");
    symbols.push_back("TWTR");
    symbols.push_back("IBM");
    map<string, Stock> stockMap;
    
    string sConfigFile = "config.csv";
    map<string, string> config_map = ProcessConfigData(sConfigFile);
    
    
    string daily_url_common = config_map["daily_url_common"];
//    cout << daily_url_common.back() << endl;
    string intraday_url_common = config_map["intraday_url_common"];
    string start_date = config_map["start_date"];
    string end_date = config_map["end_date"];
    long start_date_intraday = 1640995200;
    long end_date_intraday = 1646092800;
    string api_token = config_map["api_token"];
//    string symbol = config_map["symbol"];
    
    
    
    
    
    bool bCompleted = false;
    char selection;
    while (!bCompleted)
    {
        cout << "Menu" << endl;
        cout << "A - Create and Populate Pair Table\n"
                "B - Retrieve and Populate Historical Data for Each Stock\n"
                "C - Create PairPrices Table\n"
                "D - Calculate Volatility\n"
                "E - Back Test\n"
                "F - Calculate Profit and Loss For Each Pair\n"
                "G - Manual Testing\n"
                "H - Drop All the Tables\n"
                "X - Exit" << endl;
        cout << "Please select an option: " << endl;
        cin >> selection;
        cout << "You choose " << selection << endl;
        switch (selection)
        {
            case 'A':
            {
                // drop table existed, create table populate
                for (auto tableName : {"Pairs",
                    "Pair1Stocks", "Pair2Stocks", "PairPrices", "Trades"
                })
                {
                    string sql_Droptable = string("DROP TABLE IF EXISTS ") + string(tableName);
                    if (DropTable(db, sql_Droptable.c_str()) == -1)
                        return -1;
                    cout << "Drop table " << tableName << endl;
                }
                
                vector<string> sql_Createtables(5, "");
                sql_Createtables[0] = string(
                    "CREATE TABLE IF NOT EXISTS Pairs ("
                    "id INT NOT NULL,"
                    "symbol1 CHAR(20) NOT NULL,"
                    "symbol2 CHAR(20) NOT NULL,"
                    "bolatility FLOAT,"
                    "profit_loss FLOAT,"
                    "PRIMARY KEY(symbol1, symbol2)"
                    ");");
                
                sql_Createtables[1] = string(
                    "CREATE TABLE IF NOT EXISTS Pair1Stocks ("
                    "symbol CHAR(20) NOT NULL, "
                    "date CHAR(20) NOT NULL,"
                    "open REAL NOT NULL,"
                    "high REAL NOT NULL,"
                    "low REAL NOT NULL,"
                    "close REAL NOT NULL,"
                    "adjusted_close REAL NOT NULL,"
                    "volume INT NOT NULL,"
                    "PRIMARY KEY (Symbol, Date)"
                    ");");
                
                sql_Createtables[2] = string(
                    "CREATE TABLE IF NOT EXISTS Pair2Stocks ("
                    "symbol CHAR(20) NOT NULL, "
                    "date CHAR(20) NOT NULL,"
                    "open REAL NOT NULL,"
                    "high REAL NOT NULL,"
                    "low REAL NOT NULL,"
                    "close REAL NOT NULL,"
                    "adjusted_close REAL NOT NULL,"
                    "volume INT NOT NULL,"
                    "PRIMARY KEY (Symbol, Date)"
                    ");");
            
                sql_Createtables[3] = string(
                    "CREATE TABLE IF NOT EXISTS PairPrices ("
                    "symbol1 CHAR(20) NOT NULL, "
                    "symbol2 CHAR(20) NOT NULL, "
                    "date CHAR(20) NOT NULL,"
                    "open1 REAL NOT NULL,"
                    "close1 REAL NOT NULL,"
                    "open2 REAL NOT NULL,"
                    "close2 REAL NOT NULL,"
                    "profit_loss REAL,"
                    "PRIMARY KEY(symbol1, symbol2, date)"
                    ");");
                
                sql_Createtables[4] = string(
                    "CREATE TABLE IF NOT EXISTS Trades ("
                    "symbol1 CHAR(20) NOT NULL, "
                    "symbol2 CHAR(20) NOT NULL, "
                    "date CHAR(20) NOT NULL,"
                    "Profit_loss REAL,"
                    "PRIMARY KEY(symbol1, symbol2, date)"
                    ");");
                
                for (int i = 0; i < sql_Createtables.size(); i++)
                {
                    if (ExecuteSQL(db, sql_Createtables[i].c_str()) == -1)
                        return -1;
                    else
                        cout << "Finished Creating table " << i << endl;
                }
                break;
            }
                
            case 'B':
            {
                // retrieve for each stock
                
                string daily_url_request;
                for (auto& symbol : symbols)
                {
                    string daily_read_buffer;
                    cout << "symbol: " << symbol << endl;
                    daily_url_request = daily_url_common + symbol + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d&fmt=json";
                
                    PullMarketData(daily_url_request, daily_read_buffer);

                    Stock stock(symbol, {});
                    PopulateDailyTrades(daily_read_buffer, stock);
                    stockMap[symbol] = stock;
                }
                cout << "Finished populating stock and intraday data" << endl;
                
                
                
                break;
                
            }
            
            case 'X':
            {
                
                return 0;
            }
        }
    
    }
    return 0;
}
