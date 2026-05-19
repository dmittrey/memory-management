# Написать маленькую однопоточную библиотеку для автоматического освобождения строк

- Использовать однобитовые счётчики ссылок и «умные» указатели

- Набор операций должен включать:
    - Создание умного указателя, инициализированного по умолчанию, строкой и другим указателем
    - Присваивание умному указателю строки и умного указателя
    - Извлечение из умного указателя строки
    - Печать умного указателя, включая бит уникальности и значение
    - Трассировку освобождения строк в режиме отладки
    - Любые другие удобные и естественные операции

- Набор тестов должен включать разнообразные инициализации, присваивания и пузырьковую сортировку массива умных указателей на строки
    - Сортировка должна сохранять счётчики ссылок элементов массива

# Результаты выполнения

```
Mode: release
--- initializations ---
a: {null}
b: {null}
c: {unique=0, "hello"}
d: {unique=0, "hello"}
--- assignments ---
before:
  a: {null}
  c: {unique=0, "hello"}
  copy: {unique=0, "hello"}
after c = "world": {unique=1, "world"}
after a = c:
  a: {unique=0, "world"}
  c: {unique=0, "world"}
after c = SmartString("tmp"):
  a: {unique=0, "world"}
  c: {unique=1, "tmp"}
--- bubble sort ---
before:
  [0] {unique=1, "delta"}
  [1] {unique=1, "alpha"}
  [2] {unique=1, "charlie"}
  [3] {unique=1, "bravo"}
  [4] {unique=1, "alpha"}
  [5] {unique=1, "echo"}
after:
  [0] {unique=1, "alpha"}
  [1] {unique=1, "alpha"}
  [2] {unique=1, "bravo"}
  [3] {unique=1, "charlie"}
  [4] {unique=1, "delta"}
  [5] {unique=1, "echo"}
---
Mode: debug (release tracing enabled)
--- initializations ---
a: {null}
b: {null}
c: {unique=0, "hello"}
d: {unique=0, "hello"}
--- assignments ---
before:
  a: {null}
  c: {unique=0, "hello"}
  copy: {unique=0, "hello"}
after c = "world": {unique=1, "world"}
after a = c:
  a: {unique=0, "world"}
  c: {unique=0, "world"}
after c = SmartString("tmp"):
  a: {unique=0, "world"}
  c: {unique=1, "tmp"}
--- bubble sort ---
before:
  [0] {unique=1, "delta"}
  [1] {unique=1, "alpha"}
  [2] {unique=1, "charlie"}
  [3] {unique=1, "bravo"}
  [4] {unique=1, "alpha"}
  [5] {unique=1, "echo"}
after:
  [0] {unique=1, "alpha"}
  [1] {unique=1, "alpha"}
  [2] {unique=1, "bravo"}
  [3] {unique=1, "charlie"}
  [4] {unique=1, "delta"}
  [5] {unique=1, "echo"}
[trace] release "tmp" @ 0x168b6350
[trace] release "alpha" @ 0x168b63c0
[trace] release "alpha" @ 0x168b64d0
[trace] release "bravo" @ 0x168b6450
[trace] release "charlie" @ 0x168b6420
[trace] release "delta" @ 0x168b6350
[trace] release "echo" @ 0x168b63f0
```

# Проверка в Linux-контейнере

```
docker build -t string-collector .
docker run --rm string-collector
```