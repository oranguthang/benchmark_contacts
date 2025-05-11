FROM php:8.1-fpm-alpine

RUN apk add --no-cache libpng libjpeg libfreetype && \
    docker-php-ext-configure gd --with-freetype --with-jpeg && \
    docker-php-ext-install gd pdo pdo_mysql

WORKDIR /var/www

COPY composer.json composer.lock ./

RUN curl -sS https://getcomposer.org/installer | php && \
    php composer.phar install --no-scripts --no-autoloader

COPY ./app /app

EXPOSE 8080

CMD ["php-fpm"]
