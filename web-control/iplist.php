<?php
    $cmd = "";
    
    if(isset($_GET['cmd'])) $cmd = strtolower($_GET['cmd']);
    
    switch($cmd) {
	case 'doadd': {
	
	    check_input();
	    clear_div();
	    
	    if(!$err_input) {
		$sql = 'insert into iplists(name, startip, endip) values("'.convert_quot($ipname).'", INET_ATON("'.$ipstart.'"), INET_ATON("'.$ipend.'"))';
		$dbh->query($sql);
	    }
	}; break;
	case 'edit': {
	    $err_edit_notfound = 1;
	    if(isset($_GET['id'])) {
		$id = (int)$_GET['id'];
		$sql = "select id, name, INET_NTOA(startip) as ipstart, INET_NTOA(endip) as ipend from iplists where id=$id;";
		$sth = $dbh->query($sql);
		if($fa = $dbh->fetch_array($sth)) {
		    $err_edit_notfound = 0;
		    show_form("edit", "Редактирование списка IP: ".$fa[1], $fa[0], $fa[1], $fa[2], $fa[3]);
		}
	    } elseif(isset($_POST['id'])) {
		$id = (int)$_POST['id'];
		
		check_input();
		if(!$err_input) {
		    $sql = 'update iplists set name="'.convert_quot($ipname).'", startip=INET_ATON("'.$ipstart.'"), endip=INET_ATON("'.$ipend.'") where id='.$id;
		    if($dbh->query($sql)) {
			$err_edit_notfound = 0;
			show_message("Обновление списка IP", 'Операция выполнена успешно<br /><a href="index.php?pos=iplist">Вернуться</a>');
		    };
		}
	    };
	    
	    if($err_edit_notfound) {
		show_message("Обновление списка IP", 'Во время операции обновления произошла ошибка<br /><a href="index.php?pos=iplist">Вернуться</a>');
	    };
	}; return;
	case 'del': {
	    $err_delete = 1;
	    if(isset($_GET['id'])) {
		$id = (int)$_GET['id'];
		
		// сначала проверим, не назначен ли это лист какому-либо интерфейсу
		$sql = "select int_mask from interfaces where iplist=".$id;
		if($fa=$dbh->fetch_array($dbh->query($sql))) {
		    show_error("Удаление не возможно:<br /> Удаляемый список IP назначен интерфейсной маске '".$fa[0]."'");
		} else {
		    $sql = 'delete from iplists where id='.$id;
		    if($dbh->query($sql)) {
			$err_delete = 0;
			show_message("Удаление списка IP", 'Операция выполнена успешно<br /><a href="index.php?pos=iplist">Вернуться</a>');
		    }
		}
	    };
	    if($err_delete) {
		show_message("Удаление списка IP", 'Во время операции удаления произошла ошибка<br /><a href="index.php?pos=iplist">Вернуться</a>');
	    };
	}; return;
    }
?>
<div class="clr"></div>
<div class="data">
<table class="tabledata">
<thead>
    <tr>
	<th>#</th>
	<th>Имя листа</th>
	<th>Начальный IP</th>
	<th>Конечный IP</th>
	<th colspan="2">Действия</th>
    </tr>
</thead>
<tbody>
<?php
    $sql = "select id, name, INET_NTOA(startip) as ipstart, INET_NTOA(endip) as ipend from iplists;";
    $sth = $dbh->query($sql);
    
    while($fa=$dbh->fetch_array($sth)) {
	echo '<tr>
  <td>'.$fa[0].'</td>
  <td>'.$fa[1].'</td>
  <td>'.$fa[2].'</td>
  <td>'.$fa[3].'</td>
  <td><a href="index.php?pos=iplist&cmd=edit&id='.$fa[0].'">Изменить</a></td>
  <td><a href="index.php?pos=iplist&cmd=del&id='.$fa[0].'">Удалить</a></td>
</tr>';
    };
?>
</tbody>
</table>
</div>
<div class="clr"></div>

<?php
show_form("doadd", "Добавить список IP");
function show_form($cmd, $legend, $id=0, $name="", $fromip="", $toip="") {
    echo '<div class="clr"></div><div><form method="post" action="index.php?pos=iplist&cmd='.$cmd.'"><fieldset>
    <legend>'.$legend.'</legend>
    <input type="hidden" name="id" value="'.$id.'" />
    <table border="0">
	<tr class="field"><td>Имя списка</td><td><input type="text" name="name" value="'.$name.'" /></td></tr>
	<tr class="field"><td>Начальный IP</td><td><input type="text" name="fromip" value="'.$fromip.'" /></td></tr>
	<tr class="field"><td>Конечный IP</td><td><input type="text" name="endip" value="'.$toip.'" /></td></tr>
	<tr class="field fieldctrl"><td colspan="2"><input type="submit" value="Отправить" /></td></tr>
    </table>
</fieldset></form><div class="clr"></div></div>';
}

function check_input() {
	    global $ipname, $ipstart, $ipend;
	    
	    $ipname = $_POST['name'];
	    $ipstart = $_POST['fromip'];
	    $ipend = $_POST['endip'];
	    
	    check_data($ipname, "не указано имя списка");
	    check_ipinput($ipstart, "введён неправильный начальный IP");
	    check_ipinput($ipend, "введён неправильный конечный IP");
}
?>