#include "orderbookmanager.h"
#include <regex>

std::vector<std::string> tokenize(
	const std::string& str,
	const std::regex re)
{
	std::sregex_token_iterator begin(str.begin(), str.end(), re, -1), end;
	std::vector<std::string> tokenized{ begin, end };

	// Additional check to remove empty strings 
	tokenized.erase(
		std::remove_if(tokenized.begin(),
			tokenized.end(),
			[](std::string const& s) {
				return s.size() == 0;
			}),
		tokenized.end());

	return tokenized;
}

void OrderBookManager::sanitizeInputs(int productId, int orderId, char side, int quantity, double price)
{
	if (productId <= 0)
		throw std::runtime_error("Received invalid productId");

	if (orderId <= 0)
		throw std::runtime_error("Received invalid orderId");

	if (side != SIDE::BUY && side != SIDE::SELL)
		throw std::runtime_error("Invalid Side received");

	if (quantity <= 0 || price <= 0)
		throw std::runtime_error("Invalid price/quantity received");
}


void OrderBookManager::sanitizeInputs(int orderId, char side, int quantity, double price)
{
	if (orderId <= 0)
		throw std::runtime_error("Received invalid orderId");

	if (side != SIDE::BUY && side != SIDE::SELL)
		throw std::runtime_error("Invalid Side received");

	if (quantity <= 0 || price <= 0)
		throw std::runtime_error("Invalid price/quantity received");
}

void OrderBookManager::sanitizeInputs(int productId, int quantity, double price)
{
	if (productId <= 0)
		throw std::runtime_error("Received invalid productId");

	if (quantity <= 0 || price <= 0)
		throw std::runtime_error("Invalid price/quantity received");
}

// take actions as per the orderbook for the productId (look up orderbook from orderId if productId not already available)
void OrderBookManager::action(const char action, int productId, int orderId, char side, int quantity, double price)
{
	try {
		switch (action)
		{
		case ACTION::NEW:
		{
			// sanitize the received inputs
			sanitizeInputs(productId, orderId, side, quantity, price);

			// check if orderId already exists
			if (ordIdToOrderBook_.find(orderId) != ordIdToOrderBook_.end())
				throw std::runtime_error("OrderId already exists!!!");

			auto op = prdIdToOrderBook_.emplace(productId, productId);
			// add new order now
			op.first->second.enterOrder(orderId, side, price, quantity);

			ordIdToOrderBook_.emplace(orderId, op.first);
			break;
		}
		case ACTION::MODIFY:
		{
			// sanitize the received inputs
			sanitizeInputs(orderId, side, quantity, price);

			// check if orderId already exists
			auto op = ordIdToOrderBook_.find(orderId);
			if (op == ordIdToOrderBook_.end())
				throw std::runtime_error("OrderId not available!!!");

			auto& ob = op->second->second;
			if (!ob.modifyOrder(orderId, quantity))
				throw std::runtime_error("Failed modifying order");
			break;
		}
		case ACTION::REMOVE:
		{
			// sanitize the received inputs
			sanitizeInputs(orderId, side, quantity, price);

			// check if orderId already exists
			auto op = ordIdToOrderBook_.find(orderId);
			if (op == ordIdToOrderBook_.end())
				throw std::runtime_error("OrderId not available!!!");

			auto& ob = op->second->second;
			if (!ob.deleteOrder(orderId))
				throw std::runtime_error("Failed deleting order");
			break;
		}
		case ACTION::TRADE:
		{
			// sanitize the received inputs
			sanitizeInputs(productId, quantity, price);
			auto it = prdIdToOrderBook_.find(productId);

			auto& ob = it->second;
			ob.handleTrade(price, quantity);

			break;
		}
		default:
			throw std::runtime_error("Invalid Action provided!!");
		}
	}
	catch (std::exception& ex)
	{
		exceptions_.emplace_back(ex.what(), orderId);
	}
}

void OrderBookManager::action(const std::string& msg)
{
	try {
		const std::regex re(R"([,;: ])");
		std::vector<std::string> cmds = tokenize(msg, re);
		const char cmd = (cmds.at(0)).at(0);

		switch (cmd)
		{
		case ACTION::NEW:
			if (cmds.size() != 6)
				throw std::runtime_error("Invalid arguments for new order");
			action(cmd, std::stoi(cmds.at(1)), std::stoi(cmds.at(2)), (cmds.at(3)).at(0), std::stoi(cmds.at(4)), std::stod(cmds.at(5)));
			break;
		case ACTION::MODIFY:
		case ACTION::REMOVE:
			if (cmds.size() != 5)
				throw std::runtime_error("Invalid arguments for modify/cancel order");
			action(cmd, 0, std::stoi(cmds.at(1)), (cmds.at(2)).at(0), std::stoi(cmds.at(3)), std::stod(cmds.at(4)));
			break;
		case ACTION::TRADE: // trade
			if (cmds.size() != 4)
				throw std::runtime_error("Invalid arguments for trade");
			action(cmd, std::stoi(cmds.at(1)), 0, 0, std::stoi(cmds.at(2)), std::stod(cmds.at(3)));
			break;
		default:
			throw std::runtime_error("Invalid Action provided!!");
		}
	}
	catch (std::exception& ex)
	{
		exceptions_.emplace_back(ex.what(), 0);
	}
}

void OrderBookManager::printOB(const int productId/* = 0*/)
{
	int quantity;
	double price;

	if (!productId)
	{
		// print the orderbook for all the existing orderbooks
		for (const auto& elem : prdIdToOrderBook_)
		{
			std::cout << "ProductId [" << elem.first << "]" << std::endl;
			elem.second.printOrderBook();

			elem.second.getLastTradeDetails(price, quantity);
			std::cout << "Last Traded Price [" << price << "] Last Traded Quantity [" << quantity << "]" << std::endl;
		}
	}
	else
	{
		std::cout << "ProductId [" << productId << "]" << std::endl;
		auto ob = prdIdToOrderBook_.find(productId);
		if (ob == prdIdToOrderBook_.end())
			throw std::runtime_error("OrderBook doesn't exists for productId");
		ob->second.printOrderBook();

		ob->second.getLastTradeDetails(price, quantity);
		std::cout << "Last Traded Price [" << price << "] Last Traded Quantity [" << quantity << "]" << std::endl;
	}
}

void OrderBookManager::printExceptions()
{
	for (const auto& elem : exceptions_)
	{
		if (elem.id_)
			std::cout << "OrderId [" << elem.id_ << "] msg [" << elem.msg_ << "]" << std::endl;
		else
			std::cout << "Msg parsing failed with error [" << elem.msg_ << "]" << std::endl;
	}
}