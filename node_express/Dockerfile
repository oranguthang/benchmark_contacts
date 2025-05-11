FROM node:18-slim

WORKDIR /app

COPY package.json package-lock.json ./

RUN npm install

COPY ./app /app

EXPOSE 8080

CMD ["node", "index.js"]
