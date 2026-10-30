<?php
//Adatbázis kapcsolat
$host = "SAJAT_ADATBAZIS_HOST"; 
$user = "SAJAT_FELHASZNALO";
$pass = "TITKOS_JELSZO"; 
$db   = "SAJAT_ADATBAZIS_NEVE";

$conn = new mysqli($host, $user, $pass, $db);
if ($conn->connect_error) { die("Adatbazis hiba"); }
$conn->query("SET time_zone = '+02:00';");

$action = $_GET['action'] ?? null;

//Mért adatok fogadása
if (isset($_GET['ho']) && isset($_GET['kid']) && !$action) {
    $ho = floatval($_GET['ho']);
    $kid = intval($_GET['kid']);
    $frek = floatval($_GET['frek'] ?? 0); 
    
    //Utolsó kijáró állapot lekérése az adatbázisból
    $res = $conn->query("SELECT kijaro FROM teszt_adatok WHERE kaptar_id = $kid ORDER BY id DESC LIMIT 1");
    $kijaro = ($res && $res->num_rows > 0) ? $res->fetch_assoc()['kijaro'] : 0;

    //Új mérés mentése
    $conn->query("INSERT INTO teszt_adatok (kaptar_id, homerseklet, kijaro, frekvencia) VALUES ($kid, $ho, $kijaro, $frek)");
    
    //Napi átlag frekvencia számítása a riasztáshoz
    $res_atlag = $conn->query("SELECT AVG(frekvencia) as napi_atlag FROM teszt_adatok WHERE kaptar_id = $kid AND DATE(idobelyeg) = CURDATE() AND frekvencia > 0");
    $napi_atlag = ($res_atlag && $res_atlag->num_rows > 0) ? floatval($res_atlag->fetch_assoc()['napi_atlag']) : 0;

    $riasztas_kell = ($napi_atlag > 50 && $frek > ($napi_atlag * 1.5)) ? 1 : 0;

    //SMS engedélyezve vagy nem
    $sms_engedelyezve = file_exists('sms_status.txt') ? (int)file_get_contents('sms_status.txt') : 1;
    if ($sms_engedelyezve === 0) { 
        $riasztas_kell = 0; 
    }

    //Válasz a LilyGO-nak
    echo "OK KIJARO:" . $kijaro . " R:" . $riasztas_kell;
    exit();
}

// Frontend
switch ($action) {
    
    case 'get_sms_status':
        echo file_exists('sms_status.txt') ? file_get_contents('sms_status.txt') : "1";
        break;

    case 'set_sms_status':
        if (isset($_GET['state'])) {
            file_put_contents('sms_status.txt', intval($_GET['state']));
            echo "OK";
        }
        break;

    case 'get_data':
        //Legfrissebb adatok lekérése minden kaptárhoz
        header('Content-Type: application/json'); 
        $sql = "SELECT t1.* FROM teszt_adatok t1 
                INNER JOIN (SELECT kaptar_id, MAX(id) as max_id FROM teszt_adatok GROUP BY kaptar_id) t2 
                ON t1.id = t2.max_id ORDER BY t1.kaptar_id ASC";
        $result = $conn->query($sql);
        
        $adatok = array();
        while($row = $result->fetch_assoc()) { $adatok[] = $row; }
        echo json_encode($adatok);
        break;

    case 'set_gate':
        //Kijáró állapotának beállítása webről
        if (isset($_GET['kid']) && isset($_GET['state'])) {
            $kid = intval($_GET['kid']);
            $state = intval($_GET['state']);
            
            $res = $conn->query("SELECT homerseklet, frekvencia FROM teszt_adatok WHERE kaptar_id = $kid ORDER BY id DESC LIMIT 1");
            $ho = 0; $frek = 0;
            
            if($res && $res->num_rows > 0) {
                $row = $res->fetch_assoc();
                $ho = $row['homerseklet'];
                $frek = $row['frekvencia'] ?? 0;
            }

            $conn->query("INSERT INTO teszt_adatok (kaptar_id, homerseklet, kijaro, frekvencia) VALUES ($kid, $ho, $state, $frek)");
            echo "OK";
        }
        break;

    case 'get_history':
        //Visszamenőleges adatok lekérése a grafikonhoz
        if (isset($_GET['kid'])) {
            header('Content-Type: application/json');
            $kid = intval($_GET['kid']);
            $hours = isset($_GET['hours']) ? intval($_GET['hours']) : 24;
            
            if ($hours === 0) {
                $sql = "SELECT homerseklet, frekvencia, idobelyeg FROM teszt_adatok WHERE kaptar_id = $kid ORDER BY id DESC LIMIT 2000";
            } else {
                $utolso_datum = date('Y-m-d H:i:s');
                $res_max = $conn->query("SELECT MAX(idobelyeg) as utolso FROM teszt_adatok WHERE kaptar_id = $kid");
                
                if ($res_max && $res_max->num_rows > 0) {
                    $row = $res_max->fetch_assoc();
                    if (!empty($row['utolso'])) $utolso_datum = $row['utolso'];
                }
                
                $sql = "SELECT homerseklet, frekvencia, idobelyeg FROM teszt_adatok 
                        WHERE kaptar_id = $kid AND idobelyeg >= DATE_SUB('$utolso_datum', INTERVAL $hours HOUR) 
                        ORDER BY id DESC LIMIT 1000";
            }
            
            $result = $conn->query($sql);
            $adatok = array();
            if ($result) {
                while($row = $result->fetch_assoc()) { $adatok[] = $row; }
            }
            
            echo json_encode(array_reverse($adatok));
        }
        break;
}

$conn->close();
?>