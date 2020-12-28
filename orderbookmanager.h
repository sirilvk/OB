#pragma once

#include "orderbook.h"
#include <map>
#include <exception>

namespace ACTION
{
    const char NEW = 'N';
    const char MODIFY = 'M';
    const char REMOVE = 'R';
    const char TRADE = 'X';
}

class OrderBookManager
{
public:
    OrderBookManager() = default;

    // do not copy
    OrderBookManager(const OrderBookManager&) = delete;
    OrderBookManager& operator=(const OrderBookManager&) = delete;

    void action(const char action, int productId, int orderId, char side, int quantity, double price);
    void action(const std::string& msg);
    void printOB(const int productId = 0);
    void printExceptions();

private:
    void sanitizeInputs(int productId, int orderId, char side, int quantity, double price);
    void sanitizeInputs(int orderId, char side, int quantity, double price);
    void sanitizeInputs(int productId, int quantity, double price);

    typedef std::map<int, OrderBook> ProductIdToOrderBookMap;

    // not required if all the message types (amend & cancels come with the productId)
    typedef std::map<int, ProductIdToOrderBookMap::iterator> OrderIdToOrderBookMap;

    ProductIdToOrderBookMap prdIdToOrderBook_;
    OrderIdToOrderBookMap ordIdToOrderBook_;

    struct Exceptions
    {
        std::string msg_;
        int id_;
        Exceptions(const std::string& msg, const int id) :msg_(msg), id_(id) {}
    };

    std::vector<Exceptions> exceptions_;
};
