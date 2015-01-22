<?php
?>
<div class="clr"></div>
<div class="data">
<table class="tabledata">
<thead>
    <tr>
	<th>IP адрес</th>
	<th>Интерфейс</th>
	<th>Срок аренды истекает</th>
	<th>MAC адрес</th>
	<th>NAS ID</th>
    </tr>
</thead>
<tbody>
<?php
    $sql = "select INET_NTOA(ipaddr) as ip, interface, offered_till, chaddr, nasid from leases order by ipaddr asc;";
    $sth = $dbh->query($sql);
    
    while($fa=$dbh->fetch_array($sth)) {
	echo '<tr>
  <td>'.$fa[0].'</td>
  <td>'.$fa[1].'</td>
  <td>'.$fa[2].'</td>
  <td>'.$fa[3].'</td>
  <td>'.$fa[4].'</td>
</tr>';
    };
?>
</tbody>
</table>
</div>
<div class="clr"></div>

