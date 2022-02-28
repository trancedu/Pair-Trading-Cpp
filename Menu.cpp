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

#include "Trade.h"
#include "Database.h"

#include "json/json.h"
#include "curl/curl.h"

#include "Util.h"
#include "GetMarketData.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    
    string database_name = "PairTrading.db";
    cout << "Opening Database..." << endl;
    sqlite3* db = NULL;
    if (OpenDatabase(database_name.c_str(), db) != 0)      return -1;

    
    map<string, string> stockPairs;
//    stockPairs["AAPL"] = "HPQ";
//    stockPairs["AXP"] = "COF";
//    stockPairs["BAC"] = "JPM";
    stockPairs = GetPairs("PairTrading.txt");
    
    vector<string> symbols1;
    vector<string> symbols2;
    vector<string> symbols;
    for (auto it = stockPairs.begin(); it != stockPairs.end(); it++)
    {
        symbols1.push_back(it->first);
        symbols2.push_back(it->second);
        symbols.push_back(it->first);
        symbols.push_back(it->second);
    }
    map<string, Stock> stockMap; // 存储每个股票每天的价格
    map<pair<string, string>, StockPairPrices> pairPriceMap; // 存储每个 pair 对应的价格
    
    string sConfigFile = "config.csv";
    map<string, string> config_map = ProcessConfigData(sConfigFile);
    
    string daily_url_common = config_map["daily_url_common"];
    string start_date = config_map["start_date"];
    string end_date = config_map["end_date"];
    string api_token = config_map["api_token"];

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
                for (auto tableName : {"StockPairs",
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
                    "CREATE TABLE IF NOT EXISTS StockPairs ("
                    "id INT NOT NULL,"
                    "symbol1 CHAR(20) NOT NULL,"
                    "symbol2 CHAR(20) NOT NULL,"
                    "volatility FLOAT,"
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
                    "profit_loss REAL," //this could be in Trades--WT
                    "PRIMARY KEY(symbol1, symbol2, date)"
                    ");");
                //We can leave it and create this table later by selecting from Pair1Stocks and Pair2Stocks.--WT
                
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
                cout << "Finished populating stock data" << endl;
                

                // Populate stockPairPrices
                for (auto it = stockPairs.begin(); it != stockPairs.end(); it++)
                {
                    string symbol1, symbol2;
                    cout << symbol1 << " " << symbol2 << endl;
                    symbol1 = it->first;
                    symbol2 = it->second;
                    StockPairPrices stockPairPrices(make_pair(symbol1, symbol2));
                    vector<TradeData> trades1 = stockMap[symbol1].getTrades();
                    vector<TradeData> trades2 = stockMap[symbol2].getTrades();
                    
                    // 这边需要每一天两只股票的开盘价和收盘价,需要对每一天进行遍历
                    // 如何对日期遍历防止日期错开呢? 同向双指针了
//                    int i = 0, j = 0;
//                    while (i < trades1.size() && j < trades2.size())
//                    {
//                        string date1 = trades1[i].GetsDate();
//                        string date2 = trades2[j].GetsDate();
//
//                        if (date1 == date2)
//                        {
//                            PairPrice pairPrice(trades1[i].GetdOpen(), trades1[i].GetdClose(),
//                                                trades2[j].GetdOpen(), trades2[j].GetdClose());
//                            stockPairPrices.SetDailyPairPrice(date1, pairPrice);
//                            i++; j++;
//                        }
//                        else if (date1.compare(date2) > 0)
//                        {
//                            j++;
//                            cout << "Pair Date " << date1 << " " << date2 << endl;
//                            cout << "Day1 higher" << endl;
//                        }
//                        else {
//                            i++;
//                            cout << "Pair Date " << date1 << " " << date2 << endl;
//                            cout << "Day2 higher" << endl;
//                        }
//                    }
//                    pairPriceMap[make_pair(symbol1, symbol2)] = stockPairPrices;
                    char sql_Insert[512];
                    //WT
                    
                    stringstream sql_stmt;
                    sql_stmt << "INSERT INTO Pair1Stocks Values ";
                    for (vector<TradeData>::const_iterator itr = trades1.begin(); itr != trades1.end(); itr++)
                    {
                        sql_stmt << "( '" << symbol1 << "', '" << (*itr).GetsDate() << "', " << (*itr).GetdOpen() << ", " << (*itr).GetdHigh() \
                        << ", " << (*itr).GetdLow() << ", " << (*itr).GetdClose() << ", " << (*itr).GetdAdjClose() << ", " << (*itr).GetlVolumn() << " ),";
                    }
                    string statement = sql_stmt.str();
                    statement = statement.substr(0, statement.size() - 1); // delete the last ","
                    statement += ";";
        
                    int rc = ExecuteSQL(db, statement.c_str());
                    if (rc == -1) cout << "Error populating Pair1Stocks" << endl;

                    stringstream sql_stmt2;
                    sql_stmt2 << "INSERT INTO Pair2Stocks Values ";
                    for (vector<TradeData>::const_iterator itr = trades2.begin(); itr != trades2.end(); itr++)
                    {
                        sql_stmt2 << "( '" << symbol2 << "', '" << (*itr).GetsDate() << "', " << (*itr).GetdOpen() << ", " << (*itr).GetdHigh() \
                        << ", " << (*itr).GetdLow() << ", " << (*itr).GetdClose() << ", " << (*itr).GetdAdjClose() << ", " << (*itr).GetlVolumn() << " ),";
                    }
                    statement = sql_stmt2.str();
                    statement = statement.substr(0, statement.size()-1); // delete the last ","
                    statement += ";";
                    
                    rc = ExecuteSQL(db, statement.c_str());
                    if (rc == -1) cout << "Error populating Pair2Stocks" << endl;
                }
                cout << "Successfully Inserted into Pair1Stocks" << endl;
                cout << "Successfully Inserted into Pair2Stocks" << endl;
                break;
                
            }
                
            case 'C':
            {
                // "C - Create PairPrices Table\n"
                char sql_Insert[512];
                int id = 1;
                for (auto it = pairPriceMap.begin(); it != pairPriceMap.end(); it++)
                {
                    sprintf(sql_Insert, "INSERT INTO StockPairs VALUES(%d, \"%s\", \"%s\", %f, %f)",
                            id, it->first.first.c_str(), it->first.second.c_str(), 0.0, 0.0);
                    cout << sql_Insert << endl;
                    if (ExecuteSQL(db, sql_Insert) == -1)
                        return -1;
                    id++;
                }
                cout << "Inserted into StockPairs" << endl;
                
                break;
            }
                
            case 'D':
            {
                // "D - Calculate Volatility\n"

                string insert_sql = string("Insert into PairPrices ") +
                    "Select StockPairs.symbol1 as symbol1, StockPairs.symbol2 as symbol2, "
                    + "Pair1Stocks.date as date, Pair1Stocks.open as open1, "
                    + "Pair1Stocks.close as close1, Pair2Stocks.open as open2, "
                    + "Pair2Stocks.close as close2, 0 as profit_loss "
                    + "From StockPairs, Pair1Stocks, Pair2Stocks "
                    + "Where (((StockPairs.symbol1 = Pair1Stocks.symbol) and (StockPairs.symbol2 = Pair2Stocks.symbol)) and (Pair1Stocks.date = Pair2Stocks.date)) "
                    + "ORDER BY symbol1, symbol2;";
                cout << "insert statement: " << insert_sql << endl;
                if (ExecuteSQL(db, insert_sql.c_str()) == -1)
                    return -1;
                cout << "Finish insert PairPrices" << endl;
                
                string back_test_start_date = "2021-12-31";
                string calculate_volatility_for_pair = string("Update StockPairs SET volatility =") 
                        + "(SELECT(AVG((close1/close2)*(close1/close2)) - AVG(close1/close2)*AVG(close1/close2)) as variance " 
                        + "FROM PairPrices " 
                        + "WHERE StockPairs.symbol1 = PairPrices.symbol1 AND StockPairs.symbol2 = PairPrices.symbol2 AND PairPrices.date <= \'" 
                        + back_test_start_date + "\');";

                cout << "calculate volatility statement: " << calculate_volatility_for_pair << endl;
                if (ExecuteSQL(db, calculate_volatility_for_pair.c_str()) == -1)
                    return -1;
                cout << "Finish insert volatility" << endl;
            }
            case 'E':
            {
                // "E - Back Test\n"
                break;
            }
            case 'F':
            {
                // "F - Calculate Profit and Loss For Each Pair\n"
                break;
            }
            case 'G':
            {
                // "G - Manual Testing\n"
                break;
            }
            case 'H':
            {
                // "H - Drop All the Tables\n"
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

