<?php
    $cmd = "";
    
    if(isset($_GET['cmd'])) $cmd = strtolower($_GET['cmd']);
    
    switch($cmd) {
	case 'check': {
	    $interface = isset($_POST['inter'])?convert_quot($_POST['inter']):'';
	    
	    $sql = "select interfaces.id, interfaces.int_mask, interfaces.iplist, interfaces.options, interfaces.static, interfaces.route, INET_NTOA(iplists.startip), INET_NTOA(iplists.endip), interfaces.ptp
		    from interfaces,iplists
		    where '$interface' regexp interfaces.int_mask and iplists.id=interfaces.iplist order by interfaces.prio desc limit 0,1";
	
	    if($fa=$dbh->fetch_array($dbh->query($sql))) {
		echo '<div class="data">';
		echo '<table border="0">
    <tr><td>Запрос адреса с</td><td>'.$interface.'</td></tr>
    <tr><td>Интерфейсная маска</td><td>'.$fa[1].'</td></tr>
    <tr><td>Статичный адрес</td><td>'.($fa[4]==1?'Да':'Нет').'</td></tr>
    <tr><td>Прописывание маршрута</td><td>'.($fa[5]==1?'Да':'Нет').'</td></tr>
    <tr><td>За интерфейсом 1 абонент</td><td>'.($fa[8]==1?'Да':'Нет').'</td></tr>
    <tr><td>Будет выдан адрес из диапазона</td><td>'.$fa[6].'-'.$fa[7].'</td></tr>
    <tr><td colspan="2">Будут переданы следующие опции:</td></tr>';
    
		$sql = "select code,value,type from options_template_data where template=".$fa[3];
		$sth = $dbh->query($sql);
		while($fa=$dbh->fetch_array($sth)) {
		    echo '<tr><td>'.(isset($dhcp_options[$fa[0]])?$dhcp_options[$fa[0]]:'UNKNOWN_'.$fa[0]).'</td><td>';
		    if($fa[2]==1) {
			echo sprintf("0x%02x", $fa[1]);
		    } elseif($fa[2]==5) {
			echo sprintf("0x%08x", $fa[1]);
		    } else {
			echo $fa[1];
		    }
		    echo '</td></tr>';
		}
    
		echo '</table></div>';
	    }
	    
	    clear_div();
	    check_form($interface);
	}; return;
	case 'doadd': {

	    // global $intmask, $iplist, $opts;
	
	    check_input();
	    clear_div();
	    
	    if(!$err_input) {
		$static = isset($_POST['static'])?(int)$_POST['static']:0;
		$routed = isset($_POST['routed'])?(int)$_POST['routed']:1;
		$prio = isset($_POST['prio'])?(int)$_POST['prio']:0;
		$note = isset($_POST['note'])?convert_quot($_POST['note']):'';
		$ptp = isset($_POST['ptp'])?(int)$_POST['ptp']:1;
		
		$sql = 'insert into interfaces(note, int_mask, iplist, options, static, route, prio) values("'.$note.'", "'.convert_quot($intmask).'", '.$iplist.', '.$opts.', '.$static.', '.$routed.', '.$prio.')';
		$dbh->query($sql);
	    }
	}; break;
	case 'edit': {
	    $err_edit_notfound = 1;
	    if(isset($_GET['id'])) {
		$id = (int)$_GET['id'];
		$sql = "select id, int_mask, iplist, options, static, route, prio, note, ptp from interfaces where id=$id;";
		$sth = $dbh->query($sql);
		if($fa = $dbh->fetch_array($sth)) {
		    $err_edit_notfound = 0;
		    show_form("edit", "Редактирование маски интерфейса", $fa[0], $fa[6], $fa[1], $fa[2], $fa[3], $fa[4], $fa[5], $fa[7], $fa[8]);
		}
	    } elseif(isset($_POST['id'])) {
		$id = (int)$_POST['id'];
		
		check_input();
		clear_div();
		if(!$err_input) {
		    $static = isset($_POST['static'])?(int)$_POST['static']:0;
		    $routed = isset($_POST['routed'])?(int)$_POST['routed']:1;
		    $prio = isset($_POST['prio'])?(int)$_POST['prio']:0;
		    $note = isset($_POST['note'])?convert_quot($_POST['note']):'';
		    $ptp = isset($_POST['ptp'])?(int)$_POST['ptp']:1;

		    $sql = 'update interfaces set ptp='.$ptp.', note="'.$note.'", prio='.$prio.', int_mask="'.convert_quot($intmask).'", iplist='.$iplist.', options='.$opts.', static='.$static.', route='.$routed.'  where id='.$id;
		    if($dbh->query($sql)) {
			$err_edit_notfound = 0;
			show_message("Обновление маски интерфейса", 'Операция выполнена успешно<br /><a href="index.php?pos=intmask">Вернуться</a>');
		    };
		}
	    };
	    
	    if($err_edit_notfound) {
		show_message("Обновление маски интерфейса", 'Во время операции обновления произошла ошибка<br /><a href="index.php?pos=intmask">Вернуться</a>');
	    };
	}; return;
	case 'del': {
	    $err_delete = 1;
	    if(isset($_GET['id'])) {
		$id = (int)$_GET['id'];
		
		$sql = 'delete from interfaces where id='.$id;
		if($dbh->query($sql)) {
		    $err_delete = 0;
		    show_message("Удаление маски интерфейса", 'Операция выполнена успешно<br /><a href="index.php?pos=intmask">Вернуться</a>');
		}
	    };
	    if($err_delete) {
		show_message("Удаление маски интерфейса", 'Во время операции удаления произошла ошибка<br /><a href="index.php?pos=intmask">Вернуться</a>');
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
	<th>Приоритет</th>
	<th>Маска</th>
	<th>список IP</th>
	<th>набор опций</th>
	<th>Адреса<br />статичны</th>
	<th>Создавать<br />маршруты</th>
	<th>ptp</th>
	<th>Записка</th>
	<th colspan="2">Действия</th>
    </tr>
</thead>
<tbody>
<?php
    $sql = "select interfaces.id, interfaces.int_mask, interfaces.iplist, interfaces.options, interfaces.static, interfaces.route,
            iplists.name, options_template.name, interfaces.prio, interfaces.note, interfaces.ptp from interfaces,iplists,options_template where iplists.id=interfaces.iplist and options_template.id=interfaces.options;";
    $sth = $dbh->query($sql);
    
    while($fa=$dbh->fetch_array($sth)) {
	echo '<tr>
  <td>'.$fa[0].'</td>
  <td>'.$fa[8].'</td>
  <td>'.$fa[1].'</td>
  <td>'.$fa[6].'</td>
  <td>'.$fa[7].'</td>
  <td>'.($fa[4]==1?'Да':'Нет').'</td>
  <td>'.($fa[5]==1?'Да':'Нет').'</td>
  <td>'.($fa[10]==1?'Да':'Нет').'</td>
  <td>'.$fa[9].'</td>
  <td><a href="index.php?pos=intmask&cmd=edit&id='.$fa[0].'">Изменить</a></td>
  <td><a href="index.php?pos=intmask&cmd=del&id='.$fa[0].'">Удалить</a></td>
</tr>';
    };
?>
</tbody>
</table>
</div>
<div class="clr"></div>

<?php
show_form("doadd", "Добавить маску интерфеса");
check_form();

function show_form($cmd, $legend, $id=0, $prio=0, $intmask="", $iplist=0, $opts=0, $static=0, $routed=1, $note='', $ptp=1) {
    echo '<div class="form"><form method="post" action="index.php?pos=intmask&cmd='.$cmd.'"><fieldset>
    <legend>'.$legend.'</legend>
    <input type="hidden" name="id" value="'.$id.'" />
    <table border="0">
	<tr class="field"><td>Приоритет</td><td><input type="text" name="prio" value="'.$prio.'" /></td></tr>
	<tr class="field"><td>Маска интерфейса</td><td><input type="text" name="intmask" value="'.$intmask.'" /></td></tr>
	<tr class="field"><td>лист IP</td><td>'.generate_ip_list($iplist).'</td></tr>
	<tr class="field"><td>набор опций</td><td>'.generate_opts_list($opts).'</td></tr>
	<tr class="field"><td>Адреса статичны</td><td>'.generate_yes_no("static", $static).'</td></tr>
	<tr class="field"><td>Создавать маршруты</td><td>'.generate_yes_no("routed", $routed).'</td></tr>
	<tr class="field"><td>за интерфейсом 1 абонент</td><td>'.generate_yes_no("ptp", $ptp).'</td></tr>
	<tr class="field"><td>Описание</td><td><input type="text" name="note" value="'.$note.'" /></td></tr>
	<tr class="field fieldctrl"><td colspan="2"><input type="submit" value="Отправить" /></td></tr>
    </table>
</fieldset></form></div>';
}

function check_form($inter="") {
    echo '<div class="form"><form method="post" action="index.php?pos=intmask&cmd=check"><fieldset>
    <legend>Проверка масок</legend>
    <table border="0">
	<tr class="field"><td>Пакет пришёл с адреса</td><td><input type="text" name="inter" value="'.$inter.'" /></td></tr>
	<tr class="field fieldctrl"><td colspan="2"><input type="submit" value="Отправить" /></td></tr>
    </table>
</fieldset></form></div>';
}

function check_input() {
	    global $intmask, $iplist, $opts;
	    
	    $intmask = isset($_POST['intmask'])?$_POST['intmask']:'';
	    $iplist = isset($_POST['iplist'])?(int)$_POST['iplist']:0;
	    $opts = isset($_POST['opts'])?(int)$_POST['opts']:0;
	    
	    check_data($intmask, "не указана маска интерфеса");
	    check_data($iplist, "не выбран список IP");
	    check_data($opts, "не выбран набор опций");
}

function generate_ip_list($iplist) {
    global $dbh;
    $rv = '<select name="iplist">';
    
    $sql = "select id, name from iplists;";
    $sth = $dbh->query($sql);
    
    while($fa=$dbh->fetch_array($sth)) {
	$rv .= '<option value="'.$fa[0].'"'.($iplist==$fa[0]?' selected':'').'>'.$fa[1].'</option>';
    }
    
    $rv .= '</select>';
    return $rv;
}

function generate_opts_list($opt) {
    global $dbh;
    $rv = '<select name="opts">';

    $sql = "select id, name from options_template;";
    $sth = $dbh->query($sql);
    
    while($fa=$dbh->fetch_array($sth)) {
	$rv .= '<option value="'.$fa[0].'"'.($opt==$fa[0]?' selected':'').'>'.$fa[1].'</option>';
    }
    
    $rv .= '</select>';
    return $rv;
}

function generate_yes_no($name, $value) {
    $rv = '<select name="'.$name.'">';
    
    $rv .= '<option value="1"'.($value==1?' selected':'').'>Да</option>';
    $rv .= '<option value="0"'.($value==0?' selected':'').'>Нет</option>';
    
    $rv .= '</select>';
    return $rv;
}
?>