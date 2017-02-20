# libDAL
A  DAL(Data Acess Layer) library, written in C.

## 功能
* 支持Hash、Range sharding
* Read/Write Splitting
* Load balancing 
* failover

## DAL支持的记录主键类型
* 无符号整数(4个字节)
* 无符号整数的字符串形式(1-11个字节)
* 无符号整数的十六进制编码形式(8个字节)
* 字符串
* 二进制，比如64位整数。

## DAL支持的数据库结构
DAL支持多组数据库，每一组数据库采用单主、主-主、主-从结构，主从数据库都可以设置：读(r)、写(w)、读写( rw)。  
Group1: Master1 --> Slave1,Slave2,...SlaveN  
Group2: Master2 --> Slave1,Slave2,...SlaveN  
... ...  
GroupM: MasterM --> Slave1,Slave2,...SlaveN  

## 读写分离
数据库组中任一数据库都可以设置：读(r)、写(w)、读写( rw)。  

## Sharding
怎么根据记录主键确定记录存放到哪组数据库？  
首先根据记录主键的类型采用下面的方式计算出记录ID（记录ID是4个字节的unsigned int）：  
* hash：可以用于字符串、二进制、整数等类型的记录主键。
* unescape：十六进制解码，只可以用于十六进制编码的无符号整数的记录主键。
* strtoul： 只可以用无符号整数的记录主键。
* uint：记录主键就是无符号整数的记录ID，不需要进行任何转换。  
再用记录ID采用下面的方式来确定记录存放数据库组：
* 记录ID取模
* 记录ID范围
* 记录ID查表(暂不支持)  
**注意区分记录ID和记录主键之间的关系。**  
sharding ID取模方式中记录ID可以采用hash、unescape、strtoul、uint。  
sharding ID范围方式中记录ID不能采用hash，hash计算得出的ID范围太广。  

## Load Balance
sharding确定了记录存放在哪组数据库，lb根据记录访问方式确定记录存放该组中哪个数据库。  
lb支持的轮转类型：
* RR(round robin)
* WRR(round robin with weight )

## Failover
如果数据库故障，可以设置该数据库故障时间。

