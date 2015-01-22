<?php
    $cmd = "";
    
    if(isset($_GET['cmd'])) $cmd = strtolower($_GET['cmd']);
    
    switch($cmd) {
	case 'doadd': {
	    check_input_opts();
	    clear_div();
	    
	    if(!$err_input) {
		$sql = "insert into options_template(name) values('".convert_quot($optname)."')";
		$dbh->query($sql);
	    }
	}; break;
	case 'edit': {
	    $err_edit = 1;
	    if(isset($_GET['id'])) {
		$id = (int)$_GET['id'];
		$sql = "select name from options_template where id=".$id;
		if($fa=$dbh->fetch_array($dbh->query($sql))) {
		    $err_edit = 0;
		    show_form_opts("edit", "Изменение набора опций", $id, $fa[0]);
		}
	    } elseif(isset($_POST['id'])) {
		$id = (int)$_POST['id'];
		check_input_opts();
		clear_div();
		if(!$err_input) {
		    $sql = "update options_template set name='".convert_quot($optname)."' where id=".$id;
		    if($dbh->query($sql)) {
			$err_edit = 0;
			show_message("Изменение набора опций", 'Операция успешно выполнена<br /><a href="index.php?pos=opts">Вернуться</a>');
		    }
		}
	    }
	    if($err_edit) {
		show_message("Изменение набора опций", 'Во время операции изменения произошла ошибка<br /><a href="index.php?pos=opts">Вернуться</a>');
	    };
	}; return;
	case 'del': {
	    $err_delete = 1;
	    if(isset($_GET['id'])) {
		$id = (int)$_GET['id'];
		
		// сначала проверим, не назначен ли этои опции какому-либо интерфейсу
		$sql = "select int_mask from interfaces where options=".$id;
		if($fa=$dbh->fetch_array($dbh->query($sql))) {
		    show_error("Удаление не возможно:<br /> Удаляемый набор опций назначен интерфейсной маске '".$fa[0]."'");
		} else {
		    $sql = 'delete from options_template where id='.$id;
		    if($dbh->query($sql)) {
			$sql = 'delete from options_template_data where template='.$id;
			$dbh->query($sql);
			$err_delete = 0;
			show_message("Удаление набора опций", 'Операция выполнена успешно<br /><a href="index.php?pos=opts">Вернуться</a>');
		    }
		}
	    };
	    if($err_delete) {
		show_message("Удаление набора опций", 'Во время операции удаления произошла ошибка<br /><a href="index.php?pos=opts">Вернуться</a>');
	    };
	}; return;
	case 'optadd': {
	    check_input_opts_data();
	    clear_div();
	    
	    if(!$err_input) {
		$sql = "insert into options_template_data(template, code, value, type) values($opactive, $optcode, '".convert_quot($optvalue)."', $optype);";
		$dbh->query($sql);
	    }
	}; break;
	case 'optedit': {
	    $optedit_err = 1;
	    
	    if(isset($_GET['id'])&&isset($_GET['active'])) {
		$id = (int)$_GET['id'];
		$active = (int)$_GET['active'];
		$sql = "select code,value,type from options_template_data where id=".$id;
		if($fa=$dbh->fetch_array($dbh->query($sql))) {
		    $optedit_err = 0;
		    show_form_opts_edit("optedit", "Редактирование опции в ".get_option_template_name($active), $active, $id, $fa[0], $fa[1], $fa[2]);
		}
	    } elseif($_POST['id']&&isset($_GET['active'])) {
		$id = (int)$_POST['id'];
		$active = (int)$_GET['active'];
		
		check_input_opts_data();
		clear_div();
		
		if(!$err_input) {
		    $sql = "update options_template_data set code=$optcode, value='".convert_quot($optvalue)."', type=$optype where id=".$id;
		    if($dbh->query($sql)) {
			$optedit_err = 0;
			show_message("Редактирование опций", 'Операция выполнена успешно<br /><a href="index.php?pos=opts&active='.$active.'">Вернуться</a>');
		    }
		}
	    }
	    if($optedit_err) {
		show_message("Редактирование опции", 'Во время операции редактирования произошла ошибка<br /><a href="index.php?pos=opts&active='.$_GET['active'].'">Вернуться</a>');
	    };
	}; return;
	case 'optdel': {
	    $optdel_err = 1;
	    
	    if(isset($_GET['id'])&&isset($_GET['active'])) {
		$id = (int)$_GET['id'];
		$active = (int)$_GET['active'];
		
		$sql = "delete from options_template_data where id=$id and template=$active";
		if($dbh->query($sql)) {
		    $optdel_err = 0;
		    show_message("Удаление опции", 'Операция выполнена успешно<br /><a href="index.php?pos=opts&active='.$active.'">Вернуться</a>');
		}
	    }
	    if($optdel_err) {
		show_message("Удаление опции", 'Во время операции удаления произошла ошибка<br /><a href="index.php?pos=opts&active='.$_GET['active'].'">Вернуться</a>');
	    };
	}; return;
    }

    $active = (int)(isset($_GET['active'])?$_GET['active']:get_last_option_template()); 
    if($active == 0) $active = get_last_option_template();
?>
<div class="clr"></div>
<div class="data">
<table class="tabledata">
<thead>
    <tr>
	<th>#</th>
	<th>Имя набора опций</th>
	<th colspan="3">Действия</th>
    </tr>
</thead>
<tbody>
<?php
    $sql = "select id, name from options_template;";
    $sth = $dbh->query($sql);
    
    while($fa=$dbh->fetch_array($sth)) {
	$tr_active = ($active==$fa[0])?$tr_active=' class="tractive"':'';
	echo '<tr'.$tr_active.'>
  <td>'.$fa[0].'</td>
  <td>'.$fa[1].'</td>
  <td><a href="index.php?pos=opts&active='.$fa[0].'">Выбрать активным</a></td>
  <td><a href="index.php?pos=opts&cmd=edit&id='.$fa[0].'">Изменить</a></td>
  <td><a href="index.php?pos=opts&cmd=del&id='.$fa[0].'">Удалить</a></td>
</tr>';
    };
?>
</tbody>
</table>
</div>

<?php if($active > 0) { ?>
<div class="subdata">
    <legend>Редактирование опций в <?php echo get_option_template_name($active); ?></legend>
<table class="tabledata">
<thead>
    <tr>
	<th>#</th>
	<th>Код</th>
	<th>Тип</th>
	<th>Значение</th>
	<th colspan="3">Действия</th>
    </tr>
</thead>
<tbody>
<?php
    $sql = "select id,code,value,type from options_template_data where template=".$active;
    $sth = $dbh->query($sql);
    while($fa=$dbh->fetch_array($sth)) {
	echo '<tr>
    <td>'.$fa[0].'</td>
    <td>'.(isset($dhcp_options[$fa[1]])?$dhcp_options[$fa[1]].' ('.$fa[1].')':'UNKNOWN_'.$fa[1]).'</td>
    <td>'.(isset($dhcp_op_type[$fa[3]])?$dhcp_op_type[$fa[3]]:'UNKNOWN_'.$fa[3]).'</td>
    <td>'.$fa[2].'</td>
    <td><a href="index.php?pos=opts&cmd=optedit&active='.$active.'&id='.$fa[0].'">Изменить</a></td>
    <td><a href="index.php?pos=opts&cmd=optdel&active='.$active.'&id='.$fa[0].'">Удалить</a></td>
</tr>';
    };
?>
</tbody>
</table>
</div>
<?php }; ?>
<div class="clr"></div>

<?php
show_form_opts("doadd", "Добавить набор опций");
if($active > 0) show_form_opts_edit("optadd", "Добавление опции в ".get_option_template_name($active), $active);

function show_form_opts($cmd, $legend, $id=0, $name="") {
    echo '<div class="form"><form method="post" action="index.php?pos=opts&cmd='.$cmd.'"><fieldset>
    <legend>'.$legend.'</legend>
    <input type="hidden" name="id" value="'.$id.'" />
    <table border="0">
	<tr class="field"><td>Имя набора опций</td><td><input type="text" name="name" value="'.$name.'" /></td></tr>
	<tr class="field fieldctrl"><td colspan="2"><input type="submit" value="Отправить" /></td></tr>
    </table>
</fieldset></form></div>';
}

function show_form_opts_edit($cmd, $legend, $active, $id=0, $code=1, $value="", $optype=3) {
    echo '<div class="form"><form method="post" action="index.php?pos=opts&cmd='.$cmd.'&active='.$active.'"><fieldset>
    <legend>'.$legend.'</legend>
    <input type="hidden" name="id" value="'.$id.'" />
    <table border="0">
	<tr class="field"><td>Код опции</td><td><input type="text" name="code" value="'.$code.'" /></td><td>'.generate_code_list($code).'</td></tr>
	<tr class="field"><td>Значение</td><td><input type="text" name="value" value="'.$value.'" /></td><td></td></tr>
	<tr class="field"><td>Тип значения</td><td>'.generate_op_types($optype).'</td><td></td></tr>
	<tr class="field fieldctrl"><td colspan="3"><input type="submit" value="Отправить" /></td></tr>
    </table>
</fieldset></form></div>';
}

function check_input_opts() {
	    global $optname;
	    
	    $optname = $_POST['name'];
	    
	    check_data($optname, "не указано имя набора опций");
}

function check_input_opts_data() {
    global $optcode;
    global $optvalue;
    global $optype;
    global $opactive;
    
    $optcode = (isset($_POST['code'])?(int)$_POST['code']:'');
    $optvalue = (isset($_POST['value'])?$_POST['value']:'');
    $optype = (isset($_POST['optypes'])?(int)$_POST['optypes']:'');
    $opactive = (isset($_GET['active'])?(int)$_GET['active']:'');

    check_data($optcode, "не выбран код опци");
    check_data($optvalue, "не указано значение опции");
    check_data($optype, "не выбран тип опции");
    check_data($opactive, "не выбран активный набор опций");
}

function get_last_option_template() {
    global $dbh;
    
    $sql = "select id from options_template order by id desc limit 0,1;";
    if($fa=$dbh->fetch_array($dbh->query($sql))) return $fa[0];
    
    return 0;
}

function get_option_template_name($id) {
    global $dbh;
    
    $sql = "select name from options_template where id=".$id;

    if($fa=$dbh->fetch_array($dbh->query($sql))) return $fa[0];
    
    return 0;
}

function generate_op_types($active) {
    $rv = '<select name="optypes">';
    $rv .= '<option value="1"'.($active==1?' selected':'').'>Байт</option>';
    $rv .= '<option value="2"'.($active==2?' selected':'').'>Строка</option>';
    $rv .= '<option value="3"'.($active==3?' selected':'').'>IP Адрес</option>';
    $rv .= '<option value="4"'.($active==4?' selected':'').'>Список IP адресов</option>';
    $rv .= '<option value="5"'.($active==5?' selected':'').'>Число (длина 4 байта)</option>';
    $rv .= '</select>';
    return $rv;
}

function generate_code_list($code) {
    global $dhcp_options;
    
    $rv = '<select name="codelist" onchange="this.form.code.value=this.form.codelist.value">';
    foreach($dhcp_options as $key => $value) {
	$rv .= '<option value="'.$key.'"'.($code==$key?' selected':'').($key>256?' disabled':'').'>'.($key<255?$key:'').' '.$value.'</option>';
    }
    $rv .= '</select>';
    return $rv;
}
?>