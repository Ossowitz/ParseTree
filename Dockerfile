FROM gcc:latest

# Создайте рабочую директорию в контейнере
WORKDIR /app

# Скопируйте файлы программы в контейнер
COPY . /app

# Соберите программу
RUN gcc -o parseTree parseTree.c -lm

# Укажите команду для запуска программы и передачи аргументов командной строки
#CMD ["./parseTree", "parse 1+2-3*(4/5)%6!^7^8*9+x", "evaluate x=10"]
#CMD ["./parseTree", "parse 1+2-3*(4/5)%6!^7^8*9+x", "evaluate x=10", "save_prf", "save_pst", "load_prf +(2,*(2,2))", "evaluate", "load_pst (2,(2,2)*)+", "evaluate"]
CMD ["./parseTree"]