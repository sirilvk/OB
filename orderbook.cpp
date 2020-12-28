#include "orderbook.h"
#include <cmath>
#include <algorithm>

void OrderBook::enterOrder(int id, char side, double price, int quantity)
{
    // if id already exists throw error
    if (orderIdHashMap_.find(id) != orderIdHashMap_.end())
    {
        throw std::runtime_error("Order with Id already exists");
    }

    // else add the order to the hash map and also add and update the set based on the side (if valid side)
    switch (side)
    {
    case SIDE::BUY:
        orderIdHashMap_[id] = std::make_shared<Order>(id, side, price, quantity);
        addOrUpdateSet(orderIdHashMap_[id], SIDE::BUY);
        break;
    case SIDE::SELL:
        orderIdHashMap_[id] = std::make_shared<Order>(id, side, price, quantity);
        addOrUpdateSet(orderIdHashMap_[id], SIDE::SELL);
        break;
    default:
        throw std::runtime_error("Invalid SIDE received. Order Creation failed!!!");
    }
}


bool OrderBook::getOrderFromId(int id, Order& record)
{
    OrderIdHashMap::iterator iter = orderIdHashMap_.find(id);

    if (iter != orderIdHashMap_.end())
    {
        record = *iter->second.get();
        return true;
    }

    return false;
}

bool OrderBook::modifyOrder(int id, int quantity)
{
    // sanity check on quantity
    if (quantity <= 0)
    {
        std::cerr << "Invalid quantity sent. Order modification failed!!!" << std::endl;
        return false;
    }

    OrderIdHashMap::iterator iter = orderIdHashMap_.find(id);

    if (iter != orderIdHashMap_.end())
    {
        // now that we have the order, need to update the orderIdHashMap and also the orderListHashMap based on price
        int quantityDiff = quantity - iter->second->quantity_;
        iter->second->quantity_ = quantity;

        OrderListHashMap& orderListHashMap = (iter->second->side_ == SIDE::BUY) ? bidOrderHashMap_ : offerOrderHashMap_;
        // update the quantity diff on the OrderList total quantity
        orderListHashMap[iter->second->price_]->totalQty_ = orderListHashMap[iter->second->price_]->totalQty_ + quantityDiff;
        return true;
    }

    return false;
}

void OrderBook::printOrderBook() const
{
    std::cout << "Printing Bid OrderBook (till level 5)" << std::endl;
    int level = 5;
    for (AscendOrderSet::const_iterator iter = bidSet_.cbegin(); iter != bidSet_.cend(); ++iter)
    {
        std::cout << (*iter)->price_ << " : " << (*iter)->totalQty_ << std::endl;
        if (--level == 0)
            break;
    }

    level = 5;
    std::cout << "Printing Offer OrderBook (till level 5)" << std::endl;
    for (DescendOrderSet::const_iterator iter = offerSet_.cbegin(); iter != offerSet_.cend(); ++iter)
    {
        std::cout << (*iter)->price_ << " : " << (*iter)->totalQty_ << std::endl;
        if (--level == 0)
            break;
    }
}

bool OrderBook::deleteOrder(int id)
{
    OrderIdHashMap::iterator iter = orderIdHashMap_.find(id);

    if (iter != orderIdHashMap_.end())
    {
        // found the element. now delete from the set and then delete from this hash map
        auto status = deleteFromSet(iter->second, iter->second->side_);
        orderIdHashMap_.erase(iter);

        return status;
    }

    return false;
}

void OrderBook::generateFills(const std::vector<int>& totalFills, const std::map<int, int>& partialFills)
{
    for (const auto& it : totalFills)
    {
        std::cout << "Order id [" << it << "] totally filled!!" << std::endl;
        deleteOrder(it);
    }

    for (const auto& it : partialFills)
    {
        std::cout << "Order id [" << it.first << "] partially filled!! New Qty [" << it.second << "]" << std::endl;
        modifyOrder(it.first, it.second);
    }
}

bool OrderBook::checkIfValidTradeAndUpdateOrderBook(const double price, const int quantity)
{
    if (bidSet_.empty() || offerSet_.empty())
        throw std::runtime_error("Trade received on empty order books!!");

    // check if price is inline with the top of the orderbook
    auto bIt = bidSet_.begin();
    auto sIt = offerSet_.begin();

    if ((*bIt)->price_ < price || (*sIt)->price_ > price)
        throw std::runtime_error("Out of order Trade price received!!");

    // update the buy side quantity .. starting with greatest price
    std::vector<int> borderIdsToDelete, sorderIdsToDelete;
    std::map<int, int> borderIdsToModify, sorderIdsToModify;

    { // buy side
        int tradeQty = quantity;
        while (bIt != bidSet_.end())
        {
            if ((*bIt)->price_ < price) // buy level price less than trade price 
                break;

            if ((*bIt)->totalQty_ == tradeQty) // matched with first level quantity
            {
                for (const auto& it : (*bIt)->orderList_)
                    borderIdsToDelete.emplace_back(it->id_);
                tradeQty = 0;
                break;
            }
            else if ((*bIt)->totalQty_ > tradeQty)
            {
                int& qty = tradeQty;
                for (const auto& it : (*bIt)->orderList_)
                {
                    if (it->quantity_ == qty)
                    {
                        borderIdsToDelete.emplace_back(it->id_);
                        qty = 0;
                        break;
                    }
                    else if (it->quantity_ > qty)
                    {
                        borderIdsToModify[it->id_] = it->quantity_ - qty;
                        qty = 0;
                        break;

                    }
                    else
                    {
                        borderIdsToDelete.emplace_back(it->id_);
                        qty -= it->quantity_;
                    }
                }
                break;
            }
            else // tradeQty > current level quantity
            {
                for (const auto& it : (*bIt)->orderList_)
                    borderIdsToDelete.emplace_back(it->id_);
                tradeQty -= (*bIt)->totalQty_;
                bIt++;
            }
        }

        if (tradeQty > 0)
            throw std::runtime_error("Insufficient quantity to fill from Buy side OrderBook!!");
    }

    { // sell side
        int tradeQty = quantity;
        while (sIt != offerSet_.end())
        {
            if ((*sIt)->price_ > price) // sell level price greater than trade price 
                break;

            if ((*sIt)->totalQty_ == tradeQty) // matched with first level quantity
            {
                for (const auto& it : (*sIt)->orderList_)
                    sorderIdsToDelete.emplace_back(it->id_);
                tradeQty = 0;
                break;
            }
            else if ((*sIt)->totalQty_ > tradeQty)
            {
                int& qty = tradeQty;
                for (const auto& it : (*sIt)->orderList_)
                {
                    if (it->quantity_ == qty)
                    {
                        sorderIdsToDelete.emplace_back(it->id_);
                        qty = 0;
                        break;
                    }
                    else if (it->quantity_ > qty)
                    {
                        sorderIdsToModify[it->id_] = it->quantity_ - qty;
                        qty = 0;
                        break;

                    }
                    else
                    {
                        sorderIdsToDelete.emplace_back(it->id_);
                        qty -= it->quantity_;
                    }
                }
                break;
            }
            else // tradeQty > current level quantity
            {
                for (const auto& it : (*sIt)->orderList_)
                    sorderIdsToDelete.emplace_back(it->id_);
                tradeQty -= (*sIt)->totalQty_;
                sIt++;
            }
        }

        if (tradeQty > 0)
            throw std::runtime_error("Insufficient quantity to fill from Sell side OrderBook!!");
    }

    // now update the order books
    generateFills(borderIdsToDelete, borderIdsToModify);
    generateFills(sorderIdsToDelete, sorderIdsToModify);

    // direct order book update without fills
    //std::for_each(borderIdsToDelete.begin(), borderIdsToDelete.end(), [this](int id) { deleteOrder(id);  });
    //std::for_each(sorderIdsToDelete.begin(), sorderIdsToDelete.end(), [this](int id) { deleteOrder(id);  });
    //std::for_each(borderIdsToModify.begin(), borderIdsToModify.end(), [this](std::pair<const int, int>& it) { modifyOrder(it.first, it.second); });
    //std::for_each(sorderIdsToModify.begin(), sorderIdsToModify.end(), [this](std::pair<const int, int>& it) { modifyOrder(it.first, it.second); });

    return true;
}

bool OrderBook::handleTrade(double price, int quantity)
{
    // update the order books
    const bool status = checkIfValidTradeAndUpdateOrderBook(price, quantity);

    if (status)
    {
        if (fabs(lastTradedPrice_ - price) < std::numeric_limits<double>::epsilon())
        {
            lastTradedQuantity_ += quantity;
        }
        else
        {
            lastTradedPrice_ = price;
            lastTradedQuantity_ = quantity;
        }

        std::cout << "Trade Received for productId [" << productId_ << "] Total Traded Quantity [" << lastTradedQuantity_ << "] Traded Price [" << lastTradedPrice_ << "]" << std::endl;
    }
    else
    {
        throw std::runtime_error("Applying trade failed for orderbook!!");
    }

    return false;
}

void OrderBook::addOrUpdateSet(OrderPtr& orderPtr, char side)
{
    OrderListHashMap& orderListHashMap = (side == SIDE::BUY) ? bidOrderHashMap_ : offerOrderHashMap_;
    OrderListHashMap::iterator iter = orderListHashMap.find(orderPtr->price_);
    if (iter != orderListHashMap.end())
    {
        // price already exists
        iter->second->totalQty_ = iter->second->totalQty_ + orderPtr->quantity_;
        iter->second->orderList_.push_back(orderPtr);
    }
    else
    {
        // new price .. need to insert into set
        orderListHashMap[orderPtr->price_] = std::make_shared<OrderList>(orderPtr);
        if (side == SIDE::BUY)
            bidSet_.insert(orderListHashMap[orderPtr->price_]); // logn insert cost since bst
        else
            offerSet_.insert(orderListHashMap[orderPtr->price_]); // logn insert cost since bst
    }
}

bool OrderBook::deleteFromSet(OrderPtr& orderPtr, char side)
{
    OrderListHashMap& orderListHashMap = (side == SIDE::BUY) ? bidOrderHashMap_ : offerOrderHashMap_;
    OrderListHashMap::iterator iter = orderListHashMap.find(orderPtr->price_);

    if (iter != orderListHashMap.end())
    {
        iter->second->totalQty_ = iter->second->totalQty_ - orderPtr->quantity_; // reduce the quantity of the order from totalQty
        iter->second->orderList_.remove(orderPtr);

        if (iter->second->totalQty_ == 0) // remove the empty orderListPtr from the set
        {
            if (side == SIDE::BUY)
                bidSet_.erase(iter->second);
            else
                offerSet_.erase(iter->second);

            orderListHashMap.erase(iter); // remove the element off the hashmap too since no quantity ..
        }

        return true;
    }

    return false;
}

void OrderBook::getLastTradeDetails(double& price, int& quantity) const
{
    price = lastTradedPrice_;
    quantity = lastTradedQuantity_;
}
