<?php
declare(strict_types=1);

use Spiral\RoadRunner\Worker;
use Spiral\RoadRunner\Http\PSR7Worker;
use Nyholm\Psr7\Factory\Psr17Factory;

require __DIR__ . '/vendor/autoload.php';

// create RR worker automatically via RR_RPC
$worker = Worker::create();

// PSR-7 factories
$psr17 = new Psr17Factory();
$psr7  = new PSR7Worker($worker, $psr17, $psr17, $psr17);

// read CPU_CORES and build the pool
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

// load Slim app
$app = require __DIR__ . '/index.php';

// main loop
while (true) {
    try {
        $request = $psr7->waitRequest();
        if ($request === null) {
            $worker->waitPayload(); // ping from RR
            continue;
        }

        $response = $app->handle($request);
        $psr7->respond($response);
    } catch (\Throwable $e) {
        $worker->error((string)$e);
    }
}
