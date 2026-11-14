<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>Five in line solver database</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<style>
  .field-table td {
    width: 32px;
    height: 32px;
  }
</style>
</head>
<body>
<!---
<h3><a href="stat.php">Statistic</a>&nbsp;&nbsp;&nbsp;&nbsp;Participate in solving <a href="solver.zip">Windows</a> <a href="solver.tar.gz">Ubuntu</a></h3>
--->
<?php
include 'cfg.inc';
$x1=-7;
$y1=-7;
$x2=7;
$y2=7;

$neutrals=array();
$solved_wins=array();
$solved_fails=array();
$tree_wins=array();
$tree_fails=array();

class point
{
  var $x;
  var $y;
	function point()
	{
		$this->x=0;
		$this->y=0;
	}
};

define("st_empty",0);
define("st_krestik",1);
define("st_nolik",2);

class step_t extends point
{
	var $step;
	
	function step_t()
	{
		$this->step=st_krestik;
	}
};

class npoint extends point
{
	var $n;
	
	function npoint()
	{
		$this->n=0;
	}
};

class ipoint extends point
{
	var $i;
	
	function npoint()
	{
		$this->i=0;
	}
};

class ppoint extends ipoint
{
	var $proof;
	var $disproof;
	
	function ppoint()
	{
		$this->proof=1;
		$this->disproof=1;
	}
};

function hex2steps($h)
{
	$items_count=(int)(strlen($h)/6);
	
	if($items_count*6!=strlen($h))
		throw new Exception("hex2steps(): invlaid input format");
	
	$ret=array();
	
	for($i=0;$i<$items_count;$i++)
	{
		$a=new step_t;
		$a->x= hexdec(substr($h,$i*6,2));
		if($a->x>127)$a->x=$a->x-256;
		
		$a->y= hexdec(substr($h,$i*6+2,2));
		if($a->y>127)$a->y=$a->y-256;
		
		$a->step=hexdec(substr($h,$i*6+4,2));
		
		array_push($ret,$a);
	}

	return $ret;
}

function step2hex($p)
{
	$x=$p->x;
	$y=$p->y;

	if($x<0)$x=256+$x;
	if($y<0)$y=256+$y;
	
	$h=sprintf("%02X%02X",$x,$y);
	
	if($p->step==st_krestik)$h=$h."01";
	else $h=$h."02";
	
	return $h;
}

function fill_points_array($prefix,$s,&$arr)
{
	$ps=strpos($s,$prefix);
	if($ps===false||$ps!=0)return;
	
	$s=substr($s,strlen($prefix));
	
	$spts=explode(";",$s);
	
	$arr=array();
	
	for($i=0;$i<count($spts);$i++)
	{
		$x=0;
		$y=0;
	  $sp=$spts[$i];

	  if(sscanf($sp,"(%d,%d)",$x,$y)!=2)
	    continue;
		
		$p=new point;
		$p->x=$x;
		$p->y=$y;
		array_push($arr,$p);
	}
}

function fill_steps_array($prefix,$s,&$arr)
{
	$ps=strpos($s,$prefix);
	if($ps===false||$ps!=0)return;
	
	$s=substr($s,strlen($prefix));
	
	$spts=explode(";",$s);
	
	$arr=array();
	
	for($i=0;$i<count($spts);$i++)
	{
		$x=0;
		$y=0;
		$st="";
	  $sp=$spts[$i];
	  if(sscanf($sp,"(%d,%d:%1s)",$x,$y,$st)!=3)
	    continue;
		
		$p=new step_t;
		$p->x=$x;
		$p->y=$y;
		
		if($st=="X"||$st=="x")$p->step=st_krestik;
		else $p->step=st_nolik;
		
		array_push($arr,$p);
	}
}

function fill_npoints_array($prefix,$s,&$arr)
{
	$ps=strpos($s,$prefix);
	if($ps===false||$ps!=0)return;
	
	$s=substr($s,strlen($prefix));
	
	$spts=explode(";",$s);
	
	$arr=array();
	
	for($i=0;$i<count($spts);$i++)
	{
		$x=0;
		$y=0;
		$n=0;
	  $sp=$spts[$i];
	  if(sscanf($sp,"(%d,%d:%d)",$x,$y,$n)!=3)
	    continue;
		
		$p=new point;
		$p->x=$x;
		$p->y=$y;
		$p->n=$n;
		array_push($arr,$p);
	}
}

function fill_ppoints_array($prefix,$s,&$arr)
{
	$ps=strpos($s,$prefix);
	if($ps===false||$ps!=0)return;
	
	$s=substr($s,strlen($prefix));
	
	$spts=explode(";",$s);
	
	$arr=array();
	
	for($i=0;$i<count($spts);$i++)
	{
		$x=0;
		$y=0;
		$score=0;
		$proof=0;
		$disproof=0;

		$sp=$spts[$i];
		if(sscanf($sp,"(%d,%d:%d,%d,%d)",$x,$y,$score,$proof,$disproof)!=5)
			continue;
		
		$p=new ppoint;
		$p->x=$x;
		$p->y=$y;
		$p->i=$score;
		$p->proof=$proof;
		$p->disproof=$disproof;
		array_push($arr,$p);
	}
}

function expand_bound($arr)
{
	global $x1,$y1,$x2,$y2;
	
	for($i=0;$i<count($arr);$i++)
	{
		$a=$arr[$i];
		if($a->x<$x1)$x1=$a->x;
		if($a->x>$x2)$x2=$a->x;
		if($a->y<$y1)$y1=$a->y;
		if($a->y>$y2)$y2=$a->y;
	}
}

function make_point_key_array($arr)
{
	$ret=array();
	
	for($i=0;$i<count($arr);$i++)
	{
		$p=$arr[$i];
		$key=$p->x."_".$p->y;
		$ret[$key]=$p;
	}
	
	return $ret;
}

function step2str($step,$small)
{
	if($step==st_krestik)
	{
		if($small)return "x";
		else return "X";
	}
	else
	{
		if($small)return "o";
		else return "O";
	}
}

if (count($_GET)>0) $HTTP_VARS=$_GET;
else $HTTP_VARS=$_POST;

$hex_state=NULL;
if(array_key_exists("st",$HTTP_VARS))
{
  $hex_state=$HTTP_VARS["st"];
}

if(!$hex_state)
{
	$steps=Array();
	if(isset($OWN_ROOT)) $cmd="view ".escapeshellarg($OWN_ROOT);
	else $cmd="view";
}
else
{
	$steps=hex2steps($hex_state);
	
	$cmd="view ".escapeshellarg($hex_state);
}
$cmd=$EXEC_PATH." ".$DB_PATH." ".$cmd;
exec($cmd,$output);

foreach($output as $s)
{
	if(count($steps)==0)fill_steps_array("key: ",$s,$steps);
	if(!$hex_state)
	{
		$ps=strpos($s,"hex_key: ");
		if(!($ps===false)&&$ps==0)
			$hex_state=substr($s,strlen("hex_key: "));
	}

	fill_ppoints_array("neutrals: ",$s,$neutrals);
	fill_npoints_array("solved wins: ",$s,$solved_wins);
	fill_npoints_array("solved fails: ",$s,$solved_fails);
	fill_npoints_array("tree wins: ",$s,$tree_wins);
	fill_npoints_array("tree fails: ",$s,$tree_fails);
}

expand_bound($steps);
expand_bound($neutrals);
expand_bound($solved_wins);
expand_bound($solved_fails);
expand_bound($tree_wins);
expand_bound($tree_fails);

$min_proof = 0;
$min_disproof = 0;
if(count($neutrals)>0)
{
	$min_proof = $neutrals[0]->proof;
	$min_disproof = $neutrals[0]->disproof;
	foreach($neutrals as $p)
	{
		if($p->proof<$min_proof)
			$min_proof = $p->proof;

		if($p->disproof<$min_disproof)
			$min_disproof = $p->disproof;
	}
}

$key_steps=make_point_key_array($steps);
$neutrals=make_point_key_array($neutrals);
$solved_wins=make_point_key_array($solved_wins);
$solved_fails=make_point_key_array($solved_fails);
$tree_wins=make_point_key_array($tree_wins);
$tree_fails=make_point_key_array($tree_fails);

if((count($steps)%2)==0)$next_step=st_krestik;
else $next_step=st_nolik;

if(array_key_exists("godmode",$HTTP_VARS))
{
	$godmode_val=file_get_contents($DB_PATH."/godmode");
	if(!($godmode_val === false) && $HTTP_VARS["godmode"]!=$godmode_val)
	  $godmode_val=null;
}

$godmode_lnk='';
if($godmode_val)
  $godmode_lnk='&godmode='.$godmode_val;

?>
<table border="1" cellpadding="0" cellspacing="0" class="field-table">
<?php
$win_color='green';
$fail_color='red';

if((count($steps)%2)!=0)
{
	$win_color='red';
	$fail_color='green';
}

for($y=$y1;$y<=$y2;$y++)
{
	echo '<tr>';
	for($x=$x1;$x<=$x2;$x++)
	{
		$skey=$x."_".$y;
		$cnt="";
		$style="";
		
		if(array_key_exists($skey,$key_steps))
		{
			$p=$key_steps[$skey];
			
			$special=false;
			if(($x==0)&&($y==0))$special=true;
			else
			{
				$lp=$steps[count($steps)-1];
				if($lp->x==$p->x&&$lp->y==$p->y)$special=true;
				else if(count($steps)>2)
				{
					$lp=$steps[count($steps)-2];
					if($lp->x==$p->x&&$lp->y==$p->y)$special=true;
				}
			}
			
			$style="background-color:rgb(200,235,240);";
			$cnt=step2str($p->step,$special);
		}
		else if(array_key_exists($skey,$neutrals))
		{
			$p=$neutrals[$skey];
			
			$o=new step_t;
			$o->x=$p->x;
			$o->y=$p->y;
			$o->step=$next_step;
		
			$h=$hex_state.step2hex($o);
			$title=sprintf("score=%d\nproof=%d\ndisproof=%d",$p->i,$p->proof,$p->disproof);
			
			$cnt='<a href="'.$OWN_PATH.'?st='.$h;
			$cnt=$cnt.$godmode_lnk;
			$cnt=$cnt.'" title="'.$title.'">'.step2str($next_step,true).'</a>';
			
			$c = 255-min(abs($p->i)*4,255);
			
			if($p->i<1.0)
			{
				$style="background-color:rgb(255,$c,$c)";
			}
			else if($p->i>1.0)
			{
				$style="background-color:rgb($c,255,$c)";
			}
			
			$solving = false;
			$top_most = false;
			
			if($p->proof >=100000 || $p->disproof >=100000) $solving = true;
			
			if($next_step == st_krestik)
			{
				if($p->proof == $min_proof) $top_most = true;
			}
			else
			{
				if($p->disproof == $min_disproof) $top_most = true;
			}
			
			if($solving)
				$style=$style.";border: 2px solid cyan;border-radius:16px";
			else if($top_most)
				$style=$style.";border: 2px solid magenta;border-radius:16px";
		}
		else if(array_key_exists($skey,$solved_wins))
		{
			$p=$solved_wins[$skey];
			$cnt=step2str($next_step,true).$p->n;
			$style="background-color:rgb(220,220,220);color:".$win_color;
		}
		else if(array_key_exists($skey,$solved_fails))
		{
			$p=$solved_fails[$skey];
			$cnt=step2str($next_step,true).$p->n;
			$style="background-color:rgb(220,220,220);color:".$fail_color;
		}
		else if(array_key_exists($skey,$tree_wins))
		{
			$p=$tree_wins[$skey];
			
			$o=new step_t;
			$o->x=$p->x;
			$o->y=$p->y;
			$o->step=$next_step;
			
			$h=$hex_state.step2hex($o);
			$cnt='<a href="'.$OWN_PATH.'?st='.$h.$godmode_lnk.'" style="color:'.$win_color.';">'.step2str($next_step,false).$p->n.'</a>';
			$style="background-color:rgb(220,220,220);";
		}
		else if(array_key_exists($skey,$tree_fails))
		{
			$p=$tree_fails[$skey];
			
			$o=new step_t;
			$o->x=$p->x;
			$o->y=$p->y;
			$o->step=$next_step;
		
			$h=$hex_state.step2hex($o);
			$cnt='<a href="'.$OWN_PATH.'?st='.$h.$godmode_lnk.'" style="color:'.$fail_color.';">'.step2str($next_step,false).$p->n.'</a>';
			$style="background-color:rgb(220,220,220);";
		}

		echo '<td width="32" align="center" valign="middle" style="'.$style.'">';
		echo $cnt;
		echo "</td>";
	}
	echo "</tr>\r\n";
}
?>
</table>

<?php
if($godmode_val)
{
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
	
	if (in_array($hex_state, $lst))
		echo '<a href="'.$OWN_PATH.'mark_state.php?rm=1&st='.$hex_state.$godmode_lnk.'">Unmark state</a>';
	else
		echo '<a href="'.$OWN_PATH.'mark_state.php?st='.$hex_state.$godmode_lnk.'">Mark state</a>';
	
	echo '&nbsp;&nbsp;&nbsp;&nbsp;';
	echo '<a href="'.$OWN_PATH.'explore_state.php?st='.$hex_state.$godmode_lnk.'">Explore state</a>';
}
?>
<?php
	$h = '';
	echo '<p>';
	for($i=0;$i<count($steps);$i++)
	{
		$p = $steps[$i];
		
		if ($i != 0)
			echo ';';

		$h=$h.step2hex($p);
		
		if($i+1<count($steps))
		{
			echo '<a href="'.$OWN_PATH.'?st='.$h;
			echo $godmode_lnk;
			echo '">';
		}
		
		echo '('.$p->x.','.$p->y.':'.step2str($p->step,false).')';

		if($i+1<count($steps))
			echo '</a>';
	}
	echo '</p>';
?>
<pre>
<?php
$str = '';
foreach($output as $s)
  $str = $str.$s."\r\n";
echo $str;
?>
</pre>
</body>
</html>
