<?php
$err_input = 0;

function check_data($data, $errmsg) {
    global $err_input;
    
    if($data != "") return;
    
    $err_input = 1;
    
    show_error($errmsg);
};

function check_ipinput($ip, $errmsg) {
    global $err_input;

    $long = ip2long($ip);

    if(!($long == -1 || $long === FALSE)) return;
    
    $err_input = 1;
    
    show_error($errmsg);
};

function show_error($errmsg) {
    echo '<div class="error">
    <legend>Ошибка</legend>
    '.$errmsg.'
</div>';
};

function show_message($legend, $msg) {
    echo '<div class="clr"></div><div class="msgbox">
    <legend>'.$legend.'</legend>
    '.$msg.'
</div><div class="clr"></div>';
};

function clear_div() {
    echo '<div class="clr"></div>';
};

function convert_quot($indata) {
    return addslashes(str_replace('"', "&quot;", $indata));
};
?>