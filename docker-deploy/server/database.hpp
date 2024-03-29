#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdlib>
#include <string>
#include <pqxx/pqxx>
#include <fstream>
#include <iostream>
#include <ctime>

using namespace std;
using namespace pqxx;

void executeSQL(connection* C, string sql);
result getResult(work &W, string sql);

string getCurrTime();

void createTable(string SQLfile, connection *C);
void deleteTable(connection *C, string tableName);

string addAccount(connection *C, float balance);
string addPosition(connection *C, string symbol, int account_id, float amount);

string openOrder(connection *C, string symbol, int account_id, int trans_id, float amount, int price);
string cancelOrder(connection *C, int account_id, int trans_id);
string executeOrder(connection *C, string symbol, int account_id, int trans_id, float amount, int price);

string query(connection *C, string tableName);

#endif