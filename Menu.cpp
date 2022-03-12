//
//  main.cpp
//  PairTrading
//
//  Created by 杜学渊 on 2/19/22.
//  Modified by Hubert on 3/2/22
//  Modified by Qinyan on 3/5/22
//  Modified by Nicole
//

#include <iostream>
#include <iomanip>
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

int dropAllTables(sqlite3* db) {

	for (auto tableName : { "StockPairs",
		"Pair1Stocks", "Pair2Stocks", "PairPrices", "Trades"
		})
	{
		string sql_Droptable = string("DROP TABLE IF EXISTS ") + string(tableName);
		if (DropTable(db, sql_Droptable.c_str()) == -1)
			return -1;
		cout << "Drop table " << tableName << endl;
	}
	return 0;
}

int main(int argc, const char* argv[]) {
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
	vector<pair<string, string>>allpairs;
	//vector< StockPairPrices>allpairprices;

	for (auto it = stockPairs.begin(); it != stockPairs.end(); it++)
	{
		symbols1.insert(it->first);
		//cout << it->first << endl;
		symbols2.insert(	it->second);
		//cout << it->second << endl;
		symbols.insert(it->first);
		symbols.insert(it->second);
		allpairs.push_back(*it);
		//allpairprices.push_back(StockPairPrices(*it));
		
	}
    vector<string> symbols_vec(symbols.begin(), symbols.end());
	

	map<string, Stock> stockMap; // 存储每个股票每天的价格
	map<pair<string, string>, StockPairPrices> pairPriceMap; // 存储每个 pair 对应的价格

	string sConfigFile = "config.csv";
	map<string, string> config_map = ProcessConfigData(sConfigFile);

	string daily_url_common = config_map["daily_url_common"];
	string start_date = config_map["start_date"];
	string end_date = config_map["end_date"];
	string api_token = config_map["api_token"];

    string back_test_start_date = "2021-12-31";
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
			if (dropAllTables(db) == -1)
				return -1;

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
				"adjusted_close1 REAL NOT NULL,"
				"open2 REAL NOT NULL,"
				"close2 REAL NOT NULL,"
				"adjusted_close2 REAL NOT NULL,"
				"profit_loss REAL," //this could be in Trades--WT
				"PRIMARY KEY(symbol1, symbol2, date),"
                "FOREIGN KEY(symbol1, date) References Pair1Stocks(symbol, date),"
                "FOREIGN KEY(symbol2, date) References Pair2Stocks(symbol, date),"
                "FOREIGN KEY(symbol1, symbol2) References StockPairs(symbol1, symbol2)"
				");");
			//We can leave it and create this table later by selecting from Pair1Stocks and Pair2Stocks.--WT

			sql_Createtables[4] = string(
				"CREATE TABLE IF NOT EXISTS Trades ("
				"symbol1 CHAR(20) NOT NULL, "
				"symbol2 CHAR(20) NOT NULL, "
				"date CHAR(20) NOT NULL,"
				"Profit_loss REAL,"
				"PRIMARY KEY(symbol1, symbol2, date),"
                "FOREIGN KEY(symbol1, symbol2) References PairPrices(symbol1, symbol2)"
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
            
            int Num_threads = 5;
            vector<string> read_buffers;
            vector<string> url_requests;
//
            
            
			//for (auto& symbol : symbols_vec)
			//{
   //             daily_url_request = daily_url_common + symbol + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d&fmt=json";
   //             url_requests.push_back(daily_url_request);
			//	string daily_read_buffer;
			//	cout << "symbol: " << symbol << endl;
			//	daily_url_request = daily_url_common + symbol + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d&fmt=json";

			//	PullMarketData(daily_url_request, daily_read_buffer);

			//	Stock stock(symbol, {});
			//	PopulateDailyTrades(daily_read_buffer, stock);
			//	stockMap[symbol] = stock;
			//}
            
            // Multi thread but has problems
			for (auto& symbol : symbols_vec)
			{
				daily_url_request = daily_url_common + symbol + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d&fmt=json";
				url_requests.push_back(daily_url_request);
			}
			PullMarketDataMultiThread(url_requests, read_buffers, Num_threads);
            for (int i = 0; i < symbols_vec.size(); i++)
            {
                Stock stock(symbols_vec[i], {});
                PopulateDailyTrades(read_buffers[i], stock);
                stockMap[symbols_vec[i]] = stock;
            }
			
			cout << "Finished populating stock data" << endl;
			//getpairprice(stockMap, allpairs, allpairprices);
	
			break;

		}

		case 'C':
		{
			// "C - Create PairPrices Table\n"


			// Populate stockPairPrices
			int tableID = 0;
			for (auto& symbols_ : { symbols1, symbols2 })
			{
				tableID++;
				for (auto& symbol : symbols_)
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
				+ "Pair1Stocks.close as close1, Pair1Stocks.adjusted_close as adjusted_close1, Pair2Stocks.open as open2, "
				+ "Pair2Stocks.close as close2, Pair2Stocks.adjusted_close as adjusted_close2, 0 as profit_loss "
				+ "From StockPairs, Pair1Stocks, Pair2Stocks "
				+ "Where (((StockPairs.symbol1 = Pair1Stocks.symbol) and (StockPairs.symbol2 = Pair2Stocks.symbol)) and (Pair1Stocks.date = Pair2Stocks.date)) "
				+ "ORDER BY symbol1, symbol2;";
			cout << "insert statement: " << insert_sql << endl;
			if (ExecuteSQL(db, insert_sql.c_str()) == -1)
				return -1;
			cout << "Finish insert PairPrices" << endl;

			
			string calculate_volatility_for_pair = string("Update StockPairs SET volatility =")
				+ "(SELECT(AVG((adjusted_close1/adjusted_close2)*(adjusted_close1/adjusted_close2)) - AVG(adjusted_close1/adjusted_close2)*AVG(adjusted_close1/adjusted_close2)) as variance "
				+ "FROM PairPrices "
				+ "WHERE StockPairs.symbol1 = PairPrices.symbol1 AND StockPairs.symbol2 = PairPrices.symbol2 AND PairPrices.date <= \'"
				+ back_test_start_date + "\');";

			cout << "calculate volatility statement: " << calculate_volatility_for_pair << endl;
			if (ExecuteSQL(db, calculate_volatility_for_pair.c_str()) == -1)
				return -1;
			cout << "Finish insert volatility" << endl;
            break;

		}
		case 'E':
		{
			// "E - Back Test\n"
			double k = ReadDouble("Enter a valid double as parameter k: ");
			double k_square = k * k;
			cout << "Your parameter k is: " << fixed << setprecision(2) << k << '\n' << "Backtesting..." << endl;
			// Calculate P/L of Long (Short will be negative of that) 
			string calculate_daily_long = string("UPDATE PairPrices ")
				+ "SET profit_loss = \n"
				+ " 10000 * (close1 - open1) - CAST(10000 * open1 / open2 AS int) * (close2 - open2) "//N2= N1 * (Open1d2/Open2d2),
				+ " WHERE date > \'" + back_test_start_date + "\' ";

			// Convert to Short or Long based on Condition
			string convert_daily_pl = string("WITH cte AS ( \n")
				+ "SELECT symbol1, date, close2, "
				+ "LAG(close1, 1, 0) OVER (PARTITION BY symbol1 ORDER BY date) / LAG(close2, 1, 0) OVER (PARTITION BY symbol1 ORDER BY date) AS prev_frac "
				+ "FROM PairPrices "//LAG(待查询的参数列名,向上偏移的位数,超出最上面边界的默认值) OVER:分组依据
				//Close1d1 and Close2d1 are the closing prices on day d – 1 for stocks 1 and 2
				+ ") "				
				+ "UPDATE PairPrices AS p "
				+ "SET profit_loss = (CASE "
				+ "WHEN ("
				+ "( (SELECT c.prev_frac FROM cte AS c WHERE (c.symbol1, c.date) = (p.symbol1, p.date)) - (open1 / open2) )"
				+ "* ( (SELECT c.prev_frac FROM cte AS c WHERE (c.symbol1, c.date) = (p.symbol1, p.date)) - (open1 / open2) )"
				+ " > " + to_string(k_square) + " * ( SELECT volatility FROM StockPairs AS s WHERE s.symbol1 = p.symbol1) "
				+ ") THEN - profit_loss "
				+ "ELSE profit_loss "
				+ "END) "
				+ " WHERE date > \'" + back_test_start_date + "\' ";

			// Sum up to get final P/L and update in StockPairs
			string calculate_sum_pl = string("Update StockPairs SET profit_loss = ")
				+ "(SELECT SUM (PairPrices.profit_loss) "
				+ "FROM PairPrices "
				+ "WHERE (StockPairs.symbol1, StockPairs.symbol2) = (PairPrices.symbol1, PairPrices.symbol2) AND PairPrices.date > \'"
				+ back_test_start_date + "\');";

			if (ExecuteSQL(db, calculate_daily_long.c_str()) == -1)
				return -1;
			if (ExecuteSQL(db, convert_daily_pl.c_str()) == -1)
				return -1;
			if (ExecuteSQL(db, calculate_sum_pl.c_str()) == -1)
				return -1;
			cout << "Back Test Finished" << endl;

			break;
		}
		case 'F':
		{
			// "F - Calculate Profit and Loss For Each Pair\n"

			string delete_existing_data = string("DELETE FROM Trades;");

			string insert_Trades_sql = string("INSERT INTO Trades ") +
				"SELECT symbol1, symbol2, date, profit_loss "
				+ "FROM PairPrices "
				+ "WHERE profit_loss <> 0 "
				+ "ORDER BY symbol1, symbol2;";

			if (ExecuteSQL(db, delete_existing_data.c_str()) == -1) {
				cout << "Errors when deleting data from Trades" << endl;
				return -1;
			}

			if (ExecuteSQL(db, insert_Trades_sql.c_str()) == -1) {
				cout << "Errors when inserting into Trades" << endl;
				return -1;
			}
			cout << "Successfully inserted pnl for each pair into Trades Table" << endl;


			break;
		}
		case 'G':
		{	cout << "Show stock pairs" << endl;
			string select_pairs = "SELECT id,symbol1,symbol2 FROM StockPairs";
			if (ShowTable(db, select_pairs.c_str()) == -1)
				return -1;
			cout << "********************************************************" << endl;
			cout<<"Choose a pair of stock, please enter the id of the pair" << endl;
			int id_of_pair;
			cin >> id_of_pair;
			if (id_of_pair > stockPairs.size()&& id_of_pair<0) {
				cout << "Pair id out of range,please enter a valid id" << endl;
				return -1;
			}
			cout << "Your choice:" << endl;
			string stock1 = allpairs[id_of_pair - 1].first;
			string stock2 = allpairs[id_of_pair - 1].second;
			cout << stock1<< "," <<stock2 << endl;
			//Get volatility from StockPairs
			string selectvol = string("SELECT volatility FROM StockPairs WHERE symbol1=\'") + stock1 + "\'AND symbol2 =\'" + stock2 + "\';";
			int rc = 0;
			char* error = nullptr;
			char** results = NULL;
			int rows, columns;
			double vol = 0;
			rc = sqlite3_get_table(db, selectvol.c_str(), &results, &rows, &columns, &error);
			if (rc)
			{
				cerr << "Error executing SQLite3 query: " << sqlite3_errmsg(db) << std::endl << std::endl;
				sqlite3_free(error);
				return -1;
			}
			else {
				vol = stod(results[1]);
			}
			
			cout <<"Volatility of the pair: "<<vol << endl;
			double close1d1, close2d1, open1d2, open2d2, close1d2,close2d2, pnl, k;
			const double N1 = 10000;
			int longstate = 0;//longstate=1: Long, longstate=-1,short
			
			cout << "Please choose close prices for the first stock of day 1" << endl;
			cin >> close1d1;
			cout<< "Please choose close prices for the second stock of day 1" << endl;
			cin >> close2d1;
		
			cout << "Please choose open prices for the first stock of day 2" << endl;
			cin >> open1d2;
			cout << "Please choose open prices for the second stock of day 2" << endl;
			cin >> open2d2;
			cout << "Please choose close prices for the first stock of day 2" << endl;
			cin >> close1d2;
			cout << "Please choose close prices for the second stock of day 2" << endl;
			cin >> close2d2;
			cout << "Choose your k" << endl;
			cin >> k;
			double N2 = N1 * open1d2 / open2d2;
			if (abs(close1d1 / close2d1 - open1d2 / open2d2) > (vol * k))
			{
				longstate = -1;
				cout << stock1 << " is short for "<<N1<<" shares and " << stock2 << " is long for "<<N2<<" shares";
			}
			else 
			{
				longstate = 1;
				cout << stock1 << " is long for " << N1 << " shares and " << stock2 << " is short for " << N2 << " shares";
				//cout << stock1 << " is long and " << stock2 << " is short" << endl;
			}
			
			
			pnl = (-longstate * (open1d2 - close1d2) * N1) + (longstate * N2 * (open2d2 - close2d2));
			// "G - Manual Testing\n"
			cout << "Profit and Loss is " << pnl << endl;
			break;
		}
		case 'H':
		{
			// "H - Drop All the Tables\n"
			if (dropAllTables(db) == -1)
				return -1;


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

