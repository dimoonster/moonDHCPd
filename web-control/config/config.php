<?php
    // Параметры доступа к БД
    $config = new stdClass();
    $config->db_host	= "127.0.0.1";
    $config->db_user	= "root";
    $config->db_pwd	= "root";
    $config->db_name	= "dhcp";
    
    // Информация о проекте
    $prj = new stdClass();
    $prj->version	= "0.1";
    
    // Типы значений опций
    $dhcp_op_type[1]="Байт";
    $dhcp_op_type[2]="Строка";
    $dhcp_op_type[3]="IP адрес";
    $dhcp_op_type[4]="Список IP адресов";
    $dhcp_op_type[5]="Число (4 байта)";
    
    // Список DHCP опций
    $dhcp_options = array();
    $dhcp_options[3]="ROUTERS";
    $dhcp_options[1]="SUBNET_MASK";
    $dhcp_options[6]="DOMAIN_NAME_SERVERS";
    $dhcp_options[51]="DHCP_LEASE_TIME";
    $dhcp_options[1000]="-----------------------";
    $dhcp_options[2]="TIME_OFFSET";
    $dhcp_options[4]="TIME_SERVERS";
    $dhcp_options[5]="NAME_SERVERS";
    $dhcp_options[7]="LOG_SERVERS";
    $dhcp_options[8]="COOKIE_SERVERS";
    $dhcp_options[9]="LPR_SERVERS";
    $dhcp_options[10]="IMPRESS_SERVERS";
    $dhcp_options[11]="RESOURCE_LOCATION_SERVERS";
    $dhcp_options[12]="HOST_NAME";
    $dhcp_options[13]="BOOT_SIZE";
    $dhcp_options[14]="MERIT_DUMP";
    $dhcp_options[15]="DOMAIN_NAME";
    $dhcp_options[16]="SWAP_SERVER";
    $dhcp_options[17]="ROOT_PATH";
    $dhcp_options[18]="EXTENSIONS_PATH";
    $dhcp_options[19]="IP_FORWARDING";
    $dhcp_options[20]="NON_LOCAL_SOURCE_ROUTING";
    $dhcp_options[21]="POLICY_FILTER";
    $dhcp_options[22]="MAX_DGRAM_REASSEMBLY";
    $dhcp_options[23]="DEFAULT_IP_TTL";
    $dhcp_options[24]="PATH_MTU_AGING_TIMEOUT";
    $dhcp_options[25]="PATH_MTU_PLATEAU_TABLE";
    $dhcp_options[26]="INTERFACE_MTU";
    $dhcp_options[27]="ALL_SUBNETS_LOCAL";
    $dhcp_options[28]="BROADCAST_ADDRESS";
    $dhcp_options[29]="PERFORM_MASK_DISCOVERY";
    $dhcp_options[30]="MASK_SUPPLIER";
    $dhcp_options[31]="ROUTER_DISCOVERY";
    $dhcp_options[32]="ROUTER_SOLICITATION_ADDRESS";
    $dhcp_options[33]="STATIC_ROUTES";
    $dhcp_options[34]="TRAILER_ENCAPSULATION";
    $dhcp_options[35]="ARP_CACHE_TIMEOUT";
    $dhcp_options[36]="IEEE802_3_ENCAPSULATION";
    $dhcp_options[37]="DEFAULT_TCP_TTL";
    $dhcp_options[38]="TCP_KEEPALIVE_INTERVAL";
    $dhcp_options[39]="TCP_KEEPALIVE_GARBAGE";
    $dhcp_options[40]="NIS_DOMAIN";
    $dhcp_options[41]="NIS_SERVERS";
    $dhcp_options[42]="NTP_SERVERS";
    $dhcp_options[43]="VENDOR_ENCAPSULATED_OPTIONS";
    $dhcp_options[44]="NETBIOS_NAME_SERVERS";
    $dhcp_options[45]="NETBIOS_DD_SERVER";
    $dhcp_options[46]="NETBIOS_NODE_TYPE";
    $dhcp_options[47]="NETBIOS_SCOPE";
    $dhcp_options[48]="FONT_SERVERS";
    $dhcp_options[49]="X_DISPLAY_MANAGER";
    $dhcp_options[52]="OVERLOAD";
    $dhcp_options[54]="DHCP_SERVER_IDENTIFIER";
    $dhcp_options[56]="DHCP_MESSAGE";
    $dhcp_options[58]="DHCP_RENEWAL_TIME";
    $dhcp_options[59]="DHCP_REBINDING_TIME";
    $dhcp_options[60]="VENDOR_CLASS_IDENTIFIER";
    $dhcp_options[61]="DHCP_CLIENT_IDENTIFIER";
    $dhcp_options[62]="NWIP_DOMAIN_NAME";
    $dhcp_options[63]="NWIP_SUBOPTIONS";
    $dhcp_options[66]="FTP_SERVER";
    $dhcp_options[67]="BOOT_FILE";
    $dhcp_options[77]="USER_CLASS";
    $dhcp_options[81]="FQDN";

    // сервисные переменные
    $query_count = 0;
?>