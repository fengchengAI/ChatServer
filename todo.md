## 账户好友列表   
mysql添加friend数据table，记录好友关系，如添加时间。
添加room字段，记录群聊成员。
每一条命令一个包括一个id，方便后面应答。   

###  desc room;   

name表示这个群聊名称，members表示群的成员，成员之间用‘，’隔开，最多支持

| Field | Type         | Null | Key | Default | Extra          |
|-------------------|---------------|------|-----|---------|----------------|
| id       | int unsigned  | NO   | PRI | <null>  | auto_increment |
| name              | char(20)      | NO   |     | <null>  |                |
| members           | varchar(4600) | NO   |     | <null>  |                |
| registration_date | datetime      | NO   |     | <null>  |                |



### desc roomship;   

name是用户名，其中该用户有多个群聊，群聊名称在roommembers中

| Field       | Type          | Null | Key | Default | Extra          |
|-------------|---------------|------|-----|---------|----------------|
| id          | int unsigned  | NO   | PRI | <null>  | auto_increment |
| name        | varchar(20)   | NO   |     | <null>  |                |
| roommembers | varchar(4600) | NO   |     | <null>  |                |



表示name1与name2是好友关系，一般

### desc  friendship;

| Field             | Type         | Null | Key | Default | Extra          |
|-------------------|--------------|------|-----|---------|----------------|
| id                | int unsigned | NO   | PRI | <null>  | auto_increment |
| name1             | char(20)     | NO   |     | <null>  |                |
| name2             | char(20)     | NO   |     | <null>  |                |
| registration_date | datetime     | NO   |     | <null>  |                |



status表示当前用户状态，一个重要的信息是一个用户不能同时在多个设备上登录。（每次登录和退出都会对status修改）

 ### desc user;

| Field      | Type         | Null | Key | Default | Extra          |
|------------|--------------|------|-----|---------|----------------|
| id         | int unsigned | NO   | PRI | <null>  | auto_increment |
| name       | char(20)     | NO   | MUL | <null>  |                |
| password   | varchar(20)  | NO   |     | <null>  |                |
| gender     | tinyint(1)   | NO   |     | <null>  |                |
| last_lojin | datetime     | NO   |     | <null>  |                |
| status     | tinyint      | NO   |     | <null>  |                |