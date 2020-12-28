#pragma once

#include <memory>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <set>

namespace SIDE
{
	const char BUY = 'B';
	const char SELL = 'S';
}

struct Order
{
	int id_;
	char side_;
	double price_;
	int quantity_;

	Order(int id, char side, double price, int quantity) :id_(id), side_(side), price_(price), quantity_(quantity)
	{
	}
};

typedef std::shared_ptr<Order> OrderPtr;

// maintain an orderlist based on the price it holds.
struct OrderList
{
    double price_;
    int totalQty_;
    std::list<OrderPtr> orderList_;

    OrderList(OrderPtr& orderPtr) :price_(orderPtr->price_), totalQty_(orderPtr->quantity_)
    {
        orderList_.push_back(orderPtr);
    }
};

typedef std::shared_ptr<OrderList> OrderListPtr;

/*
 * @brief : OrderBook is the class to maintain and manage the orders
 * for a particular instrument.
 */
class OrderBook
{
private:
    const int productId_;

    struct AscendCompare
    {
        bool operator() (const OrderListPtr& OLP1, const OrderListPtr& OLP2) const
        {
            return comparator(OLP1->price_, OLP2->price_);
        }

        std::greater<double> comparator;
    };

    struct DescendCompare
    {
        bool operator() (const OrderListPtr& OLP1, const OrderListPtr& OLP2) const
        {
            return comparator(OLP1->price_, OLP2->price_);
        }

        std::less<double> comparator;
    };

    // store id to orderPtr hash map for constant time lookup of order based on id's
    typedef std::unordered_map<int, OrderPtr> OrderIdHashMap;

    // have a set to maintain distinct sorted price
    typedef std::set<OrderListPtr, AscendCompare> AscendOrderSet;
    typedef std::set<OrderListPtr, DescendCompare> DescendOrderSet;

    // store price to orderlist hash map for constant time lookup for existing price
    typedef std::unordered_map<double, OrderListPtr> OrderListHashMap;

    AscendOrderSet bidSet_; // will be a max set to keep the best price on top
    DescendOrderSet offerSet_; // will be a min set to keep the best ask price on top
    OrderListHashMap bidOrderHashMap_;
    OrderListHashMap offerOrderHashMap_;
    OrderIdHashMap orderIdHashMap_;

    int lastTradedQuantity_ = 0;
    double lastTradedPrice_ = 0.0;

    // do not copy
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    void addOrUpdateSet(OrderPtr& orderPtr, char side);
    bool deleteFromSet(OrderPtr& orderPtr, char side);
    void generateFills(const std::vector<int>& totalFills, const std::map<int, int>& partialFills);
    bool checkIfValidTradeAndUpdateOrderBook(const double price, const int quantity);

public:
    explicit OrderBook(const int productId):productId_(productId) {}

    void getLastTradeDetails(double& price, int& quantity) const;

    ~OrderBook() {}

    void enterOrder(int id, char side, double price, int quantity);
    bool getOrderFromId(int id, Order& record);
    bool modifyOrder(int id, int quantity);
    bool deleteOrder(int id);
    bool handleTrade(double price, int quantity);
    void printOrderBook() const;
    int getProductId() const { return productId_; }


};
