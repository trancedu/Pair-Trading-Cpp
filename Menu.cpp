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
#include <set>

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
//    stockPairs["BAC"] = "HPQ";
    stockPairs = GetPairs("PairTrading.txt");
    
    set<string> symbols1;
    set<string> symbols2;
    set<string> symbols;
    for (auto it = stockPairs.begin(); it != stockPairs.end(); it++)
    {
        symbols1.insert(it->first);
        symbols2.insert(it->second);
        symbols.insert(it->first);
        symbols.insert(it->second);
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
                break;

            }
                
            case 'C':
            {
                // "C - Create PairPrices Table\n"
                
                
                // Populate stockPairPrices
                int tableID = 0;
                for (auto &symbols_ : {symbols1, symbols2})
                {
                    tableID++;
                    for (auto &symbol : symbols_)
                    {
                        cout << symbol << endl;
                        vector<TradeData> trades = stockMap[symbol].getTrades();
                        stringstream sql_stmt;
                        sql_stmt << "INSERT INTO Pair" << tableID << "Stocks Values ";
                        for (vector<TradeData>::const_iterator itr = trades.begin(); itr != trades.end(); itr++)
                        {
                            sql_stmt << "( '" << symbol << "', '" << (*itr).GetsDate() << "', " << (*itr).GetdOpen() << ", " << (*itr).GetdHigh() \
                            << ", " << (*itr).GetdLow() << ", " << (*itr).GetdClose() << ", " << (*itr).GetdAdjClose() << ", " << (*itr).GetlVolumn() << " ),";
                        }
                        string statement = sql_stmt.str();
                        statement = statement.substr(0, statement.size() - 1); // delete the last ","
                        statement += ";";
                        int rc = ExecuteSQL(db, statement.c_str());
                        if (rc == -1) cout << "Error populating Pair1Stocks" << endl;
                    }
                }

                cout << "Successfully Inserted into Pair1Stocks" << endl;
                cout << "Successfully Inserted into Pair2Stocks" << endl;


                // "C - Create PairPrices Table\n"
                stringstream sql_Insert;
                int id = 1;
                
                sql_Insert << "INSERT INTO StockPairs VALUES ";
                for (auto it = stockPairs.begin(); it != stockPairs.end(); it++)
                {
                    sql_Insert << "(" << id << ",'" << it->first << "','" << it->second.c_str() << "'," << 0.0 << "," << 0.0 << ")";
                    if (id != stockPairs.size()) {
                        sql_Insert << ",";
                    }
                    else sql_Insert << ";";
                    id++;
                }
                if (ExecuteSQL(db, sql_Insert.str().c_str()) == -1)
                    return -1;
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

