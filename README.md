# SolverUNIX Kernel

**Ядро операционной системы SolverUNIX KNG**

**The kernel of SolverUNIX KNG operating system**

![Architecture](https://img.shields.io/badge/Architecture-x86__64-blue)
![Language](https://img.shields.io/badge/Language-C%20%2F%20Assembly-orange)
![Status](https://img.shields.io/badge/Status-Experimental-red)

---

# 🇷🇺 Русский

## О проекте

**SolverUNIX** — независимая UNIX-подобная операционная система, разрабатываемая в рамках проекта SolverUNIX KNG.

Главная цель проекта — создание собственной операционной системы с независимым ядром, системными библиотеками, утилитами, системой инициализации и инфраструктурой управления пакетами.

SolverUNIX не является дистрибутивом Linux и не использует ядро Linux.

Проект стремится создать собственную программную экосистему с нуля.

## Цели проекта

- Разработка собственного ядра для архитектуры x86_64.
- Создание `solibc` — собственной реализации стандартной библиотеки C.
- Разработка `sol6` — собственной системы инициализации.
- Реализация управления памятью и процессами.
- Разработка собственных системных вызовов.
- Создание модульной архитектуры драйверов.
- Разработка собственной пользовательской среды.
- Поддержка загрузки ELF-программ.

## Архитектура

| Компонент | Описание |
|---|---|
| `SolverUNIX Kernel` | Основное ядро ОС |
| `solibc` | Собственная библиотека C |
| `sol6` | Система инициализации |
| `solas` | Интерфейс системных команд |
| `solpkg` | Система управления пакетами |
| `KFS` | Файловая инфраструктура SolverUNIX |

Некоторые компоненты находятся в планах разработки и могут быть ещё не реализованы.

## Целевая платформа

| Свойство | Значение |
|---|---|
| Архитектура | x86_64 |
| Платформа | Bare metal |
| Формат исполняемых файлов | ELF |
| Целевой компилятор | `x86_64-elf` |
| Система сборки | GNU Make |
| Языки | C, Assembly |

## Разработка

Ядро SolverUNIX находится на экспериментальной стадии активной разработки.

Основной принцип разработки — независимость от существующих ядер операционных систем.

### Необходимые инструменты

- GNU Make
- `x86_64-elf-gcc`
- `x86_64-elf-binutils`
- ELF-совместимый линковщик
- QEMU для виртуального тестирования

**Важно:** MinGW GCC (`x86_64-w64-mingw32`) предназначен для создания Windows-программ и не заменяет кросс-компилятор `x86_64-elf`.

### Сборка

Инструкция по настройке MSYS2 и сборке в Windows: [docs/BUILD_MSYS2.md](docs/BUILD_MSYS2.md).

## Структура проекта

Структура каталогов может изменяться в процессе разработки.

```text
SolverUNIX/
├── kernel/
├── include/
├── linker.ld
├── Makefile
└── README.md
```

## Дорожная карта

- [ ] Настройка загрузки x86_64
- [ ] Точка входа ядра
- [ ] Базовый вывод в терминал
- [ ] GDT и IDT
- [ ] Обработка прерываний
- [ ] Управление физической памятью
- [ ] Управление виртуальной памятью
- [ ] Базовый планировщик процессов
- [ ] Интерфейс системных вызовов
- [ ] ELF-загрузчик
- [ ] Инфраструктура драйверов
- [ ] Интеграция с solibc
- [ ] Интеграция с sol6

## Философия проекта

SolverUNIX строится вокруг одной основной идеи:

**Независимость.**

Проект направлен на создание собственного ядра и собственной системной инфраструктуры вместо использования готового ядра другой операционной системы.

## Лицензия

Информация о лицензии будет добавлена позднее.

---

# 🇬🇧 English

## About

**SolverUNIX** is an independent UNIX-like operating system developed as part of the SolverUNIX KNG project.

The main goal is to build an operating system with its own independent kernel, system libraries, utilities, initialization system and package management infrastructure.

SolverUNIX is not a Linux distribution and does not use the Linux kernel.

The project aims to build its own software ecosystem from the ground up.

## Project goals

- Develop an independent x86_64 kernel.
- Create `solibc` — a native C standard library implementation.
- Develop `sol6` — a native initialization system.
- Implement memory and process management.
- Develop native system calls.
- Create a modular driver architecture.
- Build a native userland environment.
- Support ELF executable loading.

## Architecture

| Component | Description |
|---|---|
| `SolverUNIX Kernel` | Main operating system kernel |
| `solibc` | Native C library |
| `sol6` | Initialization system |
| `solas` | System command interface |
| `solpkg` | Package management system |
| `KFS` | SolverUNIX filesystem infrastructure |

Some components are planned and may not yet be implemented.

## Target platform

| Property | Value |
|---|---|
| Architecture | x86_64 |
| Platform | Bare metal |
| Executable format | ELF |
| Compiler target | `x86_64-elf` |
| Build system | GNU Make |
| Languages | C, Assembly |

## Development

SolverUNIX Kernel is currently experimental and under active development.

The main development principle is independence from existing operating system kernels.

### Requirements

- GNU Make
- `x86_64-elf-gcc`
- `x86_64-elf-binutils`
- An ELF-compatible linker
- QEMU for virtual machine testing

**Important:** MinGW GCC (`x86_64-w64-mingw32`) targets Windows and is not a replacement for the `x86_64-elf` cross-compiler.

### Building

See [docs/BUILD_MSYS2.md](docs/BUILD_MSYS2.md) for instructions on setting up MSYS2 and building on Windows.

## Project structure

The directory structure is subject to change during development.

```text
SolverUNIX/
├── kernel/
├── include/
├── linker.ld
├── Makefile
└── README.md
```

## Roadmap

- [ ] Establish the x86_64 boot process
- [ ] Kernel entry point
- [ ] Basic terminal output
- [ ] GDT and IDT
- [ ] Interrupt handling
- [ ] Physical memory management
- [ ] Virtual memory management
- [ ] Basic process scheduler
- [ ] System call interface
- [ ] ELF loader
- [ ] Driver infrastructure
- [ ] Integration with solibc
- [ ] Integration with sol6

## Project philosophy

SolverUNIX is built around one central idea:

**Independence.**

The project aims to develop its own kernel and native system infrastructure instead of relying on an existing operating system kernel.

## License

License information will be added later.

---

**SolverUNIX KNG**

*Independent by design.*

*Независимость — основа проекта.*
