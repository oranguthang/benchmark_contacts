<?php
require 'vendor/autoload.php';

use Slim\Factory\AppFactory;
use DI\Container;

$container = new Container();
AppFactory::setContainer($container);
$app = AppFactory::create();

$container->set('db', function() {
    return new PDO('pgsql:host=db;dbname=contacts_db', 'user', 'password', [
        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
    ]);
});

$app->post('/contacts', function ($request, $response) use ($container) {
    $data = $request->getParsedBody();

    // В PostgreSQL у вас должен быть DEFAULT-значок для id (UUID)
    // и DEFAULT now() для date_created/date_updated
    $stmt = $container->get('db')->prepare(
        'INSERT INTO contacts (external_id, phone_number)
         VALUES (:external_id, :phone_number)
         RETURNING id, external_id, phone_number, date_created, date_updated'
    );
    $stmt->execute([
        ':external_id'  => $data['external_id'],
        ':phone_number' => $data['phone_number'],
    ]);

    $contact = $stmt->fetch(PDO::FETCH_ASSOC);

    $payload = json_encode($contact);
    $response->getBody()->write($payload);

    return $response
        ->withHeader('Content-Type', 'application/json')
        ->withStatus(201);
});

$app->get('/contacts', function ($request, $response) use ($container) {
    $params = $request->getQueryParams();
    $limit  = isset($params['limit'])       ? min((int)$params['limit'], 10000) : 10000;
    $offset = isset($params['offset'])      ? (int)$params['offset']           : 0;
    $external_id  = isset($params['external_id'])  ? (int)$params['external_id']      : null;
    $phone_number = isset($params['phone_number']) ? $params['phone_number']         : null;

    $sql = 'SELECT id, external_id, phone_number, date_created, date_updated
            FROM contacts
            WHERE TRUE';
    $bind = [];
    if ($external_id !== null) {
        $sql .= ' AND external_id = :external_id';
        $bind[':external_id'] = $external_id;
    }
    if ($phone_number !== null) {
        $sql .= ' AND phone_number = :phone_number';
        $bind[':phone_number'] = $phone_number;
    }
    $sql .= ' LIMIT :limit OFFSET :offset';
    $bind[':limit']  = $limit;
    $bind[':offset'] = $offset;

    $stmt = $container->get('db')->prepare($sql);

    // sqlite/pgsql требуют чуть-чуть другого бинд-режима для LIMIT/OFFSET,
    // но в PDO это должно сработать
    foreach ($bind as $k=>$v) {
        $paramType = is_int($v) ? PDO::PARAM_INT : PDO::PARAM_STR;
        $stmt->bindValue($k, $v, $paramType);
    }

    $stmt->execute();
    $contacts = $stmt->fetchAll(PDO::FETCH_ASSOC);

    $response->getBody()->write(json_encode($contacts));
    return $response->withHeader('Content-Type', 'application/json');
});

$app->get('/ping', function ($request, $response) {
    $response->getBody()->write('pong');
    return $response->withHeader('Content-Type', 'text/plain');
});

$app->run();
