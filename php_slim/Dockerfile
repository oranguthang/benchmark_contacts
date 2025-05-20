FROM php:8.2-cli-alpine

# runtime-libs для GD и pdo_pgsql
RUN apk add --no-cache \
      postgresql-libs \
      libpng \
      libjpeg-turbo \
      freetype \
      zlib

# build-deps → собрать расширения → удалить build-deps
RUN apk add --no-cache --virtual .build-deps \
      autoconf \
      gcc \
      g++ \
      make \
      musl-dev \
      pkgconf \
      linux-headers \
      libpng-dev \
      libjpeg-turbo-dev \
      freetype-dev \
      zlib-dev \
      postgresql-dev && \
    docker-php-ext-configure gd --with-freetype --with-jpeg && \
    docker-php-ext-install gd pdo pdo_pgsql sockets && \
    apk del .build-deps

WORKDIR /var/www

# composer
COPY app/composer.json app/composer.lock ./
RUN curl -sS https://getcomposer.org/installer | php && \
    mv composer.phar /usr/local/bin/composer && \
    composer install --no-scripts --no-autoloader && \
    composer dump-autoload --optimize

# код приложения
COPY app /var/www

# передаём CPU_CORES внутрь
ENV CPU_CORES=8

EXPOSE 8080

# запускаем RoadRunner
CMD ["vendor/bin/rr", "serve", "-c", ".rr.yaml"]
