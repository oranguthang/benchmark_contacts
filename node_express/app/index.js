const express = require('express');
const { Client } = require('pg');
const bodyParser = require('body-parser');
const app = express();

app.use(bodyParser.json());

const client = new Client({
  user: 'postgres',
  host: 'python_db',
  database: 'postgres',
  password: 'postgres',
  port: 5432,
});

client.connect();

app.post('/contacts', async (req, res) => {
  const { external_id, phone_number } = req.body;

  const result = await client.query(
    'INSERT INTO contacts (external_id, phone_number) VALUES ($1, $2) RETURNING *',
    [external_id, phone_number]
  );
  res.status(201).json(result.rows[0]);
});

app.get('/contacts', async (req, res) => {
  const { external_id, phone_number, limit = 10000, offset = 0 } = req.query;

  let query = 'SELECT * FROM contacts WHERE TRUE';
  const params = [];

  if (external_id) {
    query += ' AND external_id = $' + (params.length + 1);
    params.push(external_id);
  }

  if (phone_number) {
    query += ' AND phone_number = $' + (params.length + 1);
    params.push(phone_number);
  }

  query += ' LIMIT $' + (params.length + 1) + ' OFFSET $' + (params.length + 2);
  params.push(limit, offset);

  const result = await client.query(query, params);
  res.json(result.rows);
});

app.listen(8080, () => {
  console.log('Server is running on port 8080');
});
