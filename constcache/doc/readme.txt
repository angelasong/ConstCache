һ. ConstCache���˼�룺
  1. ����һ��cд��php extension
  2. �ӿ���memcache��ͬ
  3. �洢������С������key��ĳ�����ĳ����¼
  4. �洢���ù����ڴ�ķ�ʽ
  5. ������Զ��ɾ������ȫ��������������������Ż���


��. ConstCache���ã�
  extension=constcache.so

  ; �ź�����KEY(�������Զ���)
  constcache.ksem=4567

  ; �����ڴ��KEY(�������Զ���)
  constcache.kmem=4567

  ; ����Ĺ����ڴ��С����λΪM
  constcache.memsize=32

  ; hashλ������СΪ2^x��������ò�Ҫ����20
  ; ��������Խ�󣬲��ҵ�Խ�죬���ķѵ��ڴ�Խ��
  constcache.hashpower=16



��. ConstCache�ӿ�API��
   ����Memcache�Ľӿڣ��������Щ����ķ���

   �÷����£�
   $cc = new ConstCache();
   $cc->add("k", 111);
   echo $cc->get("k");

   Ŀǰ֧�ֵķ������£�
   1. bool  ConstCache::add( string key, mixed var [, int flag [, int expire]])
      ������ݣ�������������û�á�

   2. mixed ConstCache::get(string key)
      ��ȡ����

   3. bool ConstCache::flush()
      ���ȫ������

   4. bool ConstCache::destroy()
      ɾ���Ѵ����õĹ����ڴ�
      ������������õĶ��壬һ��Ҫ��ɾ�����������ò���������
      Ҳ������unix����ipcrm��ɾ������ipcs���Բ鿴ϵͳ�еĹ����ڴ�����

   5. bool ConstCache::debug(string filepath, [, bool detail])
      ��filepathָ�����ļ����ӡ������Ϣ��detail��ʾ�Ƿ��ӡ��ϸ��Ϣ

   6. bool ConstCache::dump(string filepath)
      ��filepathָ�����ļ����ӡ��ǰ����������ݡ�

   7. array ConstCache::stat()
      ������debug, ���ص�����Ϣ����һ�����顣

