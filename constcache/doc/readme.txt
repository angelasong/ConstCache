一. ConstCache设计思想：
  1. 制作一个c写的php extension
  2. 接口与memcache相同
  3. 存储的粒度小，比如key是某个表的某条记录
  4. 存储采用共享内存的方式
  5. 数据永远不删（可以全部清除），以利于性能优化。


二. ConstCache配置：
  extension=constcache.so

  ; 信号量的KEY(整数，自定义)
  constcache.ksem=4567

  ; 共享内存的KEY(整数，自定义)
  constcache.kmem=4567

  ; 分配的共享内存大小，单位为M
  constcache.memsize=32

  ; hash位数，大小为2^x，所以最好不要超过20
  ; 这个数设的越大，查找的越快，但耗费的内存越多
  constcache.hashpower=16



三. ConstCache接口API：
   仿照Memcache的接口，并添加了些额外的方法

   用法如下：
   $cc = new ConstCache();
   $cc->add("k", 111);
   echo $cc->get("k");

   目前支持的方法如下：
   1. bool  ConstCache::add( string key, mixed var [, int flag [, int expire]])
      添加数据，后面两个变量没用。

   2. mixed ConstCache::get(string key)
      获取数据

   3. bool ConstCache::flush()
      清空全部缓存

   4. bool ConstCache::destroy()
      删除已创建好的共享内存
      如果更改了配置的定义，一定要先删除，否则配置不会起作用
      也可以用unix命令ipcrm来删除，用ipcs可以查看系统中的共享内存的情况

   5. bool ConstCache::debug(string filepath, [, bool detail])
      向filepath指定的文件里打印调试信息，detail表示是否打印详细信息

   6. bool ConstCache::dump(string filepath)
      向filepath指定的文件里打印当前缓存里的内容。

   7. array ConstCache::stat()
      类似于debug, 返回调试信息，是一个数组。

