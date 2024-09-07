# tuchuang
# 图床项目
# 需要开启mysql，redis，nginx功能，并在tc_http_server.conf中修改配置。
# 需要下载FastDFS，下面是具体操作

# 安装 libfastcommon
git clone https://gitee.com/fastdfs100/libfastcommon.git
cd libfastcommon
git checkout V1.0.50
./make.sh
sudo ./make.sh install

# 安装 FastDFS，注意安装地址，后面要用
git clone https://gitee.com/fastdfs100/fastdfs.git
cd fastdfs
git checkout V6.07
./make.sh 
sudo ./make.sh install

# 配置 Tracker
# 创建 Tracker 的存储日志和数据的根目录
sudo mkdir -p /home/fastdfs/tracker
cd /etc/fdfs
sudo cp tracker.conf.sample tracker.conf
# 配置 tracker.conf
sudo vim tracker.conf
# 启用配置文件（默认为 false，表示启用配置文件）
disabled=false
# Tracker 服务端口（默认为 22122）
port=22122
# 存储日志和数据的根目录
base_path=/home/fastdfs/tracker

# 配置 Storage
# 创建 Storage 的存储日志和数据的根目录
sudo mkdir -p /home/fastdfs/storage
cd /etc/fdfs
sudo cp storage.conf.sample storage.conf
# 配置 storage.conf
sudo vim storage.conf
# 数据和日志文件存储根目录
base_path=/home/fastdfs/storage
# 存储路径，访问时路径为 M00
# store_path1 则为 M01，以此递增到 M99（如果配置了多个存储目录的话，这里只指定 1 个）
# store_path0 M00 
store_path0=/home/fastdfs/storage
# Tracker 服务器 IP 地址和端口，单机搭建时也不要写 127.0.0.1
# tracker_server 可以多次出现，如果有多个，则配置多个
tracker_server=192.168.1.27:22122

# 启动 Tracker 服务
# 其它操作则把 start 改为 stop、restart、reload、status 即可。Storage 服务相同
sudo /etc/init.d/fdfs_trackerd start
# 启动 Storage 服务
sudo /etc/init.d/fdfs_storaged start

# 配置fdfs
cd ~/tc/fastdfs-nginx-module/src
sudo cp mod_fastdfs.conf /etc/fdfs/

 # 为fastdfs源码路径
cd /home/zjx/tc/fastdfs       
sudo cp conf/http.conf /etc/fdfs/
sudo cp conf/mime.types /etc/fdfs/
sudo mkdir  -p /home/fastdfs/mod_fastdfs
base_path =/home/fastdfs/mod_fastdfs 
sudo vim /etc/fdfs/mod_fastdfs.conf
# Tracker 服务器IP和端口修改
tracker_server=192.168.1.27:22122
# url 中是否包含 group 名称，改为 true，包含 group
url_have_group_name = true
# store_path0的路径必须和storage.conf的配置一致
store_path0=/home/fastdfs/storage
# 其它的一般默认即可，例如
group_name=group1 
storage_server_port=23000 
store_path_count=1

# nginx配置
sudo vim /usr/local/nginx/conf/nginx.conf
# 增加
location ~/group([0-9])/M([0-9])([0-9]) {     
        ngx_fastdfs_module;
}
停止：sudo /usr/local/nginx/sbin/nginx -s stop
启动：sudo /usr/local/nginx/sbin/nginx
