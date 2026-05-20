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
Standard Allocator:
Time used: 18.459 sec
Memory used: 4.770 GB
Overhead: 50.015%
---
Global mutexed pool:
Time used: 14.664 sec
Memory used: 2.386 GB
Overhead: 0.057%
---
Global lock-free pool:
Time used: 22.215 sec
Memory used: 2.386 GB
Overhead: 0.059%
---
Thread-local pools:
Time used: 1.274 sec
Memory used: 1.541 GB
Overhead: -54.669%
```

# Проверка в Linux-контейнере

```
docker build -t nonblock-sequent-pool .
docker run --rm nonblock-sequent-pool
```