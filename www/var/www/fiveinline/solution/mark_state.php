<?php
include 'cfg.inc';

if (count($_GET)>0) $HTTP_VARS=$_GET;
else $HTTP_VARS=$_POST;

$access_verified=false;
if(array_key_exists("godmode",$HTTP_VARS))
{
	$godmode_val=file_get_contents($DB_PATH."/godmode");
	if(!($godmode_val === false) && $HTTP_VARS["godmode"]==$godmode_val)
	{
		$access_verified=true;
	}
}

if(!$access_verified)
{
	echo "Access denied<br>";
	exit(1);
}

$st=$HTTP_VARS["st"];

$file_name=$DB_PATH."/marked.txt";
$lst=file_get_contents($file_name);

if($lst === false)
  $lst=array();
else
{
	$lst=trim($lst);
	if($lst === '')
		$lst=array();
	else
		$lst=explode("\n",$lst);
}

if(!$st)
{
	foreach($lst as $v)
	{
		echo '<a href="'.$OWN_PATH.'mark_state.php?rm=1&st='.$v.'&godmode='.$godmode_val.'">X</a>&nbsp;&nbsp;&nbsp;&nbsp;<a href="'.$OWN_PATH.'index.php?st='.$v.'&godmode='.$godmode_val.'">'.$v.'</a><br>';
	}
	exit(0);
}

foreach ($lst as $k => $v) 
{
  if($v==$st)
    unset($lst[$k]);
}

if(!array_key_exists("rm",$HTTP_VARS) || !$HTTP_VARS["rm"] )
{
	while(count($lst)>$MAX_MARKED)
	  array_pop($lst);
	  
	array_unshift($lst,$st);
}

if(count($lst)==0) $content = "";
else $content = trim(implode("\n",$lst));

file_put_contents($file_name,$content);

header('Location: '.$OWN_PATH.'mark_state.php?godmode='.$godmode_val);

?>
