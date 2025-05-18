FROM php:8.1-cli-alpine

RUN apk add --no-cache --virtual .build-deps \
    autoconf gcc g++ make musl-dev pkgconf \
    libpng-dev libjpeg-turbo-dev freetype-dev zlib-dev postgresql-dev && \
    docker-php-ext-configure gd --with-freetype --with-jpeg && \
    docker-php-ext-install gd pdo pdo_pgsql && \
    apk del .build-deps

WORKDIR /var/www

COPY app/composer.json ./

RUN curl -sS https://getcomposer.org/installer | php && \
    php composer.phar install --no-scripts --no-autoloader && \
    php composer.phar dump-autoload --optimize

COPY ./app /var/www

EXPOSE 8080

CMD ["php", "-S", "0.0.0.0:8080", "index.php"]
