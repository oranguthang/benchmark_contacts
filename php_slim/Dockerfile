FROM php:8.1-cli-alpine

# 1) Runtime-libs для GD + pdo_pgsql
RUN apk add --no-cache \
      postgresql-libs \
      libpng \
      libjpeg-turbo \
      freetype \
      zlib

# 2) Build-deps → сборка php-расширений → удаление build-deps
RUN apk add --no-cache --virtual .build-deps \
      autoconf gcc g++ make musl-dev pkgconf \
      libpng-dev libjpeg-turbo-dev freetype-dev zlib-dev postgresql-dev && \
    docker-php-ext-configure gd --with-freetype --with-jpeg && \
    docker-php-ext-install gd pdo pdo_pgsql sockets && \
    apk del .build-deps

WORKDIR /var/www

# 3) composer
COPY app/composer.json ./
RUN curl -sS https://getcomposer.org/installer | php && \
    mv composer.phar /usr/local/bin/composer && \
    composer install --no-scripts --no-autoloader && \
    composer dump-autoload --optimize

# 4) код
COPY app /var/www

# 5) RoadRunner CLI (Go-бинарь)
RUN apk add --no-cache curl tar && \
    RR_VER=2.12.3 && \
    curl -sSL "https://github.com/roadrunner-server/roadrunner/releases/download/v${RR_VER}/roadrunner_${RR_VER}_Linux_x86_64.tar.gz" \
      | tar -xz -C /usr/local/bin

# количество воркеров берём из переменной
ENV CPU_CORES=8

EXPOSE 8080

# финальная команда — настоящий rr
CMD ["rr","serve","-c",".rr.yaml"]
