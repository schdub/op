```
/------------------------------------------------------------\
| Мои "велосипеды", ака набор кросскомпилируемых header-only |
| библиотек, которые использую в своих проектах.             |
\------------------------------------------------------------/

* benchmark.hpp - простой бенчмарк с использованием С++11 chrono;

* db.hpp      - обертка над SQLite C API. Выручает если нужно выполнить пару запросов,
                а серьезные зависимости подключать не хочется;

* debug.hpp   - реализация логирования в файл, в консоль, в DebugOutput() под виндовс через
                макрос-функции LOG(), WARN(), с возможностью их полность отключить в релизе;

* eval.hpp   - выполнение текстовой строки, как скрипта. Поддерживается некоторые функции,
               арифметические операции;

* mmap.hpp   - простая обертка над Linux/Windows API реализациями MemoryMappedFiles;

* net.hpp    - обертка над Windows WINSOCK 2 и Linux POSIX реализациями сокетов, есть
               класс TCPSocket, есть класс HTTP, которые позволяют быстро отправить
               какой-то сетевой запрос не заморачивая с зависимостями;

* settings.hpp - простой парсер config файлов (может использоваться и для парсинга INI файлов);

* strutils.hpp - набор утилитарных функций для работы со строками;

* thread.hpp - обертка над WIN32 и PThread реализациями потоков (написана еще до С++11, но
               по прежнему выручает если нужно использовать потоки в компиляторах не
               поддерживающих С++ 11);

* url.hpp    - рарсер URL строки.

```
