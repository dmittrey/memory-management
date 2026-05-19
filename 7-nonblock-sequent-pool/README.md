# Неблокирующая реализация растущего вниз последовательного пула для элементов списка

- На основе sequent-pool

- Сравнить по затратам времени и памяти 4 аллокатора элементов списка
    - Стандартный (new/malloc)
    - Глобальный последовательный пул под мьютексом
    - Глобальный неблокирующий последовательный пул
    - Локальный в потоках последовательные пулы

- Измерения произвести для 16 потоков, каждый из которых отводит и освобождает список длиной 10М

- Обработчик SIGSEGV в сообщении должен указывать, в каком из пулов произошло переполнение

# Результаты измерений

```
Mode: default new/delete
Time used: 38.151 sec
Memory used: 4.770 GB
Overhead: 50.0%
---
Mode: global mutex pool
Time used: 59.612 sec
Memory used: 2.386 GB
Overhead:  0.1%
---
Mode: global lock-free pool
Time used: 73.975 sec
Memory used: 2.386 GB
Overhead:  0.1%
---
Mode: thread-local pools
Time used: 2.774 sec
Memory used: 1.542 GB
Overhead: 90.3%
```

# Проверка в Linux-контейнере

```
docker build -t nonblock-sequent-pool .
docker run --rm nonblock-sequent-pool
```