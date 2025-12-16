# 1) Stage "roadrunner": get official Go binary rr
FROM ghcr.io/roadrunner-server/roadrunner:2025.1.1 AS roadrunner

# 2) Stage "app": base PHP-CLI
FROM php:8.2-cli-alpine

# 3) Copy rr from first stage
COPY --from=roadrunner /usr/bin/rr /usr/local/bin/rr

# 4) Install runtime libraries
RUN apk add --no-cache \
      postgresql-libs \
      libpng \
      libjpeg-turbo \
      freetype \
      zlib

# 5) Install dev dependencies, build extensions and clean them
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
      postgresql-dev \
      protobuf-dev && \
    pecl install protobuf && \
    docker-php-ext-enable protobuf && \
    docker-php-ext-configure gd --with-freetype --with-jpeg && \
    docker-php-ext-install gd pdo pdo_pgsql sockets && \
    apk del .build-deps

WORKDIR /var/www

# 6) Copy composer manifests and install dependencies
COPY app/composer.json app/composer.lock ./
RUN curl -sS https://getcomposer.org/installer | php && \
    mv composer.phar /usr/local/bin/composer && \
    composer install --no-scripts --no-autoloader && \
    composer dump-autoload --optimize

# 7) Copy all application code
COPY app /var/www

# 8) Pass CPU_CORES inside
ENV CPU_CORES=8

EXPOSE 8080
CMD ["rr", "serve", "-c", ".rr.yaml"]
