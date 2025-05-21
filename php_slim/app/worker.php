<?php
declare(strict_types=1);

use Spiral\RoadRunner\Worker;
use Spiral\Goridge\StreamRelay;
use Spiral\RoadRunner\Http\PSR7Worker;
use Nyholm\Psr7\Factory\Psr17Factory;

require __DIR__ . '/vendor/autoload.php';

// создаём RR-воркер
$relay  = new StreamRelay(STDIN, STDOUT);
$worker = new Worker($relay);

// PSR-7 фабрики
$psr17     = new Psr17Factory();
$psr7      = new PSR7Worker($worker, $psr17, $psr17, $psr17);

// читаем CPU_CORES и строим пул
$cpuCores   = (int)(getenv('CPU_CORES') ?: 1);
$totalConns = $cpuCores * 4;
$perWorker  = max(1, (int)floor($totalConns / $cpuCores));

$pool = new SplQueue();
for ($i = 0; $i < $perWorker; $i++) {
    $pdo = new PDO(
        'pgsql:host=db;dbname=contacts_db',
        'user',
        'password',
        [
            PDO::ATTR_ERRMODE    => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_PERSISTENT => true,
        ]
    );
    $pool->enqueue($pdo);
}

// загружаем Slim-приложение
$app = require __DIR__ . '/index.php';

// основной цикл
while (true) {
    try {
        $request = $psr7->waitRequest();
        if ($request === null) {
            $worker->waitPayload(); // чтобы не сдох воркер
            continue;
        }

        $response = $app->handle($request);
        $psr7->respond($response);
    } catch (\Throwable $e) {
        $worker->error((string)$e);
    }
}
