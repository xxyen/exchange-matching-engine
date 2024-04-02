DROP TABLE IF EXISTS ORDERS CASCADE;
CREATE TABLE ORDERS (
    ORDER_ID SERIAL NOT NULL,
    TRANS_ID SERIAL NOT NULL,
    ACCOUNT_ID INT NOT NULL,
    STOCK_ID INT NOT NULL,
    AMOUNT FLOAT NOT NULL,
    PRICE INT NOT NULL,
    STATUSS VARCHAR(256) NOT NULL,
    ORDER_TIME INT NOT NULL,
    PRIMARY KEY (ORDER_ID),
    FOREIGN KEY (ACCOUNT_ID) REFERENCES ACCOUNT(ACCOUNT_ID) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (STOCK_ID) REFERENCES STOCK(STOCK_ID) ON DELETE CASCADE ON UPDATE CASCADE
);