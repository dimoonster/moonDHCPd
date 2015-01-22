<?php
class sql {
  var $dbh;

  function connect($db_host="", $db_user="", $db_pwd="", $db_name="") {
    global $config;
    if ($this->dbh == 0) {
      $db_host = $db_host!=""?$db_host:$config->db_host;
          
      $this->dbh = mysql_connect($db_host, $db_user!=""?$db_user:$config->db_user, $db_pwd!=""?$db_pwd:$config->db_pwd, true);
    };
    if (!$this->dbh) {
      $this->not_right("Ошибка подключения к БД!");
      exit();
    };
    mysql_select_db($db_name?$db_name:$config->db_name, $this->dbh);
  }

  function graberrordesc() {
    $this->error = mysql_error();
    return $this->error;
  }

  function graberrornum() {
    $this->errornum = mysql_errno();
    return $this->errornum;
  }

  function query($query) {
    global $query_count;
    
    $query_count++;
    
    $this->sth = mysql_query($query, $this->dbh);
    if(!$this->sth) {
      $this->not_right("Ошибка выполнения запроса: $query");
    };
    return $this->sth;
  }

  function fetch_array($sth) {      
    $this->row = mysql_fetch_array($sth); 
    return $this->row;
  }
  function finish_sth($sth) {
    return @mysql_free_result($sth);
  }
  function close() {
    return @mysql_close($this->dbh);
  }
  function total_rows($sth) {
    return mysql_num_rows($sth);
  }
  function num_fields($sth) {
    return mysql_num_fields($sth);
  }
  function field_name($sth,$i) {
    return mysql_field_name($sth,$i);
  }
  function not_right($error) {
    $this->errordesc = mysql_error();
    $this->errornum  = mysql_errno();  
    echo '<div class="error"><legend>Ошибка SQLClass: </legend> 
    <i>'.$error.'</i><br /> '.$this->errordesc.': '.$this->errornum.'</div>';
  }
  function affected_rows() {
    return mysql_affected_rows($this->dbh);
  } 
}

?>