####Design idea  

1. Make an extension for PHP by C language  
2. The API is same as memcache. 
3. It uses the way of sharing memory to store data.  
4. In order to being beneficial to performance optimize, the data will never be deleted (can be clear all).  
5. It read data at a very fast rate.

####Configuration  

    extension=constcache.so  
 ; The key of semaphore(integer,user-defined)  

    constcache.ksem=4567  
 ; The key of shared memory(integer,user-defined)  

    constcache.kmem=4567  
 ; The size of allocated shared memory, measured in M  

    constcache.memsize=32  
 ; The size of hash is 2^x, you'd better not exceed 20  
 ; The larger size you sets, the faster it seeks, but the cost of memory is more  

    constcache.hashpower=16

####The API of ConstCache  

It imitates the API of memcache, and adds some additional methods. Usages are as follows:  

    $cc=new ConstCache();
    $cc->add("k",111);
    echo $cc->get("k");

The methods supported now are as follows:  

    1. bool ConstCache::add(string key,mixed var[,int flag[,int expire]])  
 ;Add data, the last two variables are useless  

    2. mixed ConstCache::get(string key)
 ;Get data  

    3. bool ConstCache::flush()
 ;Empty all the data  

    4. bool ConstCache::destory()
 ;Delete the created shared memory  
 ;When you change the configuration definition, the new configuration won't work unless you delete the created shared memory  
 ;You can also use the Unix command 'ipcrm' to delete memory and use 'ipcs' to check the condition of shared memory in system  

    5. bool ConstCache::debug(string filepath,[,bool detail])
 ;Print the debug information to the filepath specified files, 'detail' means whether to print the detail information  
 
    6. bool ConstCache::dump(string filepath)
 ;Print the content of the current cache to the filepath specified files  

    7. array ConstCache::stat()
 ;Being similar to debug, return the debug information as an array  


