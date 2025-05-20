<?php
declare(strict_types=1);

use Spiral\RoadRunner\Worker;
use Spiral\Goridge\StreamRelay;
use Spiral\RoadRunner\Http\PSR7Client;
use Nyholm\Psr7\Factory\Psr17Factory;

require __DIR__ . '/vendor/autoload.php';

// создаём RR-воркер
$relay  = new StreamRelay(STDIN, STDOUT);
$worker = new Worker($relay);

// PSR-7 конвертеры
$psr17  = new Psr17Factory();
$psr7   = new PSR7Client($worker, $psr17, $psr17, $psr17);

// читаем CPU_CORES и строим пул
$cpuCores   = (int)(getenv('CPU_CORES') ?: 1);
$totalConns = $cpuCores * 4;

// каждый воркер получит равную часть пула:
$perWorker = max(1, (int)floor($totalConns / $cpuCores));

$pool = new SplQueue();
for ($i = 0; $i < $perWorker; $i++) {
    $pdo = new PDO(
        'pgsql:host=db;dbname=contacts_db',
        'user',
        'password',
        [
            PDO::ATTR_ERRMODE         => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_PERSISTENT      => true,
        ]
    );
    $pool->enqueue($pdo);
}

// загружаем Slim-приложение; в app.php мы будем использовать $pool
$app = require __DIR__ . '/app.php';

// основной цикл приёма HTTP-запросов
while ($req = $psr7->acceptRequest()) {
    try {
        $resp = $app->handle($req);
        $psr7->respond($resp);
    } catch (\Throwable $e) {
        $worker->error((string)$e);
    }
}
