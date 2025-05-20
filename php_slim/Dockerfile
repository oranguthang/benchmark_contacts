FROM php:8.1-cli-alpine

# 1) runtime-libs
RUN apk add --no-cache \
      postgresql-libs \
      libpng \
      libjpeg-turbo \
      freetype \
      zlib

# 2) build-deps → собрать расширения → удалить build-deps
RUN apk add --no-cache --virtual .build-deps \
      autoconf gcc g++ make musl-dev pkgconf \
      libpng-dev libjpeg-turbo-dev freetype-dev zlib-dev postgresql-dev && \
    docker-php-ext-configure gd --with-freetype --with-jpeg && \
    docker-php-ext-install gd pdo pdo_pgsql sockets && \
    apk del .build-deps

WORKDIR /var/www

# 3) Composer + dependencies (в composer.json подключён spiral/roadrunner)
COPY app/composer.json app/composer.lock ./
RUN curl -sS https://getcomposer.org/installer | php && \
    mv composer.phar /usr/local/bin/composer && \
    composer install --no-scripts --no-autoloader && \
    composer dump-autoload --optimize

# 4) код приложения
COPY app /var/www

# передаём CPU_CORES внутрь
ENV CPU_CORES=8

EXPOSE 8080

# 5) запустить RR из vendor/bin
CMD ["vendor/bin/rr", "serve", "-c", ".rr.yaml"]
