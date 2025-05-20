FROM php:8.1-cli-alpine

# runtime: libpq для pdo_pgsql
RUN apk add --no-cache postgresql-libs

# build & compile: GD, PDO_PGSQL и sockets
RUN apk add --no-cache --virtual .build-deps \
      autoconf gcc g++ make musl-dev pkgconf \
      libpng-dev libjpeg-turbo-dev freetype-dev zlib-dev postgresql-dev && \
    docker-php-ext-configure gd --with-freetype --with-jpeg && \
    docker-php-ext-install gd pdo pdo_pgsql sockets && \
    apk del .build-deps

WORKDIR /var/www

COPY app/composer.json ./

RUN curl -sS https://getcomposer.org/installer | php && \
    mv composer.phar /usr/local/bin/composer && \
    composer install --no-scripts --no-autoloader && \
    composer dump-autoload --optimize

COPY ./app /var/www

ENV CPU_CORES=8

EXPOSE 8080

CMD ["composer", "rr", "serve", "-c", ".rr.yaml"]
