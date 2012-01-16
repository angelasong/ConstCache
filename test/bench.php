<?php
ini_set('memory_limit', '2048M');
ini_set('max_execution_time', 0);

system('rm -rf /tmp/bench.tmp*.log');

$options = options_parse();
generate_keys($options);
if ($options['method']=='set') {
	$ret = clear_contents($options);
	if (!$ret) {
		echo "failed to clear contents\n";
		return;
	}
}
if ($options['initial']) {
	$ret = load_contents($options);
	if (!$ret) {
		echo "failed to load contents\n";
		return;
	}
}

ob_start();
$val = get_fake_value($options['length']);
$stime = time()+3+(int)(($options['concurrency']*$options['number'])/100000)*$options['amend'];

for ($i=0; $i<$options['concurrency']; $i++) {
	$pid = pcntl_fork();
	if ($pid) {
		// parent process
	} else {
		if ($options['type']=='memcached') {
			error_log(date('Y-m-d H:i:s')."\tprocess #".$i." begin running...\n", 3, '/tmp/bench.tmp.run.log');
			$child_mc = new Memcached();
			$ret = $child_mc->addServer($options['ip'], $options['port']);
			if (!$ret) exit;
			
			$fp = fopen('/tmp/bench.tmp.'.$i.'.log', 'r');
			$keys = array();
			while ($buf = fgets($fp, 4096)) $keys[] = trim($buf);
			fclose($fp);
			error_log(date('Y-m-d H:i:s')."\tprocess #".$i." after loading...\n", 3, '/tmp/bench.tmp.run.log');
			
			$curtime = time();
			if ($stime<=$curtime) exit;
			sleep($stime - $curtime);
			
			error_log(date('Y-m-d H:i:s')."\tprocess #".$i." start operating...\n", 3, '/tmp/bench.tmp.run.log');
			list($busec, $bsec) = explode(" ", microtime());
			if ($options['method']=='get') {
				foreach($keys as $key) {
					$val = $child_mc->get($key);
				}
			}
			if ($options['method']=='set') {
				foreach($keys as $key) {
					$child_mc->set($key, $val, 86400);
				}
			}
			list($eusec, $esec) = explode(" ", microtime());
			
			$tret = sprintf("%.3f", ((float)$esec + (float)$eusec) - ((float)$bsec + (float)$busec));
			error_log($tret."\n", 3, '/tmp/bench.tmp.ret.log');
			
			error_log(date('Y-m-d H:i:s')."\tprocess #".$i." finished\n", 3, '/tmp/bench.tmp.run.log');
		}
		if ($options['type']=='constcache') {
			error_log(date('Y-m-d H:i:s')."\tprocess #".$i." begin running...\n", 3, '/tmp/bench.tmp.run.log');
			$child_cc = new ConstCache();
			
			$fp = fopen('/tmp/bench.tmp.'.$i.'.log', 'r');
			$keys = array();
			while ($buf = fgets($fp, 4096)) $keys[] = trim($buf);
			fclose($fp);
			error_log(date('Y-m-d H:i:s')."\tprocess #".$i." after loading...\n", 3, '/tmp/bench.tmp.run.log');
			
			$curtime = time();
			if ($stime<=$curtime) return;
			sleep($stime - $curtime);
			
			error_log(date('Y-m-d H:i:s')."\tprocess #".$i." start operating...\n", 3, '/tmp/bench.tmp.run.log');
			list($busec, $bsec) = explode(" ", microtime());
			if ($options['method']=='get') {
				foreach($keys as $key) {
					$val = $child_cc->get($key);
				}
			}
			if ($options['method']=='set') {
				foreach($keys as $key) {
					$child_cc->add($key, $val, 0, 86400);
				}
			}
			list($eusec, $esec) = explode(" ", microtime());
			
			$tret = sprintf("%.3f", ((float)$esec + (float)$eusec) - ((float)$bsec + (float)$busec));
			error_log($tret."\n", 3, '/tmp/bench.tmp.ret.log');
			
			error_log(date('Y-m-d H:i:s')."\tprocess #".$i." finished\n", 3, '/tmp/bench.tmp.run.log');
		}
		exit;
	}
}
while (1) {
	$ret = system('ps ax|grep bench.php | grep -v grep | wc -l');
	if ($ret==1) break;
	sleep(2);
}

if (!file_exists('/tmp/bench.tmp.ret.log')) {
	echo "all process failed\n";
	return;
}
$ret = system('awk \'{num++;sum+=$1} END {print num"\t"sum}\' /tmp/bench.tmp.ret.log');
ob_end_clean();
$t = explode("\t", trim($ret));
if ($t[0]!=$options['concurrency']) {
	echo "some process failed\n";
	return;
}
echo "the time used is ".$t[1]."\n";
echo "params is type=".$options['type'].";method=".$options['method'].";concurrency=".$options['concurrency'].";number=".$options['number'].";length=".$options['length']."\n";
return;

function clear_contents($options)
{
	if ($options['type']=='constcache') {
		$cc = new ConstCache();
		return $cc->flush();
	}
	if ($options['type']=='memcached') {
		$mc = new Memcached();
		$ret = $mc->addServer($options['ip'], $options['port']);
		if (!$ret) return false;
		return $mc->flush();
	}
	return false;
}
function load_contents($options)
{
	for ($i=0; $i<$options['concurrency']; $i++) {
		$val = get_fake_value($options['length']);
		$fp = fopen('/tmp/bench.tmp.'.$i.'.log', 'r');
		if ($options['type']=='constcache') {
			$cc = new ConstCache();
		}
		if ($options['type']=='memcached') {
			$mc = new Memcached();
			$ret = $mc->addServer($options['ip'], $options['port']);
			if (!$ret) return false;
		}
		while ($buf = fgets($fp, 4096)) {
			if ($options['type']=='constcache') {
				$ret = $cc->add(trim($buf), $val);
				if (!$ret) return false;
			}
			if ($options['type']=='memcached') {
				$ret = $mc->set(trim($buf), $val);
				if (!$ret) return false;
			}
		}
		fclose($fp);
	}
	return true;
}
function generate_keys($options)
{
	$keys = array();
	for ($i=0; $i<$options['concurrency']; $i++) {
		$keys[$i] = get_fake_keys($i, $options['number']);
		$fp = fopen('/tmp/bench.tmp.'.$i.'.log', 'w');
		foreach($keys[$i] as $key) fputs($fp, $key."\n");
		fclose($fp);
	}
	unset($keys);
}
function get_fake_keys($prefix, $number)
{
	$keys = array();
	for ($i=0; $i<$number; $i++) {
		$keys[] = sprintf("%03s_%'#85s_%010s", $prefix, '', $i);
	}
	return $keys;
}
function get_fake_value($length)
{
	return sprintf("%'#".$length."s", '');
}

function options_parse()
{
	$ops = getopt("t:c:m:s:n:l:ia:");
	$options = array();
	if (!isset($ops['t'])) show_usage();
	if ($ops['t']!='memcached' && $ops['t']!='constcache') {
		echo "type should set to memcached/constcache\n";
		show_usage();
	}
	$options['server'] = 0;
	if ($ops['t']=='memcached') {
		if (!isset($ops['s'])) {
			echo "server should set when use memcached\n";
			show_usage();
		}
		$t = explode(":",$ops['s']);
		if (count($t)!=2) {
			echo "server format is ip:port\n";
			show_usage();
		}
		$options['ip'] = $t[0];
		$options['port'] = $t[1];
		$options['server'] = $t[0].':'.$t[1];
	}
	$options['type'] = $ops['t'];
	if (!isset($ops['c'])) $ops['c'] = 10;
	if (!is_numeric($ops['c'])) $ops['c'] = 10;
	if ($ops['c']>999) $ops['c'] = 999;
	$options['concurrency'] = $ops['c'];
	if (!isset($ops['m'])) $ops['m'] = 'set';
	if ($ops['m']!='get' && $ops['m']!='set') {
		echo "method should set to get/set\n";
		show_usage();
	}
	$options['method'] = $ops['m'];
	if (!isset($ops['n'])) $ops['n'] = 10000;
	if (!is_numeric($ops['n'])) $ops['n'] = 10000;
	if ($ops['n']>1000000) $ops['n'] = 1000000;
	$options['number'] = $ops['n'];
	if ($options['number']*$options['concurrency']>1000000) {
		echo "please set smaller number or concurrency\n";
		return;
	}
	if (!isset($ops['l'])) $ops['l'] = 1000;
	if (!is_numeric($ops['l'])) $ops['l'] = 1000;
	$options['length'] = $ops['l'];
	$options['initial'] = false;
	if (isset($ops['i'])) $options['initial'] = true;
	$options['amend'] = 3;
	if (isset($ops['a']) && is_numeric($ops['a'])) $options['amend'] = $ops['a'];
	if ($options['amend']>20) $options['amend']=20;
	return $options;
}

function show_usage()
{
	$str = "Usage: bench.php -ttype -cconcurrency -mmethod -sserver -nnumber -llength -i\n";
	echo $str;
	exit;
}