<?php
require 'vendor/autoload.php';

use Slim\Factory\AppFactory;
use DI\Container;
use Psr\Http\Message\ResponseInterface as Response;
use Psr\Http\Message\ServerRequestInterface as Request;

$container = new Container();
AppFactory::setContainer($container);
$app = AppFactory::create();

// Подключение к БД — у вас в docker-compose должен быть сервис db,
// создающий базу contacts_db с пользователем user/password
$container->set('db', function() {
    $pdo = new PDO(
        'pgsql:host=db;dbname=contacts_db',
        'user',
        'password',
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );
    return $pdo;
});

/**
 * POST /contacts
 * Вставляем запись и сразу возвращаем её полностью в формате JSON
 */
$app->post('/contacts', function (Request $request, Response $response) use ($container) {
    $data = (array)$request->getParsedBody();

    $sql = <<<SQL
INSERT INTO contacts (external_id, phone_number)
VALUES (:external_id, :phone_number)
RETURNING id, external_id, phone_number, date_created, date_updated
SQL;

    $stmt = $container->get('db')->prepare($sql);
    $stmt->execute([
        ':external_id'  => $data['external_id'],
        ':phone_number' => $data['phone_number'],
    ]);

    // Забираем только что созданную запись
    $contact = $stmt->fetch(PDO::FETCH_ASSOC);

    $payload = json_encode($contact, JSON_UNESCAPED_SLASHES);

    $response->getBody()->write($payload);
    return $response
        ->withHeader('Content-Type', 'application/json')
        ->withStatus(201);
});

/**
 * GET /contacts
 * Возвращаем список, применяя фильтры limit, offset, external_id, phone_number
 */
$app->get('/contacts', function (Request $request, Response $response) use ($container) {
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
    foreach ($bind as $k => $v) {
        $type = is_int($v) ? PDO::PARAM_INT : PDO::PARAM_STR;
        $stmt->bindValue($k, $v, $type);
    }
    $stmt->execute();
    $contacts = $stmt->fetchAll(PDO::FETCH_ASSOC);

    $response->getBody()->write(json_encode($contacts, JSON_UNESCAPED_SLASHES));
    return $response->withHeader('Content-Type', 'application/json');
});

/**
 * GET /ping
 */
$app->get('/ping', function (Request $request, Response $response) {
    $response->getBody()->write('pong');
    return $response->withHeader('Content-Type', 'text/plain');
});

$app->run();
