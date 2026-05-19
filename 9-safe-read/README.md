# Написать функцию безопасного чтения байта памяти по заданному адресу

- optional<uint8_t> safe_read_uint8(const uint8_t* p)

- Функция не должна падать ни при каких значениях входного параметра

- Многопоточность можно не поддерживать

# Результаты выполнения

```
Mode: release
--- nullptr ---
safe_read_uint8(nullptr): nullopt
--- valid local ---
safe_read_uint8(&local): Some(66)
--- invalid address ---
safe_read_uint8(0x1): nullopt
--- mprotect ---
safe_read_uint8(PROT_NONE page): nullopt
safe_read_uint8(PROT_READ page): Some(126)
--- after fault ---
safe_read_uint8(0x1): nullopt
safe_read_uint8(&local after fault): Some(85)
---
Mode: debug
--- nullptr ---
safe_read_uint8(nullptr): nullopt
--- valid local ---
safe_read_uint8(&local): Some(66)
--- invalid address ---
safe_read_uint8(0x1): nullopt
--- mprotect ---
safe_read_uint8(PROT_NONE page): nullopt
safe_read_uint8(PROT_READ page): Some(126)
--- after fault ---
safe_read_uint8(0x1): nullopt
safe_read_uint8(&local after fault): Some(85)
```

# Проверка в Linux-контейнере

```
docker build -t safe-read .
docker run --rm safe-read
```
