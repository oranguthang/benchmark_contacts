<?php
declare(strict_types=1);

use Slim\Factory\AppFactory;
use DI\Container;
use Psr\Http\Message\ServerRequestInterface as Request;
use Psr\Http\Message\ResponseInterface as Response;

// контейнер
$container = new Container();
AppFactory::setContainer($container);
$app = AppFactory::create();

// POST /contacts
$app->post('/contacts', function (Request $request, Response $response) use ($pool) {
    $data = (array)$request->getParsedBody();

    // берём PDO из пула
    /** @var PDO $pdo */
    $pdo = $pool->dequeue();
    try {
        $stmt = $pdo->prepare(<<<'SQL'
INSERT INTO contacts (external_id, phone_number)
VALUES (:external_id, :phone_number)
RETURNING id, external_id, phone_number, date_created, date_updated
SQL
        );
        $stmt->execute([
            ':external_id'  => $data['external_id'],
            ':phone_number' => $data['phone_number'],
        ]);
        $contact = $stmt->fetch(PDO::FETCH_ASSOC);
    } finally {
        // возвращаем соединение в пул
        $pool->enqueue($pdo);
    }

    $payload = json_encode($contact, JSON_UNESCAPED_SLASHES);
    $response->getBody()->write($payload);

    return $response
        ->withHeader('Content-Type', 'application/json')
        ->withStatus(201);
});

// GET /contacts
$app->get('/contacts', function (Request $request, Response $response) use ($pool) {
    $params = $request->getQueryParams();
    $limit  = isset($params['limit']) ? min((int)$params['limit'], 10000) : 10000;
    $offset = isset($params['offset']) ? (int)$params['offset'] : 0;
    $extId  = isset($params['external_id']) ? (int)$params['external_id'] : null;
    $phone  = $params['phone_number'] ?? null;

    // готовим запрос
    $sql  = 'SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE TRUE';
    $bind = [];
    if ($extId !== null) {
        $sql       .= ' AND external_id = :external_id';
        $bind[':external_id'] = $extId;
    }
    if ($phone !== null) {
        $sql       .= ' AND phone_number = :phone_number';
        $bind[':phone_number'] = $phone;
    }
    $sql .= ' LIMIT :limit OFFSET :offset';
    $bind[':limit']  = $limit;
    $bind[':offset'] = $offset;

    // пул
    /** @var PDO $pdo */
    $pdo = $pool->dequeue();
    try {
        $stmt = $pdo->prepare($sql);
        foreach ($bind as $k => $v) {
            $type = is_int($v) ? PDO::PARAM_INT : PDO::PARAM_STR;
            $stmt->bindValue($k, $v, $type);
        }
        $stmt->execute();
        $contacts = $stmt->fetchAll(PDO::FETCH_ASSOC);
    } finally {
        $pool->enqueue($pdo);
    }

    $response->getBody()->write(json_encode($contacts, JSON_UNESCAPED_SLASHES));
    return $response->withHeader('Content-Type', 'application/json');
});

// GET /ping
$app->get('/ping', function (Request $request, Response $response) {
    $response->getBody()->write('pong');
    return $response->withHeader('Content-Type', 'text/plain');
});

return $app;
