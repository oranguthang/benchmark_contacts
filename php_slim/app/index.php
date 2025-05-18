<?php
require 'vendor/autoload.php';

use Slim\Factory\AppFactory;
use DI\Container;

$container = new Container();
AppFactory::setContainer($container);
$app = AppFactory::create();

$container->set('db', function() {
    return new PDO('pgsql:host=python_db;dbname=postgres', 'postgres', 'postgres');
});

$app->post('/contacts', function ($request, $response) use ($container) {
    $data = $request->getParsedBody();
    $stmt = $container->get('db')->prepare('INSERT INTO contacts (external_id, phone_number) VALUES (?, ?)');
    $success = $stmt->execute([$data['external_id'], $data['phone_number']]);
    if (!$success) {
        // Ошибка вставки
        return $response->withStatus(500)->write('DB error');
    }
    return $response->withStatus(201);
});

$app->get('/contacts', function ($request, $response) use ($container) {
    $params = $request->getQueryParams();
    $limit = isset($params['limit']) ? min((int)$params['limit'], 10000) : 10000;
    $offset = isset($params['offset']) ? (int)$params['offset'] : 0;
    $external_id = isset($params['external_id']) ? (int)$params['external_id'] : null;
    $phone_number = isset($params['phone_number']) ? $params['phone_number'] : null;

    $sql = 'SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE TRUE';
    $paramsList = [];
    if ($external_id) {
        $sql .= ' AND external_id = ?';
        $paramsList[] = $external_id;
    }
    if ($phone_number) {
        $sql .= ' AND phone_number = ?';
        $paramsList[] = $phone_number;
    }
    $sql .= ' LIMIT ? OFFSET ?';
    $paramsList[] = $limit;
    $paramsList[] = $offset;

    $stmt = $container->get('db')->prepare($sql);
    $stmt->execute($paramsList);
    $contacts = $stmt->fetchAll(PDO::FETCH_ASSOC);

    $response->getBody()->write(json_encode($contacts));
    return $response->withHeader('Content-Type', 'application/json');
});

$app->get('/ping', function ($request, $response) {
    $response->getBody()->write('pong');
    return $response->withHeader('Content-Type', 'text/plain');
});

$app->run();
