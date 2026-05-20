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
--- initializations ---
a: non-unique string: ""
b: non-unique string: ""
c: non-unique string: "hello"
d: non-unique string: "hello"
--- assignments ---
before:
  a: non-unique string: ""
  c: non-unique string: "hello"
  copy: non-unique string: "hello"
after c = "world": unique string: "world"
after a = c:
  a: non-unique string: "world"
  c: non-unique string: "world"
after c = string_ptr("tmp"):
  a: non-unique string: "world"
  c: unique string: "tmp"
Debug: deallocating memory for string: "tmp"
--- bubble sort ---
before:
  [0] unique string: "delta"
  [1] unique string: "alpha"
  [2] unique string: "charlie"
  [3] unique string: "bravo"
  [4] unique string: "alpha"
  [5] unique string: "echo"
after:
  [0] unique string: "alpha"
  [1] unique string: "alpha"
  [2] unique string: "bravo"
  [3] unique string: "charlie"
  [4] unique string: "delta"
  [5] unique string: "echo"
Debug: deallocating memory for string: "alpha"
Debug: deallocating memory for string: "alpha"
Debug: deallocating memory for string: "bravo"
Debug: deallocating memory for string: "charlie"
Debug: deallocating memory for string: "delta"
Debug: deallocating memory for string: "echo"
```

# Проверка в Linux-контейнере

```
docker build -t string-collector .
docker run --rm string-collector
```