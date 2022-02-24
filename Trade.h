//
//  Trade.hpp
//  PairTrading
//
//  Created by 杜学渊 on 2/19/22.
//

#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <map>
using namespace std;

class TradeData
{
private:
    string sDate;
    double dOpen;
    double dHigh;
    double dLow;
    double dClose;
    double dAdjClose;
    long lVolume;
public:
    TradeData(): sDate(""), dOpen(0), dHigh(0), dLow(0), dClose(0), dAdjClose(0), lVolume(0){}
    TradeData(string sDate_, double dOpen_, double dHigh_, double dLow_, double dClose_, double dAdjClose_, long lVolume_): sDate(sDate_), dOpen(dOpen_), dHigh(dHigh_), dLow(dLow_), dClose(dClose_), dAdjClose(dAdjClose_), lVolume(lVolume_) {}
    TradeData(const TradeData& TradeData): sDate(TradeData.sDate), dOpen(TradeData.dOpen), dHigh(TradeData.dHigh), dLow(TradeData.dLow), dClose(TradeData.dClose), dAdjClose(TradeData.dAdjClose), lVolume(TradeData.lVolume) {}
    ~TradeData() {};

    TradeData operator=(const TradeData & TradeData)
    {
        sDate = TradeData.sDate;
        dOpen = TradeData.dOpen;
        dHigh = TradeData.dHigh;
        dLow = TradeData.dLow;
        dClose = TradeData.dClose;
        dAdjClose = TradeData.dAdjClose;
        lVolume = TradeData.lVolume;
        return *this;
    }

    string GetsDate() const { return sDate; }
    double GetdOpen() const { return dOpen; }
    double GetdHigh() const { return dHigh; }
    double GetdLow() const { return dLow; }
    double GetdClose() const { return dClose; }
    double GetdAdjClose() const { return dAdjClose; }
    long GetlVolumn() const { return lVolume; }
};


class Stock
{
private:
    string sSymbol;
    vector<TradeData> trades;
    
public:
    Stock(): sSymbol("") {}
    Stock(string sSymbol_, const vector<TradeData> trades_): sSymbol(sSymbol_), trades(trades_) {}
    Stock(const Stock & stock): sSymbol(stock.sSymbol), trades(stock.trades) {}
    Stock operator=(const Stock & stock)
    {
        sSymbol = stock.sSymbol;
        trades = stock.trades;
        return *this;
    }
    
    void addTrade(const TradeData & trade) { trades.push_back(trade); }
    string getSymbol() { return sSymbol; }
    const vector<TradeData> & getTrades() const { return trades; }
    friend ostream & operator << (ostream & ostr, const Stock & stock)
    {
        ostr << "Symbol: " << stock.sSymbol << endl;
        for (vector<TradeData>::const_iterator itr = stock.trades.begin(); itr != stock.trades.end(); itr++)
        {
            ostr << &*itr;
        }
        return ostr;
    }
};


struct PairPrice
{
    double dOpen1;
    double dClose1;
    double dOpen2;
    double dClose2;
    double dProfit_Loss;
    PairPrice() : dOpen1(0), dClose1(0), dOpen2(0), dClose2(0), dProfit_Loss(0) {}
    PairPrice(double dOpen1_, double dClose1_, double dOpen2_, double dClose2_) : dOpen1(dOpen1_), dClose1(dClose1_), dOpen2(dOpen2_), dClose2(dClose2_), dProfit_Loss(0) {}
};


class StockPairPrices
{
private:
    pair<string, string> stockPair; //pair symbols
    double volatility;
    double k;
    map<string, PairPrice> dailyPairPrices; //<date, pair price>
public:
    StockPairPrices() { volatility = 0; k = 0;}
    StockPairPrices(pair<string, string> stockPair_) { stockPair = stockPair_; volatility = 0; k = 0; }
    void SetDailyPairPrice(string sDate_, PairPrice pairPrice_)
    {
        dailyPairPrices.insert(pair<string, PairPrice>(sDate_, pairPrice_));
    }
    void SetVolatility(double volatility_) { volatility = volatility_; }
    void SetK(double k_) { k = k_; }
    void UpdateProfitLoss(string sDate_, double dProfitLoss_)
    {
        dailyPairPrices[sDate_].dProfit_Loss = dProfitLoss_;
    }
    pair<string, string> GetStockPair() const { return stockPair; }
    map<string, PairPrice> GetDailyPrices() const { return dailyPairPrices; }
    double GetVolatility() const { return volatility; }
    double GetK() const { return k; }
};



//
//class Trade
//{
//protected:
//    float open;
//    float high;
//    float low;
//    float close;
//    int volume;
//public:
//    Trade(float open_, float high_, float low_, float close_, int volume_) :
//    open(open_), high(high_), low(low_), close(close_), volume(volume_) {}
//    ~Trade() {}
//    float GetOpen() const {  return open; }
//    float GetHigh() const   {  return high; }
//    float GetLow() const   {  return low;  }
//    float GetClose() const {  return close; }
//    int GetVolume() const {  return volume; }
//};
//
//class DailyTrade : public Trade
//{
//    private:
//    string date;
//    float adjusted_close;
//public:
//    DailyTrade(string date_, float open_, float high_, float low_, float close_, float adjusted_close_, int volume_) :
//    Trade(open_, high_, low_, close_, volume_), date(date_), adjusted_close(adjusted_close_) {}
//    ~DailyTrade() {}
//    string GetDate() const {  return date; }
//    float GetAdjustedClose() const {  return close;  }
//    friend ostream& operator << (ostream& out, const DailyTrade& t)
//    {
//    out << "Date: " << t.date << " Open: " << t.open << " High: " << t.high << " Low: " << t.low
//    << " Close: " << t.close << " Adjusted_Close: " << t.adjusted_close << " Volume: " << t.volume << endl;
//    return out;
//    }
//};
//
//
//class IntradayTrade : public Trade
//{
//private:
//    string date;
//    string timestamp;
//public:
//    IntradayTrade(string date_, string timestamp_, float open_, float high_, float low_, float close_, int volume_) :
//    Trade(open_, high_, low_, close_, volume_), date(date_), timestamp(timestamp_) {}
//    ~IntradayTrade() {}
//    string GetDate() const {  return date; }
//    string GetTimestamp() const {  return timestamp; }
//    friend ostream& operator << (ostream& out, const IntradayTrade& t)
//    {
//    out << " Date: " << t.date << " Timestamp: " << t.timestamp << " Open: " << t.open
//    << " High: " << t.high << " Low: " << t.low << " Close: " << t.close
//    << " Volume: " << t.volume << endl;
//    return out;
//    }
//};
//
//
//class Stock
//{
//private:
//    string symbol;
//    vector<DailyTrade> dailyTrades;
//    vector<IntradayTrade> intradayTrades;
//public:
//    Stock() :symbol("")    {     dailyTrades.clear(); intradayTrades.clear();     }
//    Stock(string symbol_) :symbol(symbol_) {     dailyTrades.clear(); intradayTrades.clear();     }
//    Stock(const Stock& stock)  {    memcpy(this, &stock, sizeof(stock));     }
//    ~Stock() {}
//    void addDailyTrade(const DailyTrade& aTrade)     {     dailyTrades.push_back(aTrade);    }
//    void addIntradayTrade(const IntradayTrade& aTrade)    {    intradayTrades.push_back(aTrade);    }
//    string GetSymbol() const {     return symbol;     }
//    const vector<DailyTrade>& GetDailyTrade(void) const {     return dailyTrades;     }
//    const vector<IntradayTrade>& GetIntradayTrade(void) const {         return intradayTrades;         }
//    friend ostream& operator << (ostream& out, const Stock& s)
//    {
//        out << "Symbol: " << s.symbol << endl;
//        for (vector<DailyTrade>::const_iterator itr = s.dailyTrades.begin(); itr != s.dailyTrades.end(); itr++)
//            out << *itr;
//        for (vector<IntradayTrade>::const_iterator itr = s.intradayTrades.begin(); itr != s.intradayTrades.end(); itr++)
//            out << *itr;
//        return out;
//    }
//};
