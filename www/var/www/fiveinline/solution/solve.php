<?php
include 'cfg.inc';

$HTTP_VARS=$_GET;

$cmd=$HTTP_VARS["cmd"];

chdir(dirname($EXEC_PATH));

function write_result_file($file_name)
{
	$content = file_get_contents('php://input');
	file_put_contents($file_name, $content);
}

function get_job_from_list()
{
	global $EXEC_PATH,$DB_PATH,$USE_ANT;
	
	if($USE_ANT)
		$cmd='get_ant_job';
	else 
		$cmd='get_job';

	$cmd=$EXEC_PATH." ".$DB_PATH." ".escapeshellarg($cmd);
	$file_name=$DB_PATH."/marked.txt";
	$lst=file_get_contents($file_name);

	if($lst === false)
	  return false;
  

	$lst=explode("\n",trim($lst));
	while(count($lst)>0)
	{
		$k=mt_rand(0,count($lst)-1);
		
		if(!empty($lst[$k]))
		{
			$v=$lst[$k];
			$cc=$cmd." ".escapeshellarg($v);

			$output=array();
			exec($cc,$output);
			
			$output=implode("\n",$output);
			
			if(strpos($output,"no job anymore") === false)
			{
				echo $output;
				return true;
			}
		}
		
		unset($lst[$k]);
		$lst = array_values($lst);
		
		if(count($lst)==0) $content = "";
		else $content = trim(implode("\n",$lst));
		
		file_put_contents($file_name, $content );
	}
	return false;
}


if($cmd=='get_job')
{
	if($USE_ANT)
		$cmd='get_ant_job';
	
	$cmd=$EXEC_PATH." ".$DB_PATH." ".escapeshellarg($cmd);
	
	if(array_key_exists("root",$HTTP_VARS))
	  $root=$HTTP_VARS["root"];
	else if(isset($OWN_ROOT))
	  $root=$OWN_ROOT;
	  
    if(!$root)
	{
	  if(get_job_from_list())
	    exit(0);
	}
	
    if($root)
	  $cmd=$cmd." ".escapeshellarg($root);
	  
	passthru($cmd,$r);
}
else if($cmd=='save_job')
{
	if($_SERVER['REQUEST_METHOD'] != 'POST')
	{
		echo 'expect POST<br>';
		exit(1);
	}
	
	$file_name = tempnam(sys_get_temp_dir(), 'save_job');
	
	write_result_file($file_name);
	
	passthru($EXEC_PATH." ".$DB_PATH
		." ".escapeshellarg($cmd)
		." ".escapeshellarg($file_name)
		,$r);
	
	unlink($file_name);
}
else
{
  echo 'five in line solve database<br>';
}

?>
