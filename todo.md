
每一条命令一个包括一个id，方便后面应答。   
历史消息记录存储在本地mysql，未应答的数据，或者离线未接受的数据放在服务器。


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



###  desc  historys;   
type为消息类型， message_id为消息id，messagebody为消息体，

| Field       | Type             | Null | Key | Default | Extra          |
|-------------|------------------|------|-----|---------|----------------|
| id          | int unsigned     | NO   | PRI | <null>  | auto_increment |
| sender      | varchar(40)      | NO   |     | <null>  |                |
| receiver    | varchar(40)      | NO   | MUL | <null>  |                |
| type        | tinyint unsigned | NO   |     | <null>  |                |
| message_id  | int unsigned     | NO   |     | <null>  |                |
| messagebody | varchar(230)     | YES  |     | <null>  |                |
| time        | datetime         | NO   |     | <null>  |                |

