<?php
$cc = new ConstCache();
//$cc->destroy();
$cc->flush();
$cc->add('1', array("123", 1));
print_r($cc->get('1'));
echo "\n";
$cc->debug('/tmp/ccdebug.log');
$cc->dump('/tmp/ccdump.log');
?>
