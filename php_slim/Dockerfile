FROM php:8.1-cli-alpine

# runtime-libs для GD и PostgreSQL
RUN apk add --no-cache \
      postgresql-libs \
      libpng \
      libjpeg-turbo \
      freetype \
      zlib

# build-deps, сборка расширений и их удаление
RUN apk add --no-cache --virtual .build-deps \
      autoconf gcc g++ make musl-dev pkgconf \
      libpng-dev libjpeg-turbo-dev freetype-dev zlib-dev postgresql-dev && \
    docker-php-ext-configure gd --with-freetype --with-jpeg && \
    docker-php-ext-install gd pdo pdo_pgsql sockets && \
    apk del .build-deps

WORKDIR /var/www

# composer
COPY app/composer.json ./
RUN curl -sS https://getcomposer.org/installer | php && \
    mv composer.phar /usr/local/bin/composer && \
    composer install --no-scripts --no-autoloader && \
    composer dump-autoload --optimize

# код приложения
COPY app /var/www

# чтобы можно было вызвать rr напрямую
ENV CPU_CORES=8 \
    PATH="/var/www/vendor/bin:${PATH}"

EXPOSE 8080

# запускаем RoadRunner сразу из vendor/bin
CMD ["rr", "serve", "-c", ".rr.yaml"]
