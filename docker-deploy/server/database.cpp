#include "database.hpp"

void executeSQL(connection* C, string sql) {
    work W(*C);
    W.exec(sql);
    W.commit();
}

void getResult(connection* C, string sql, result& res) {
    work W(*C);
    res = result(W.exec(sql));
    W.commit();
}

time_t getCurrTime() {
    std::time_t current_time = std::time(nullptr);
    return current_time;
}

void createTable(string SQLfile, connection* C) {
    string sql;
    string curr;
    ifstream f(SQLfile);

    if (f.is_open()) {
        while (getline(f, curr)) {
            sql.append(curr);
        }
        f.close();
    }
    else {
        std::cout << "Cannot open file" << endl;
        return;
    }
    executeSQL(C, sql);
}

void deleteTable(connection* C, string tableName) {
    string sql = "DROP TABLE IF EXISTS " + tableName + " CASCADE;";
    executeSQL(C, sql);

}

string  addAccount(connection* C, int account_id, float balance) {
    work W(*C);
    if (balance <= 0) {  //TBD: CAN?
        W.commit();
        return "  <error id=\"" + to_string(account_id) + "\">Non-positive blance is not allowed</error>\n";
    }

    // Check account existance
    string sql;
    sql = "SELECT ACCOUNT_ID FROM ACCOUNT WHERE ACCOUNT_ID =" + to_string(account_id) + ";";
    result res;
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    if (res.size() != 0) {
        W.commit();
        return "  <error id=\"" + to_string(account_id) + "\">Account already exists</error>\n";
    }

    sql = "INSERT INTO ACCOUNT (ACCOUNT_ID, BALANCE) VALUES (" + to_string(account_id) + "," + to_string(balance) + ");";
    // executeSQL(C, sql);
    W.exec(sql);
    W.commit();
    return "  <created id=\"" + to_string(account_id) + "\"/>\n";
}

string addPosition(connection* C, string symbol, int account_id, float amount) {
    work W(*C);
    if (amount <= 0) {
        W.commit();
        return "  <error sym=\"" + to_string(symbol) + "\" id=\"" + to_string(account_id) + "\">Non-positive amount is not allowed</error>\n";
    }

    string sql;
    // check account
    sql = "SELECT ACCOUNT.ACCOUNT_ID FROM ACCOUNT WHERE ACCOUNT.ACCOUNT_ID =" + to_string(account_id) + ";";
    result res;
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    if (res.size() == 0) {
        W.commit();
        return "  <error sym=\"" + to_string(symbol) + "\" id=\"" + to_string(account_id) + "\">Account not exists</error>\n";
    }

    // check stock existance
    sql = "SELECT STOCK.STOCK_ID FROM STOCK WHERE SYMBOL=\'" + symbol + "\';";
    // result res;
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    if (res.size() == 0) {
        sql = "INSERT INTO STOCK (SYMBOL) VALUES (\'" + symbol + "\');";
        // executeSQL(C, sql);
        W.exec(sql);
    }

    sql = "SELECT STOCK_ID FROM STOCK WHERE SYMBOL =\'" + symbol + "\';";

    // getResult(C, sql, res);
    res = result(W.exec(sql));
    int stock_id = res.at(0).at(0).as<int>();


    // update position
    // sql = "SELECT COUNT(*) FROM POSITION, ACCOUNT WHERE SYMBOL ="+ to_string(symbol) + ";";
    // if(getResult(C, sql) != 0){ // find
    //     sql = "SELECT POSITION.AMOUNT FROM POSITION "
    //         "WHERE ACCOUNT_ID=" + to_string(account_id) +" AND STOCK_ID=" + to_string(stock_id) + ";";
    //     int amount = getResult(C, sql);
    //     sql = "UPDATE POSITION SET AMOUNT = 2500.00 WHERE ACCOUNT_ID = 2;"
    //     executeSQL(C, sql);
    // }else{ // not find
    //     sql = "INSERT"
    // }

    //为什么两个account_id?
    sql = "INSERT INTO POSITION (STOCK_ID, ACCOUNT_ID, AMOUNT) VALUES (" +
        to_string(stock_id) + "," + to_string(account_id) + "," + to_string(amount) +
        ")ON CONFLICT (STOCK_ID, ACCOUNT_ID) DO UPDATE SET AMOUNT = POSITION.AMOUNT + " + to_string(amount) + ";";
    // executeSQL(C, sql);
    W.exec(sql);
    W.commit();
    return "  <created sym=\"" + to_string(symbol) + "\" id=\"" + to_string(account_id) + "\"/>\n";
}

string cancelOrder(connection* C, int account_id, int trans_id) {
    work W(*C);
    // find trans order
    string sql;
    sql = "SELECT ORDERS.ACCOUNT_ID, ORDERS.STOCK_ID, ORDERS.AMOUNT, ORDERS.PRICE, STATUSS"
        " FROM ORDERS WHERE ORDERS.TRANS_ID=" + to_string(trans_id) + " AND ORDERS.STATUSS=\'OPEN\';";
    result res;
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    if (res.size() != 1) {
        W.commit();
        return "  <error id=\"" + to_string(trans_id) + "\">Order does not exist</error>\n";
    }
    int account = res.at(0).at(0).as<int>();
    int stock_id = res.at(0).at(1).as<int>();
    float amount = res.at(0).at(2).as<int>();
    int price = res.at(0).at(3).as<int>();
    string status = res.at(0).at(4).as<string>();
    if (account != account_id) {
        W.commit();
        return "  <error id=\"" + to_string(trans_id) + "\">Account does not have this order</error>\n";
    }

    if (amount > 0) {     // buy
        // add balance, REFUND
        sql = "UPDATE ACCOUNT SET BALANCE=BALANCE+" + to_string(price * amount) + " WHERE ACCOUNT.ACCOUNT_ID=" + to_string(account_id) + ";";
        // executeSQL(C, sql);
        W.exec(sql);
    }
    else {      // sell
        // add position
        sql = "INSERT INTO POSITION (STOCK_ID, ACCOUNT_ID, AMOUNT) VALUES (" + to_string(stock_id) + ", " + to_string(account_id) + ", " + to_string(abs(amount)) + ") "
            + "ON CONFLICT (STOCK_ID, ACCOUNT_ID) DO UPDATE SET AMOUNT = POSITION.AMOUNT+" + to_string(abs(amount)) + ";";
        // executeSQL(C, sql);
        W.exec(sql);
    }
    // order status cancel
    sql = "UPDATE ORDERS SET STATUSS=\'CANCELED\', ORDER_TIME=" + to_string(getCurrTime()) + " WHERE ORDERS.TRANS_ID=" + to_string(trans_id) + " AND STATUSS='OPEN';";
    // executeSQL(C, sql);
    W.exec(sql);

    string msg = "  <canceled id=\"" + to_string(trans_id) + "\">\n";

    sql = "SELECT ORDERS.AMOUNT, ORDER_TIME FROM ORDERS WHERE ORDERS.TRANS_ID=" + to_string(trans_id) + " AND STATUSS=\'CANCELED\';";
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    for (result::const_iterator it = res.begin(); it != res.end(); ++it) {
        msg += "    <canceled shares=\"" + to_string(it[0].as<int>()) + "\" time=\"" + to_string(it[1].as<string>()) + "\"/>\n";
    }
    sql = "SELECT ORDERS.AMOUNT, ORDERS.PRICE, ORDER_TIME FROM ORDERS WHERE ORDERS.TRANS_ID=" + to_string(trans_id) + " AND ORDERS.STATUSS=\'EXECUTED\';";
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    for (result::const_iterator it = res.begin(); it != res.end(); ++it) {
        msg += "    <executed shares=\"" + to_string(it[0].as<int>()) + "\" price=\"" + to_string(it[1].as<int>()) + "\" time=" + to_string(it[2].as<string>()) + "/>\n";
    }
    msg += "  </canceled>\n";
    W.commit();
    return msg;
}

string executeOrder(connection* C, string symbol, int account_id, float amount, int price) {
    // work W(*C);
    pqxx::transaction<pqxx::isolation_level::serializable> W(*C);
    string sql;
    sql = "SELECT ACCOUNT_ID FROM ACCOUNT WHERE ACCOUNT.ACCOUNT_ID=" + to_string(account_id) + ";";
    result res;
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    if (res.size() == 0) {
        W.commit();
        return "  <error sym=\"" + to_string(symbol) + "\" amount=\"" + to_string(amount) +
            "\" limit=\"" + to_string(price) + "\">Account not exists</error>\n";
    }
    if (amount == 0) {
        W.commit();
        return "  <error sym=\"" + to_string(symbol) + "\" amount=\"" + to_string(amount) +
            "\" limit=\"" + to_string(price) + "\">Ammount cannot be 0</error>\n";
    }

    sql = "SELECT STOCK.STOCK_ID FROM STOCK WHERE STOCK.SYMBOL=\'" + to_string(symbol) + "\';";
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    if (res.size() == 0) {
        W.commit();
        return "  <error sym=\"" + to_string(symbol) + "\" amount=\"" + to_string(amount) +
            "\" limit=\"" + to_string(price) + "\">Stock does not exist</error>\n";
    }
    int stock_id = res.at(0).at(0).as<int>();
    // int original_amount = amount;

    int trans_id;
    int order_time;

    if (amount > 0) { // buy
        // check balance and substract
        sql = "SELECT BALANCE FROM ACCOUNT WHERE ACCOUNT.ACCOUNT_ID=" + to_string(account_id) + ";";
        // getResult(C, sql, res);
        res = result(W.exec(sql));
        int balance = res.at(0).at(0).as<int>();
        if (balance < amount * price) {
            W.commit();
            return "  <error sym=\"" + to_string(symbol) + "\" amount=\"" + to_string(amount) +
                "\" limit=\"" + to_string(price) + "\">Balance is not enough</error>\n";
        }
        sql = "UPDATE ACCOUNT SET BALANCE=BALANCE-" + to_string(amount * price) + " WHERE ACCOUNT.ACCOUNT_ID=" + to_string(account_id) + ";";
        // executeSQL(C, sql);
        W.exec(sql);

        // add new order
        sql = "INSERT INTO ORDERS (ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(account_id) + "," + to_string(stock_id) + "," + to_string(amount)
            + "," + to_string(price) + ", \'OPEN\', " + to_string(getCurrTime()) + ") RETURNING ORDERS.TRANS_ID, ORDERS.ORDER_TIME;";
        // getResult(C, sql, res);
        res = result(W.exec(sql));

        for (result::const_iterator it = res.begin(); it != res.end(); ++it) {
            trans_id = it["TRANS_ID"].as<int>();
            order_time = it["ORDER_TIME"].as<int>();
        }
        std::cout << "line 229: new order time: " << order_time << std::endl;
        try {
            matchSellOrders(C, W, trans_id, account_id, stock_id, symbol, amount, price, order_time);
        }
        catch (const std::exception& e) {
            std::cerr << "CATCH: " << e.what() << std::endl;
        }

    }
    else {  // sell
        // 是否需要将amount转为正数?
        int abs_amount = abs(amount);
        // check position and substract
        sql = "SELECT POSITION.AMOUNT, POSITION.STOCK_ID FROM POSITION, STOCK WHERE STOCK.SYMBOL=\'" + to_string(symbol) + "\' AND STOCK.STOCK_ID = POSITION.STOCK_ID AND "
            "POSITION.ACCOUNT_ID=" + to_string(account_id) + ";";
        // getResult(C, sql, res);
        res = result(W.exec(sql));
        if (res.size() == 0) {
            W.commit();
            return "  <error sym=\"" + to_string(symbol) + "\" amount=\"" + to_string(amount) +
                "\" limit=\"" + to_string(price) + "\">Account does not have this stock</error>\n";
        }
        int original_amount = res.at(0).at(0).as<int>();
        stock_id = res.at(0).at(1).as<int>();
        if (original_amount < abs_amount) {
            W.commit();
            return "  <error sym=\"" + to_string(symbol) + "\" amount=\"" + to_string(amount) +
                "\" limit=\"" + to_string(price) + "\">Amount is not enough</error>\n";
        }
        else if (original_amount == abs_amount) {
            // delete data from position
            sql = "DELETE FROM POSITION WHERE POSITION.STOCK_ID=" + to_string(stock_id) + " AND POSITION.ACCOUNT_ID=" + to_string(account_id) + ";";
            // executeSQL(C, sql);
            W.exec(sql);
        }
        else if (original_amount > abs_amount) {
            //update data in position
            sql = "UPDATE POSITION SET AMOUNT=AMOUNT-" + to_string(abs_amount) +
                " WHERE POSITION.STOCK_ID=" + to_string(stock_id) + " AND POSITION.ACCOUNT_ID=" + to_string(account_id) + ";";
            // executeSQL(C, sql);
            W.exec(sql);
        }
        // add new order
        sql = "INSERT INTO ORDERS (ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(account_id) + "," + to_string(stock_id) + "," + to_string(amount)
            + "," + to_string(price) + ", \'OPEN\', " + to_string(getCurrTime()) + ") RETURNING ORDERS.TRANS_ID, ORDERS.ORDER_TIME;";
        // getResult(C, sql, res);
        res = result(W.exec(sql));
        for (result::const_iterator it = res.begin(); it != res.end(); ++it) {
            trans_id = it["TRANS_ID"].as<int>();
            order_time = it["ORDER_TIME"].as<int>();
        }
        std::cout << "line 267: new order time: " << order_time << std::endl;

        matchBuyOrders(C, W, trans_id, account_id, stock_id, symbol, amount, price, order_time);
    }

    W.commit();
    return "  <opened sym=\"" + to_string(symbol) + "\" amount=\"" + to_string(amount) + "\" limit=\"" +
        to_string(price) + "\" id=\"" + to_string(trans_id) + "\"/>\n";
}

void matchBuyOrders(connection* C, pqxx::transaction<pqxx::isolation_level::serializable>& W, int seller_trans_id, int sellerId, int stock_id, string symbol, float amount, int price, int order_time) {
    std::cout << "line 288: order_time in matchBuyOrders: " << order_time << std::endl;
    result res;
    // work W(*C);
    // find all matched buy orders
    string sqlMatch = "SELECT TRANS_ID, ACCOUNT_ID, AMOUNT, PRICE, ORDER_TIME FROM ORDERS , STOCK "
        "WHERE STOCK.SYMBOL = '" + symbol + "' AND STOCK.STOCK_ID=ORDERS.STOCK_ID AND "
        "STATUSS = \'OPEN\' AND " +
        "(AMOUNT > 0  AND PRICE >= " + to_string(price) + ") "
        "ORDER BY ORDER_TIME ASC, ORDER_ID ASC, PRICE ASC;";
    // getResult(C, sqlMatch, res);
    res = result(W.exec(sqlMatch));

    // result matchingOrders = W.exec(sqlMatch);

    if (!res.empty()) {
        for (auto order : res) {
            int matchingTransId = order["TRANS_ID"].as<int>();
            int matchingAccountId = order["ACCOUNT_ID"].as<int>();
            float matchingAmount = order["AMOUNT"].as<float>();
            int matchingPrice = order["PRICE"].as<int>();
            int matchingTime = order["ORDER_TIME"].as<int>();

            int executionPrice = order["ORDER_TIME"].as<int>() <= getCurrTime() ? matchingPrice : price;
            float executionAmount = min(abs(amount), abs(matchingAmount));

            std::cout << "in matchBuyOrders: amount: " << to_string(amount) << std::endl;

            std::cout << "in matchBuyOrders: matchingBuyerId: " << to_string(matchingAccountId) << " matchingAmount: " << to_string(matchingAmount) << std::endl;

            float splitAmount;
            int type = 65535;
            // seller split
            if ((abs(amount) != abs(matchingAmount)) && (executionAmount == abs(matchingAmount))) {
                // float splitAmount = executionAmount - abs(amount);
                // string sql = "INSERT INTO ORDERS (TRANS_ID, ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(seller_trans_id) + "," + to_string(sellerId) + "," + to_string(stock_id) + "," + to_string(splitAmount)
                //     + "," + to_string(price) + ", \'OPEN\', " + to_string(order_time) + ");";
                // executeSQL(C, sql);
                splitAmount = executionAmount - abs(amount);
                std::cout << "line 321: amount:" << amount << " matchingAmount: " << matchingAmount << " executionAmount: " << executionAmount << " splitAmount: " << splitAmount << std::endl;
                type = 1;
                std::cout << "line 315: seller_trans_id: " << seller_trans_id << " seller_id: " << sellerId << " stock_id: " << stock_id << " splitAmount: " << splitAmount << " price: " << price << " order_time: " << order_time << std::endl;
            }
            // buyer split
            else if ((abs(amount) != abs(matchingAmount)) && (executionAmount == abs(amount))) {
                // float splitAmount = abs(matchingAmount) - executionAmount;
                // string sql = "INSERT INTO ORDERS (TRANS_ID, ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(matchingTransId) + "," + to_string(matchingAccountId) + "," + to_string(stock_id) + "," + to_string(splitAmount)
                //     + "," + to_string(matchingPrice) + ", \'OPEN\', " + to_string(matchingTime) + ");";
                // executeSQL(C, sql);
                splitAmount = abs(matchingAmount) - executionAmount;
                std::cout << "line 332: amount:" << amount << " matchingAmount: " << matchingAmount << " executionAmount: " << executionAmount << " splitAmount: " << splitAmount << std::endl;
                type = 0;
                std::cout << "line 323: buyer_trans_id: " << matchingTransId << " buyer_id: " << matchingAccountId << " stock_id: " << stock_id << " splitAmount: " << splitAmount << " price: " << matchingPrice << " order_time: " << matchingTime << std::endl;
            }

            string refundBuyerBalance = "UPDATE ACCOUNT SET BALANCE=BALANCE+" + to_string(executionAmount * matchingPrice) + " WHERE ACCOUNT.ACCOUNT_ID=" + to_string(matchingAccountId) + ";";
            // string refundBuyerBalance = "UPDATE ACCOUNT SET BALANCE=BALANCE+" + to_string(matchingAmount * matchingPrice) + " WHERE ACCOUNT.ACCOUNT_ID=" + to_string(matchingAccountId) + ";";
            // executeSQL(C, refundBuyerBalance);
            W.exec(refundBuyerBalance);

            // string refundSellerPosition = "UPDATE POSITION SET AMOUNT=AMOUNT+" + to_string(abs(amount)) +
            string refundSellerPosition = "UPDATE POSITION SET AMOUNT=AMOUNT+" + to_string(abs(executionAmount)) +
                " WHERE POSITION.STOCK_ID=" + to_string(stock_id) + " AND POSITION.ACCOUNT_ID=" + to_string(sellerId) + ";";
            // executeSQL(C, refundSellerPosition);
            W.exec(refundSellerPosition);
            std::cout << "line 332: amount: " << amount << " stock_id: " << stock_id << " sellerId: " << sellerId << std::endl;

            updateBalancesAndPositions(C, W, matchingAccountId, sellerId, symbol, executionAmount, executionPrice);

            int executed_time = getCurrTime();
            markOrdersAsExecuted(C, W, seller_trans_id, sellerId, -executionAmount, executionPrice, executed_time);
            markOrdersAsExecuted(C, W, order["TRANS_ID"].as<int>(), matchingAccountId, executionAmount, executionPrice, executed_time);

            if (type == 1) {
                string sql = "INSERT INTO ORDERS (TRANS_ID, ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(seller_trans_id) + "," + to_string(sellerId) + "," + to_string(stock_id) + "," + to_string(splitAmount)
                    + "," + to_string(price) + ", \'OPEN\', " + to_string(order_time) + ");";
                // executeSQL(C, sql);
                W.exec(sql);
                std::cout << "line 350: seller_trans_id: " << seller_trans_id << " seller_id: " << sellerId << " stock_id: " << stock_id << " splitAmount: " << splitAmount << " price: " << price << " order_time: " << order_time << std::endl;
            }
            // buyer split
            else if (type == 0) {
                string sql = "INSERT INTO ORDERS (TRANS_ID, ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(matchingTransId) + "," + to_string(matchingAccountId) + "," + to_string(stock_id) + "," + to_string(splitAmount)
                    + "," + to_string(matchingPrice) + ", \'OPEN\', " + to_string(matchingTime) + ");";
                // executeSQL(C, sql);
                W.exec(sql);
                std::cout << "line 357: buyer_trans_id: " << matchingTransId << " buyer_id: " << matchingAccountId << " stock_id: " << stock_id << " splitAmount: " << splitAmount << " price: " << matchingPrice << " order_time: " << matchingTime << std::endl;
            }

            amount += executionAmount;
            std::cout << "line 383: in matchBuyOrders: amount: " << to_string(amount) << std::endl;

            if (amount == 0) {
                std::cout << "line 385: delete from orders: account_id: " + to_string(matchingAccountId) + " stock_id: " + to_string(stock_id) << std::endl;
                string deletesql = "DELETE FROM ORDERS WHERE ORDERS.STOCK_ID=" + to_string(stock_id) + " AND ORDERS.ACCOUNT_ID=" + to_string(matchingAccountId) + " AND ORDERS.AMOUNT = 0;";
                // executeSQL(C, deletesql);
                W.exec(deletesql);
                // W.commit();
                // break;
                continue;
            }

            std::cout << "line 380: sellerId: " << to_string(sellerId) << " stock_id: " << to_string(stock_id) << std::endl;
            string getSellerAmount = "SELECT POSITION.AMOUNT FROM POSITION WHERE POSITION.STOCK_ID="
                + to_string(stock_id) + " AND POSITION.ACCOUNT_ID=" + to_string(sellerId) + ";";
            result sellerOrder;
            // getResult(C, getSellerAmount, sellerOrder);
            sellerOrder = result(W.exec(getSellerAmount));
            if (sellerOrder.size() == 0) {
                std::cout << "line 386: sellerOrder size is 0" << std::endl;
                continue;
            }
            float sellerAmount = sellerOrder.at(0).at(0).as<float>();
            if (sellerAmount == 0) {
                string deletesql = "DELETE FROM POSITION WHERE POSITION.STOCK_ID=" + to_string(stock_id) + " AND POSITION.ACCOUNT_ID=" + to_string(sellerId) + ";";
                // executeSQL(C, deletesql);
                W.exec(deletesql);
                std::cout << "line 393: seller amount in match buy orders is 0!!!" << std::endl;
                break;
            }


        }
    }
    // if (amount != 0) {
    //     string sql = "INSERT INTO ORDERS VALUES (ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) (" + to_string(sellerId) + "," + to_string(stock_id) + "," + to_string(amount)
    //         + "," + to_string(price) + ", OPEN, " + to_string(getCurrTime()) + ");";
    //     executeSQL(C, sql);
    // }
    // W.commit();

}


void matchSellOrders(connection* C, pqxx::transaction<pqxx::isolation_level::serializable>& W, int buyer_trans_id, int buyerId, int stock_id, string symbol, float amount, int price, int order_time) {
    std::cout << "line 387: order_time in matchSellOrders: " << order_time << std::endl;
    result res;
    // work W(*C);
    // find all matched sell orders
    string sqlMatch = "SELECT TRANS_ID, ACCOUNT_ID, AMOUNT, PRICE, ORDER_TIME FROM ORDERS, STOCK "
        "WHERE STOCK.SYMBOL = '" + symbol + "' AND STOCK.STOCK_ID=ORDERS.STOCK_ID AND "
        "STATUSS = 'OPEN' AND " +
        "(AMOUNT < 0  AND PRICE <= " + to_string(price) + ") "
        "ORDER BY ORDER_TIME ASC, ORDER_ID ASC, PRICE ASC;";
    // getResult(C, sqlMatch, res);
    res = result(W.exec(sqlMatch));

    // result matchingOrders = W.exec(sqlMatch);

    if (!res.empty()) {
        for (auto order : res) {
            int matchingTransId = order["TRANS_ID"].as<int>();
            int matchingAccountId = order["ACCOUNT_ID"].as<int>();
            float matchingAmount = order["AMOUNT"].as<float>();
            int matchingPrice = order["PRICE"].as<int>();
            int matchingTime = order["ORDER_TIME"].as<int>();

            int executionPrice = order["ORDER_TIME"].as<int>() <= getCurrTime() ? matchingPrice : price;
            float executionAmount = min(abs(amount), abs(matchingAmount));

            float splitAmount;
            int type = 65535;
            // buyer split
            if ((abs(amount) != abs(matchingAmount)) && (executionAmount == abs(matchingAmount))) {
                // float splitAmount = abs(amount) - executionAmount;
                // string sql = "INSERT INTO ORDERS (TRANS_ID, ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(buyer_trans_id) + "," + to_string(buyerId) + "," + to_string(stock_id) + "," + to_string(splitAmount)
                //     + "," + to_string(price) + ", \'OPEN\', " + to_string(order_time) + ");";
                // executeSQL(C, sql);
                splitAmount = abs(amount) - executionAmount;
                std::cout << "line 422: amount:" << amount << " matchingAmount: " << matchingAmount << " executionAmount: " << executionAmount << " splitAmount: " << splitAmount << std::endl;
                type = 1;
                std::cout << "line 390: buyer_trans_id: " << buyer_trans_id << " buyer_id: " << buyerId << " stock_id: " << stock_id << " splitAmount: " << splitAmount << " price: " << price << " order_time: " << order_time << std::endl;
            }
            // seller split
            else if ((abs(amount) != abs(matchingAmount)) && (executionAmount == abs(amount))) {
                // float splitAmount = executionAmount - abs(matchingAmount);
                // string sql = "INSERT INTO ORDERS (TRANS_ID, ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(matchingTransId) + "," + to_string(matchingAccountId) + "," + to_string(stock_id) + "," + to_string(splitAmount)
                //     + "," + to_string(matchingPrice) + ", \'OPEN\', " + to_string(matchingTime) + ");";
                // executeSQL(C, sql);
                splitAmount = executionAmount - abs(matchingAmount);
                std::cout << "line 433: amount:" << amount << " matchingAmount: " << matchingAmount << " executionAmount: " << executionAmount << " splitAmount: " << splitAmount << std::endl;
                type = 0;
                std::cout << "line 398: seller_trans_id: " << matchingTransId << " buyer_id: " << matchingAccountId << " stock_id: " << stock_id << " splitAmount: " << splitAmount << " price: " << matchingPrice << " order_time: " << matchingTime << std::endl;
            }

            // string refundBuyerBalance = "UPDATE ACCOUNT SET BALANCE=BALANCE+" + to_string(amount * price) + " WHERE ACCOUNT.ACCOUNT_ID=" + to_string(buyerId) + ";";
            string refundBuyerBalance = "UPDATE ACCOUNT SET BALANCE=BALANCE+" + to_string(executionAmount * price) + " WHERE ACCOUNT.ACCOUNT_ID=" + to_string(buyerId) + ";";
            // executeSQL(C, refundBuyerBalance);
            W.exec(refundBuyerBalance);

            // string refundSellerPosition = "UPDATE POSITION SET AMOUNT=AMOUNT+" + to_string(abs(matchingAmount)) +
            string refundSellerPosition = "UPDATE POSITION SET AMOUNT=AMOUNT+" + to_string(abs(executionAmount)) +
                " WHERE POSITION.STOCK_ID=" + to_string(stock_id) + " AND POSITION.ACCOUNT_ID=" + to_string(matchingAccountId) + ";";
            // executeSQL(C, refundSellerPosition);
            W.exec(refundSellerPosition);
            std::cout << "line 407: matchingAmount: " << matchingAmount << " stock_id: " << stock_id << " sellerId: " << matchingAccountId << std::endl;

            updateBalancesAndPositions(C, W, buyerId, matchingAccountId, symbol, executionAmount, executionPrice);

            int executed_time = getCurrTime();
            markOrdersAsExecuted(C, W, buyer_trans_id, buyerId, executionAmount, executionPrice, executed_time);
            markOrdersAsExecuted(C, W, order["TRANS_ID"].as<int>(), matchingAccountId, -executionAmount, executionPrice, executed_time);

            // after mark executed, add new open
            if (type == 1) {
                // float splitAmount = abs(amount) - executionAmount;
                string sql = "INSERT INTO ORDERS (TRANS_ID, ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(buyer_trans_id) + "," + to_string(buyerId) + "," + to_string(stock_id) + "," + to_string(splitAmount)
                    + "," + to_string(price) + ", \'OPEN\', " + to_string(order_time) + ");";
                // executeSQL(C, sql);
                W.exec(sql);
                std::cout << "line 426: buyer_trans_id: " << buyer_trans_id << " buyer_id: " << buyerId << " stock_id: " << stock_id << " splitAmount: " << splitAmount << " price: " << price << " order_time: " << order_time << std::endl;
            }
            // seller split
            else if ((abs(amount) != abs(matchingAmount)) && (executionAmount == abs(amount))) {
                // float splitAmount = executionAmount - abs(matchingAmount);
                string sql = "INSERT INTO ORDERS (TRANS_ID, ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(matchingTransId) + "," + to_string(matchingAccountId) + "," + to_string(stock_id) + "," + to_string(splitAmount)
                    + "," + to_string(matchingPrice) + ", \'OPEN\', " + to_string(matchingTime) + ");";
                // executeSQL(C, sql);
                W.exec(sql);
                std::cout << "line 434: seller_trans_id: " << matchingTransId << " buyer_id: " << matchingAccountId << " stock_id: " << stock_id << " splitAmount: " << splitAmount << " price: " << matchingPrice << " order_time: " << matchingTime << std::endl;
            }

            amount -= executionAmount;

            std::cout << "line 509: buyerId: " << to_string(buyerId) << " stock_id: " << to_string(stock_id) << " amount: " << to_string(amount) << std::endl;

            string getSellerAmount = "SELECT POSITION.AMOUNT FROM POSITION WHERE POSITION.STOCK_ID="
                + to_string(stock_id) + " AND POSITION.ACCOUNT_ID=" + to_string(matchingAccountId) + ";";
            // result sellerOrder = W.exec(getSellerAmount);
            result sellerOrder;
            // getResult(C, getSellerAmount, sellerOrder);
            sellerOrder = result(W.exec(getSellerAmount));
            if (sellerOrder.size() == 0) {
                continue;
            }
            float sellerAmount = sellerOrder.at(0).at(0).as<float>();
            if (sellerAmount == 0) {
                string deletesql = "DELETE FROM POSITION WHERE POSITION.STOCK_ID=" + to_string(stock_id) + " AND POSITION.ACCOUNT_ID=" + to_string(matchingAccountId) + ";";
                // executeSQL(C, deletesql);
                W.exec(deletesql);
                // W.exec(deletesql);
                // W.commit();
                // break;
                continue;
            }
        }
    }
    // if (amount != 0) {
    //     string sql = "INSERT INTO ORDERS (ACCOUNT_ID, STOCK_ID, AMOUNT, PRICE, STATUSS, ORDER_TIME) VALUES (" + to_string(buyerId) + "," + to_string(stock_id) + "," + to_string(amount)
    //         + "," + to_string(price) + ", \'OPEN\', " + to_string(getCurrTime()) + ");";
    //     executeSQL(C, sql);
    // }
    // W.commit();

}


void updateBalancesAndPositions(connection* C, pqxx::transaction<pqxx::isolation_level::serializable>& W, int buyerId, int sellerId, string symbol, float amount, int price) {
    float totalCost = amount * price;

    // update buyer's balance (subtract total cost)
    string updateBuyerBalance = "UPDATE ACCOUNT SET BALANCE = BALANCE - " + to_string(totalCost) +
        " WHERE ACCOUNT_ID = " + to_string(buyerId) + ";";
    W.exec(updateBuyerBalance);
    // executeSQL(C, updateBuyerBalance);

    // update seller's balance (add total cost)
    string updateSellerBalance = "UPDATE ACCOUNT SET BALANCE = BALANCE + " + to_string(totalCost) +
        " WHERE ACCOUNT_ID = " + to_string(sellerId) + ";";
    W.exec(updateSellerBalance);
    // executeSQL(C, updateSellerBalance);

    string sql = "SELECT COUNT(*) FROM STOCK WHERE SYMBOL =\'" + symbol + "\';";
    result res;
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    int stock_id = res.at(0).at(0).as<int>();

    // update or insert buyer's position
    string updateBuyerPosition = "INSERT INTO POSITION (STOCK_ID, ACCOUNT_ID, AMOUNT) VALUES (" +
        to_string(stock_id) + ", " + to_string(buyerId) + ", " + to_string(amount) +
        ") ON CONFLICT (STOCK_ID, ACCOUNT_ID) DO UPDATE SET " +
        "AMOUNT = POSITION.AMOUNT + EXCLUDED.AMOUNT;";
    W.exec(updateBuyerPosition);
    // executeSQL(C, updateBuyerPosition);

    // update seller's position(amount<0, so still add)
    string updateSellerPosition = "UPDATE POSITION SET AMOUNT = AMOUNT - " + to_string(amount) +
        " WHERE ACCOUNT_ID = " + to_string(sellerId) +
        " AND STOCK_ID = (SELECT STOCK_ID FROM STOCK WHERE SYMBOL = '" + symbol + "');";
    W.exec(updateSellerPosition);
    // executeSQL(C, updateSellerPosition);
    std::cout << "line 476: amount: " << amount << " account_id : " << sellerId << " symbol:" << symbol << std::endl;
}

void markOrdersAsExecuted(connection* C, pqxx::transaction<pqxx::isolation_level::serializable>& W, int orderId, int accountId, float amount, int price, int executed_time) {
    string markOrderExecuted = "UPDATE ORDERS SET STATUSS = 'EXECUTED', AMOUNT = " + to_string(amount) +
        ", PRICE = " + to_string(price) + ", ORDER_TIME = " + to_string(executed_time) +
        " WHERE ORDERS.TRANS_ID = " + to_string(orderId) +
        " AND ORDERS.ACCOUNT_ID = " + to_string(accountId) +
        " AND STATUSS='OPEN';";
    // std::cout << "line 426: orderId: " << orderId << " accountId: " << accountId << " amount: " << amount << " price: " << price << std::endl;

    W.exec(markOrderExecuted);
    // executeSQL(C, markOrderExecuted);
    std::cout << "line 484: transId: " << orderId << " accountId: " << accountId << " amount: " << amount << " price: " << price << " executed time: " << executed_time << std::endl;

}


string query(connection* C, int trans_id, int account_id) {
    // work W(*C);
    pqxx::transaction<pqxx::isolation_level::serializable> W(*C);
    string msg = "  <status id=\"" + to_string(trans_id) + "\">\n";
    // check account
    string sql;
    sql = "SELECT ACCOUNT_ID FROM ACCOUNT WHERE ACCOUNT.ACCOUNT_ID=" + to_string(account_id) + ";";
    result res;
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    if (res.size() == 0) {
        W.commit();
        return "  <error id=\"" + to_string(account_id) + "\">Account not exists</error>\n";
    }

    // check trans_id
    sql = "SELECT ORDERS.STOCK_ID, ORDERS.AMOUNT, ORDERS.PRICE, ORDER_TIME, ORDERS.ACCOUNT_ID FROM ORDERS WHERE "
        "ORDERS.TRANS_ID=" + to_string(trans_id) + " AND ORDERS.ACCOUNT_ID=" + to_string(account_id) + ";";
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    if (res.size() == 0) {
        W.commit();
        return "  <error id=\"" + to_string(trans_id) + "\">Account does not have this order</error>\n";
        // TBD order not exist
    }    

    // order可能split了，查询结果可能有多个
    // find open
    sql = "SELECT ORDERS.AMOUNT FROM ORDERS WHERE ORDERS.TRANS_ID=" + to_string(trans_id) + " AND ORDERS.ACCOUNT_ID=" + to_string(account_id) + " AND ORDERS.STATUSS=\'OPEN\';";
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    for (result::const_iterator it = res.begin(); it != res.end(); ++it) {
        msg += "    <open shares=\"" + to_string(it[0].as<int>()) + "\"/>\n";
    }
    // msg += "/r<open shares=" + to_string(res.at(0).at(0).as<int>()) + "/>\n";

    // find cancel
    sql = "SELECT ORDERS.AMOUNT, ORDER_TIME FROM ORDERS WHERE ORDERS.TRANS_ID=" + to_string(trans_id) + " AND ORDERS.ACCOUNT_ID=" + to_string(account_id) + " AND ORDERS.STATUSS=\'CANCELED\';";
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    for (result::const_iterator it = res.begin(); it != res.end(); ++it) {
        msg += "    <canceled shares=\"" + to_string(it[0].as<int>()) + "\" time=\"" + to_string(it[1].as<string>()) + "\"/>\n";
    }
    // msg += "/r<canceled shares=" + to_string(res.at(0).at(0).as<int>()) + " time=" + to_string(res.at(0).at(1).as<string>()) + "/>\n";

    // find execute
    sql = "SELECT ORDERS.AMOUNT, ORDERS.PRICE, ORDER_TIME FROM ORDERS WHERE ORDERS.TRANS_ID=" + to_string(trans_id) + " AND ORDERS.ACCOUNT_ID=" + to_string(account_id) + " AND ORDERS.STATUSS=\'EXECUTED\';";
    // getResult(C, sql, res);
    res = result(W.exec(sql));
    for (result::const_iterator it = res.begin(); it != res.end(); ++it) {
        msg += "    <executed shares=\"" + to_string(it[0].as<int>()) + "\" price=\"" + to_string(it[1].as<int>()) + "\" time=\"" + to_string(it[2].as<string>()) + "\"/>\n";
    }
    // msg += "/r<executed shares=" + to_string(res.at(0).at(0).as<int>()) + " price=" + to_string(res.at(0).at(1).as<int>()) + " time=" + to_string(res.at(0).at(2).as<string>()) + "/>\n";

    msg += "  </status>\n";
    W.commit();
    return msg;
}

