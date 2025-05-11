FROM openjdk:17-jdk-slim

WORKDIR /app

COPY ./app /app

RUN ./mvnw clean package -DskipTests

EXPOSE 8084

CMD ["java", "-jar", "target/benchmark_contacts_java.jar"]
