<?php
    require_once("config/config.php");
    require_once("lib.php");
    require_once("mysql.lib.php");
    
    $__exec_time = time();

    $dbh = new sql();
    $dbh->connect();
        
    require_once("design/head.php");

    if(!isset($_GET['pos'])) $_GET['pos'] = "";

    switch(strtolower($_GET['pos'])) {
	case 'iplist': {
	    require_once("iplist.php");
	}; break;
	case 'opts': {
	    require_once("opts.php");
	}; break;
	case 'leases': {
	    require_once("leases.php");
	}; break;
	default: {
	    require_once("intmask.php");
	};
    }
    
    $__exec_time = time() - $__exec_time;

    require_once("design/bottom.php");
?>